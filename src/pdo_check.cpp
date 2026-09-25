// Shared verified bus setup: no-enable communication test or bounded PP trial.
// Reference: LC-E manual sections 6.1, 6.3, 6.4.2 and SOEM v2.0.0 simple_ng.
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>
#include <signal.h>
#include <time.h>
#include "soem/soem.h"
#include "sdo_trace.h"
#include "lc10e_esi.h"
#include "pdo_iomap.h"
#include "motion_trial.h"

namespace {
constexpr int64_t period_ns = 1000000; // LC-E supports 1 ms for CSP.
constexpr int test_cycles = 5000;

void errors(ecx_contextt *ctx)
{
    while (ecx_iserror(ctx)) printf("SOEM: %s", ecx_elist2string(ctx));
}

template<typename T>
bool read_sdo(ecx_contextt *ctx, uint16_t index, uint8_t sub, T &value)
{
    int size = sizeof(value);
    begin_sdo_trace(index, sub, sizeof(value));
    const int wkc = ecx_SDOread(ctx, 1, index, sub, FALSE, &size, &value, EC_TIMEOUTRXM);
    if (!finish_sdo_trace(wkc, size)) {
        printf("Gagal baca SDO %04X:%02X (ukuran %d, expected %zu).\n",
               index, sub, size, sizeof(value));
        errors(ctx);
        return false;
    }
    return true;
}

void states(ecx_contextt *ctx)
{
    ecx_readstate(ctx);
    for (int i = 1; i <= ctx->slavecount; ++i)
        printf("Slave %d: state=0x%02X ALstatus=0x%04X (%s)\n", i,
               ctx->slavelist[i].state, ctx->slavelist[i].ALstatuscode,
               ec_ALstatuscode2string(ctx->slavelist[i].ALstatuscode));
    errors(ctx);
}

void request_state(ecx_contextt *ctx, uint16_t wanted)
{
    // Address each known slave, so the write response identifies the recipient.
    for (int i = 1; i <= ctx->slavecount; ++i) {
        ctx->slavelist[i].state = wanted;
        const int wkc = ecx_writestate(ctx, i);
        printf("Request slave %d addr=0x%04X -> state=0x%02X: write WKC=%d\n",
               i, ctx->slavelist[i].configadr, wanted, wkc);
    }
}

bool reached(ecx_contextt *ctx, int slave, uint16_t wanted, int timeout)
{
    // SOEM's return value masks out AL error bits; inspect the full cached state.
    const uint16_t actual = ecx_statecheck(ctx, slave, wanted, timeout);
    return actual == wanted && ctx->slavelist[slave].state == wanted;
}

void state_registers(ecx_contextt *ctx, int slave)
{
    const uint16_t regs[] = {ECT_REG_STADR, ECT_REG_ALCTL, ECT_REG_ALSTAT, ECT_REG_ALSTATCODE};
    const char *names[] = {"Station address", "AL control", "AL status", "AL status code"};
    for (unsigned i = 0; i < sizeof(regs) / sizeof(regs[0]); ++i) {
        uint16_t value = 0;
        const int wkc = ecx_FPRD(&ctx->port, ctx->slavelist[slave].configadr,
                                  regs[i], sizeof(value), &value, EC_TIMEOUTRET);
        if (wkc > 0)
            printf("  Slave %d %s [%04X]=%04X read WKC=%d\n", slave,
                   names[i], regs[i], etohs(value), wkc);
        else
            printf("  Slave %d %s [%04X]: baca gagal, WKC=%d\n", slave, names[i], regs[i], wkc);
    }
}

bool state(ecx_contextt *ctx, uint16_t wanted)
{
    request_state(ctx, wanted);
    bool ok = true;
    // Always wait for actual state, even when a write acknowledgement was lost.
    for (int i = 1; i <= ctx->slavecount; ++i) {
        if (!reached(ctx, i, wanted, EC_TIMEOUTSTATE)) {
            printf("Slave %d gagal mencapai state 0x%02X; actual=0x%02X.\n",
                   i, wanted, ctx->slavelist[i].state);
            state_registers(ctx, i);
            ok = false;
        }
    }
    if (!ok) states(ctx);
    return ok;
}

// Derive offsets from the drive, not a packed struct or a particular PDO number.
// Use the active assignment unchanged: firmware may differ from the manual.
struct Layout {
    uint16_t assignment;
    uint16_t mapping = 0;
    std::vector<uint32_t> entries;
    int bytes = 0;
    int control = -1, target = -1, mode = -1, max_speed = -1;
    int status = -1, error = -1, position = -1;

    explicit Layout(uint16_t index) : assignment(index) {}

    bool from_esi = false;

    bool load(ecx_contextt *ctx, bool verbose = true) {
        uint8_t count = 0;
        if (!read_sdo(ctx, assignment, 0, count)) return false;
        if (count != 1) {
            printf("Assignment %04X jumlah=%u; uji mendukung satu PDO per arah.\n", assignment, count);
            return false;
        }
        uint16_t wire_mapping = 0;
        if (!read_sdo(ctx, assignment, 1, wire_mapping)) return false;
        mapping = etohs(wire_mapping);
        const bool is_rx = (assignment == 0x1c12);
        if (mapping < (is_rx ? 0x1600u : 0x1a00u) || mapping > (is_rx ? 0x17ffu : 0x1bffu)) {
            printf("Assignment %04X menunjuk PDO tidak valid: %04X.\n", assignment, mapping);
            return false;
        }
        if (!read_sdo(ctx, mapping, 0, count)) return false;
        if (verbose) printf("Assignment %04X -> PDO %04X: %u entri aktual\n", assignment, mapping, count);
        if (!count || count > 32) {
            printf("Jumlah entri PDO di luar batas uji (1..32).\n");
            return false;
        }

        entries.clear();
        if (from_esi) {
            const uint16_t expected = is_rx ? lc10e_esi::rx_index : lc10e_esi::tx_index;
            const size_t expected_count = is_rx ? std::size(lc10e_esi::rx) : std::size(lc10e_esi::tx);
            if (!lc10e_esi::matches(ctx) || mapping != expected || count != expected_count) {
                printf("Identitas/assignment/jumlah entri tidak cocok dengan ESI; dibatalkan.\n");
                return false;
            }
            if (is_rx) entries.assign(std::begin(lc10e_esi::rx), std::end(lc10e_esi::rx));
            else entries.assign(std::begin(lc10e_esi::tx), std::end(lc10e_esi::tx));
            if (verbose) printf("  Descriptor bersumber dari LC10E V1.04.xml (fixed PDO).\n");
        } else {
            for (uint8_t i = 1; i <= count; ++i) {
                uint32_t wire = 0;
                if (!read_sdo(ctx, mapping, i, wire)) return false;
                entries.push_back(etohl(wire));
            }
        }
        if (verbose) for (size_t i = 0; i < entries.size(); ++i) {
            const auto e = entries[i];
            printf("  %04X:%02zX = %08X -> %04X:%02X, %u bit\n",
                   mapping, i + 1, e, e >> 16, (e >> 8) & 0xff, e & 0xff);
        }
        return true;
    }

    bool validate() {
        const bool is_rx = (assignment == 0x1c12);
        for (size_t i = 0; i < entries.size(); ++i) {
            const uint32_t entry = entries[i];
            const uint16_t index = entry >> 16;
            const uint8_t sub = (entry >> 8) & 0xff, bits = entry & 0xff;
            if (!bits || bits % 8 || bytes + bits / 8 > 256) {
                printf("PDO %04X:%02zX: panjang/alignment tidak didukung.\n", mapping, i + 1);
                return false;
            }
            int expected_bits = 0;
            int *offset = nullptr;
            if (is_rx) {
                switch (index) {
                case 0x0000: if (!sub) expected_bits = bits; break; // Padding.
                case 0x6040: expected_bits = 16; offset = &control; break;
                case 0x607a: expected_bits = 32; offset = &target; break;
                case 0x6060: expected_bits = 8; offset = &mode; break;
                case 0x607f: expected_bits = 32; offset = &max_speed; break;
                case 0x60ff: case 0x60b1: expected_bits = 32; break; // Zero velocity.
                case 0x6071: case 0x60b2: case 0x60b8: expected_bits = 16; break; // Zero torque/probe.
                default: break;
                }
            } else {
                // Other input fields are read-only and can safely be skipped.
                expected_bits = bits;
                if (index == 0x6041) { expected_bits = 16; offset = &status; }
                if (index == 0x603f) { expected_bits = 16; offset = &error; }
                if (index == 0x6064) { expected_bits = 32; offset = &position; }
            }
            bool duplicate = false;
            if (is_rx && index) {
                for (size_t j = 0; j < i; ++j)
                    if ((entries[j] >> 8) == (entry >> 8)) duplicate = true;
            }
            if (bits != expected_bits || ((is_rx || offset) && sub != 0) ||
                (offset && *offset >= 0) || duplicate) {
                printf("PDO %04X:%02zX tidak aman untuk uji: %04X:%02X/%u bit "
                       "(objek, ukuran, subindex, atau duplikasi).\n", mapping, i + 1, index, sub, bits);
                return false;
            }
            if (offset) *offset = bytes;
            bytes += bits / 8;
        }
        if ((is_rx && (control < 0 || target < 0)) ||
            (!is_rx && (status < 0 || error < 0 || position < 0))) {
            printf("PDO %04X tidak memuat objek wajib untuk uji posisi tanpa enable.\n", mapping);
            return false;
        }
        printf("PDO %04X tervalidasi: %d byte.\n", mapping, bytes);
        return true;
    }

    bool unchanged(ecx_contextt *ctx) const {
        Layout current(assignment);
        current.from_esi = from_esi;
        if (!current.load(ctx, false) || current.mapping != mapping || current.entries != entries) {
            printf("Assignment/mapping berubah selama setup; uji dibatalkan.\n");
            return false;
        }
        return true;
    }
};

int64_t ns(const timespec &t) { return int64_t(t.tv_sec) * 1000000000 + t.tv_nsec; }
timespec as_time(int64_t t) { return {time_t(t / 1000000000), long(t % 1000000000)}; }

// Absolute monotonic deadlines avoid accumulating the processing time as drift.
// Phase correction follows the DC reference; late cycles are counted, not hidden.
class CycleClock {
    int64_t next;
public:
    int late_cycles = 0;
    int64_t max_late_ns = 0;
    CycleClock() {
        timespec t{};
        clock_gettime(CLOCK_MONOTONIC, &t);
        next = ns(t);
    }
    bool wait(int64_t dc, const volatile sig_atomic_t *running) {
        int64_t phase = (dc - 50000) % period_ns;
        if (phase < -period_ns / 2) phase += period_ns;
        if (phase > period_ns / 2) phase -= period_ns;
        next += period_ns - (dc ? phase / 100 : 0);
        const timespec deadline = as_time(next);
        int rc;
        do { rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr); }
        while (rc == EINTR && *running);
        if (!*running || rc != 0) return false;
        timespec now{};
        clock_gettime(CLOCK_MONOTONIC, &now);
        const int64_t late = ns(now) - next;
        max_late_ns = std::max(max_late_ns, late);
        if (late >= period_ns) {
            ++late_cycles;
            next = ns(now); // Do not send a burst to catch up missed deadlines.
        }
        return true;
    }
};

uint16_t get16(const uint8_t *p) { uint16_t x; memcpy(&x, p, 2); return etohs(x); }
int32_t get32(const uint8_t *p) { uint32_t x; memcpy(&x, p, 4); return int32_t(etohl(x)); }

bool exchange(ecx_contextt *ctx, int expected, bool check_wkc,
              const Layout &rx, const Layout &tx)
{
    auto &s = ctx->slavelist[1];
    // All other outputs retain their verified zero/preserved initialization.
    memset(s.outputs + rx.control, 0, 2);
    ecx_send_processdata(ctx);
    const int wkc = ecx_receive_processdata(ctx, EC_TIMEOUTRET);
    if (check_wkc && wkc != expected) {
        printf("GAGAL: WKC=%d expected=%d; data tidak digunakan.\n", wkc, expected);
        return false;
    }
    if (wkc == expected) {
        const uint16_t sw = get16(s.inputs + tx.status);
        if (sw & 0x0004) {
            printf("GAGAL: status servo masih enabled (SW=%04X).\n", sw);
            return false;
        }
        // Hold the reported position even though operation stays disabled.
        memcpy(s.outputs + rx.target, s.inputs + tx.position, 4);
    }
    return true;
}
} // namespace

static int run_bus(const char *ifname, const volatile sig_atomic_t *running, bool use_esi, MotionTrial *motion)
{
    if (motion) printf("Mode: uji putar PP 4\u00d7360 derajat +524288 unit, poros bebas; enable hanya setelah validasi.\n");
    else printf("Mode: uji PDO/DC 5000 siklus, 1 ms; control word selalu 0x0000.\n");
    auto storage = std::make_unique<ecx_contextt>();
    auto *ctx = storage.get();
    if (!ecx_init(ctx, ifname)) {
        printf("Gagal membuka '%s'; periksa antarmuka dan hak raw socket.\n", ifname);
        return 1;
    }
    // A buffer larger than all possible I/O bits for the single accepted slave.
    uint8_t iomap[16384]{};
    Layout rx{0x1c12}, tx{0x1c13};
    rx.from_esi = tx.from_esi = use_esi;
    if (use_esi) printf("Profil ESI LC10E V1.04: 766/402/204, PDO 1702/1B02, 15/28 byte.\n");
    bool dc_active = false;
    bool ok = [&]() {
        if (ecx_config_init(ctx) <= 0) {
            printf("Tidak ada slave ditemukan.\n");
            return false;
        }
        auto &s = ctx->slavelist[1];
        printf("Slave count=%d VendorID=%08X ProductCode=%08X DC=%d\n",
               ctx->slavecount, s.eep_man, s.eep_id, s.hasdc);
        printf("Identitas EEPROM: name=%s revision=%08X\n", s.name, s.eep_rev);
        if (ctx->slavecount != 1 || s.eep_man != 0x766 || s.eep_id != 0x402) {
            printf("Uji hanya mendukung satu drive yang telah teridentifikasi (766/402).\n");
            return false;
        }
        if (use_esi && !lc10e_esi::matches(ctx)) {
            printf("Identitas/revision berbeda dari XML; uji ESI dibatalkan.\n");
            return false;
        }
        // Scan previously reported INIT: wait explicitly for PRE-OP before SDO.
        if (!state(ctx, EC_STATE_PRE_OP)) return false;
        uint16_t sw = 0, error = 0;
        uint32_t position = 0;
        int8_t mode = 0;
        if (!read_sdo(ctx, 0x6041, 0, sw) || !read_sdo(ctx, 0x603f, 0, error) ||
            !read_sdo(ctx, 0x6064, 0, position) || !read_sdo(ctx, 0x6061, 0, mode)) return false;
        printf("Awal: SW=%04X ERR=%04X posisi=%d mode=%d\n",
               etohs(sw), etohs(error), int32_t(etohl(position)), mode);
        /* Jika drive fault (dari sesi sebelumnya yang crash), coba fault reset
         * sebelum batal. Hanya untuk mode motion (rem sudah dilepas, aman). */
        const bool has_fault = (etohs(sw) & 8) || etohs(error);
        if ((etohs(sw) & 0x0004) || (motion && has_fault)) {
            if (motion && has_fault && !(etohs(sw) & 0x0004)) {
                printf("Drive fault terdeteksi (SW=%04X ERR=%04X); mencoba fault reset...\n",
                       etohs(sw), etohs(error));
                /* CiA402 Fault Reset: bit 7 rising edge pada 6040h */
                uint16_t cw_reset = htoes(0x0080); int sz = 2;
                ecx_SDOwrite(ctx, 1, 0x6040, 0, FALSE, sz, &cw_reset, EC_TIMEOUTRXM);
                osal_usleep(100000);  /* 100ms: drive proses reset */
                uint16_t cw_zero = 0; sz = 2;
                ecx_SDOwrite(ctx, 1, 0x6040, 0, FALSE, sz, &cw_zero, EC_TIMEOUTRXM);
                osal_usleep(100000);
                /* Baca ulang status setelah reset */
                if (!read_sdo(ctx, 0x6041, 0, sw) || !read_sdo(ctx, 0x603f, 0, error)) return false;
                printf("Setelah reset: SW=%04X ERR=%04X\n", etohs(sw), etohs(error));
                if ((etohs(sw) & 8) || etohs(error)) {
                    printf("Fault tidak bersih setelah reset (ERR=%04X); matikan dan hidupkan drive.\n", etohs(error));
                    return false;
                }
                printf("Fault berhasil direset; melanjutkan uji.\n");
            } else {
                printf("Servo sudah enabled atau fault permanen; uji dibatalkan.\n");
                return false;
            }
        }
        if (!s.hasdc) { printf("Drive tidak menyediakan DC.\n"); return false; }
        // Dump both active maps completely before deciding if their fields are safe.
        const bool rx_read = rx.load(ctx), tx_read = tx.load(ctx);
        if (!rx_read || !tx_read || !rx.validate() || !tx.validate() || !*running) return false;
        if (motion) printf("Assignment aktif dipertahankan; parameter profil PP sementara akan diverifikasi.\n");
        else printf("Menggunakan assignment aktif tanpa menulis SDO/mengganti PDO.\n");
        int8_t commanded_mode = 0;
        uint32_t maximum_speed = 0;
        if (rx.mode >= 0 && !read_sdo(ctx, 0x6060, 0, commanded_mode)) return false;
        if (rx.max_speed >= 0 && !read_sdo(ctx, 0x607f, 0, maximum_speed)) return false;

        if (motion) {
            if (!motion->prepare(ctx)) return false;
            commanded_mode = 1;
            maximum_speed = htoel(MotionTrial::speed);
        }
        if (!*running) return false;
        ctx->manualstatechange = 1;
        if (use_esi && !lc10e_esi::begin_mapping(ctx)) return false;
        const int mapped_bytes = ecx_config_map_group(ctx, iomap, 0);
        const bool supplied = use_esi ? lc10e_esi::end_mapping() : false;
        if (use_esi && !supplied) {
            printf("SOEM tidak menggunakan profil ESI; dibatalkan.\n");
            return false;
        }
        printf("SOEM config_map: Output=%u Input=%u byte\n", s.Obytes, s.Ibytes);
        if (!validate_pdo_iomap(ctx, iomap, sizeof(iomap), rx.bytes, tx.bytes, mapped_bytes))
            return false;
        if (use_esi) {
            for (int i = 2; i <= 3; ++i) {
                ec_smt actual{};
                const auto &expected_sm = s.SM[i];
                if (ecx_FPRD(&ctx->port, s.configadr, ECT_REG_SM0 + i * sizeof(ec_smt),
                             sizeof(actual), &actual, EC_TIMEOUTRET) <= 0 ||
                    actual.StartAddr != expected_sm.StartAddr || actual.SMlength != expected_sm.SMlength ||
                    (etohl(actual.SMflags) & 0x000100ffu) != (etohl(expected_sm.SMflags) & 0x000100ffu)) {
                    printf("Readback SM%d tidak sesuai XML; dibatalkan.\n", i);
                    return false;
                }
                printf("SM%d readback: alamat=%04X ukuran=%u control=%02X OK\n", i,
                       etohs(actual.StartAddr), etohs(actual.SMlength), etohl(actual.SMflags) & 0xff);
            }
        }

        if (!rx.unchanged(ctx) || !tx.unchanged(ctx)) return false;
        memset(s.outputs, 0, s.Obytes);
        memcpy(s.outputs + rx.target, &position, 4);
        if (rx.mode >= 0) memcpy(s.outputs + rx.mode, &commanded_mode, 1);
        if (rx.max_speed >= 0) memcpy(s.outputs + rx.max_speed, &maximum_speed, 4);
        if (!ecx_configdc(ctx)) { printf("Konfigurasi DC gagal.\n"); return false; }
        ecx_dcsync0(ctx, 1, TRUE, period_ns, 0);
        dc_active = true;
        if (!state(ctx, EC_STATE_SAFE_OP)) return false;
        /* Verifikasi DC register setelah slave masuk SAFE-OP — register baru
         * aktif dan dapat dibaca setelah transisi ke SAFE-OP selesai.        */
        uint8_t activation = 0;
        uint32_t cycle_time = 0;
        if (ecx_FPRD(&ctx->port, s.configadr, ECT_REG_DCSYNCACT, 1, &activation, EC_TIMEOUTRET) <= 0 ||
            ecx_FPRD(&ctx->port, s.configadr, ECT_REG_DCCYCLE0, 4, &cycle_time, EC_TIMEOUTRET) <= 0) {
            printf("Readback DC register gagal (FPRD error).\n");
            return false;
        }
        printf("DC readback: activation=0x%02X cycle=%u ns (expect %lld ns).\n",
               activation, etohl(cycle_time), (long long)period_ns);
        if ((activation & 3) != 3 || etohl(cycle_time) != (uint32_t)period_ns) {
            printf("DC/SYNC0 tidak cocok: activation=0x%02X (expect 0x03), "
                   "cycle=%u (expect %lld).\n",
                   activation, etohl(cycle_time), (long long)period_ns);
            return false;
        }
        printf("DC SYNC0 aktif: %u ns (readback OK).\n", etohl(cycle_time));
        const int expected = ctx->grouplist[0].outputsWKC * 2 + ctx->grouplist[0].inputsWKC;
        if (expected <= 0) return false;
        printf("SAFE-OP tercapai; expected WKC=%d.\n", expected);
        CycleClock clock;
        // SYNC0 starts after SOEM's 100 ms delay; exchange while it settles.
        for (int i = 0; i < 200; ++i)
            if (!clock.wait(ctx->DCtime, running) || !exchange(ctx, expected, false, rx, tx)) return false;
        request_state(ctx, EC_STATE_OPERATIONAL);
        bool operational = false;
        for (int i = 0; i < 2000 && *running; ++i) {
            if (!clock.wait(ctx->DCtime, running) || !exchange(ctx, expected, false, rx, tx)) return false;
            if (reached(ctx, 1, EC_STATE_OPERATIONAL, 0)) {
                operational = true;
                break;
            }
        }
        if (!operational) { printf("Gagal masuk OP.\n"); states(ctx); return false; }
        printf("EtherCAT OP tercapai; control word tetap 0x0000.\n");
        if (motion) {
            // Establish fresh PDO position/healthy WKC before any motor enable.
            for (int i = 0; i < 100; ++i)
                if (!clock.wait(ctx->DCtime, running) || !exchange(ctx, expected, true, rx, tx)) return false;
            return motion->run_cycle(ctx, running);
        }
        const int64_t start_dc = ctx->DCtime;
        for (int i = 1; i <= test_cycles; ++i) {
            if (!clock.wait(ctx->DCtime, running) || !exchange(ctx, expected, true, rx, tx)) return false;
            const uint16_t status_word = get16(s.inputs + tx.status), error_code = get16(s.inputs + tx.error);
            if ((status_word & 0x0008) || error_code) {
                printf("Fault drive: SW=%04X ERR=%04X; tidak melakukan reset otomatis.\n",
                       status_word, error_code);
                return false;
            }
            if (i == 1 || i % 1000 == 0) {
                printf("[%d/%d] WKC=%d/%d SW=%04X ERR=%04X posisi=%d DC=%lld ns\n",
                       i, test_cycles, expected, expected, status_word, error_code,
                       get32(s.inputs + tx.position), static_cast<long long>(ctx->DCtime));
                if (!reached(ctx, 1, EC_STATE_OPERATIONAL, 0)) {
                    printf("Slave kehilangan OP.\n"); return false;
                }
            }
        }
        printf("Timing: terlambat >=1 ms=%d, keterlambatan maksimum=%.1f us.\n",
               clock.late_cycles, clock.max_late_ns / 1000.0);
        if (ctx->DCtime <= start_dc) { printf("DC time tidak bertambah.\n"); return false; }
        printf("LULUS komunikasi: 5000 siklus WKC sesuai, OP bertahan, servo tidak enabled.\n");
        return true;
    }();

    bool stop_ok = true;
    if (motion && motion->started()) {
        stop_ok = motion->stop(ctx);
        ok = ok && stop_ok;
    }
    if (!ok) { printf("Uji belum lulus atau dihentikan.\n"); states(ctx); }
    // Leave OP before disabling SYNC0. No assignment/SDO writes need restoration.
    bool cleaned = true;
    if (ctx->slavecount > 0) {
        if (dc_active) {
            const bool preop = state(ctx, EC_STATE_PRE_OP);
            cleaned = preop;
            ecx_dcsync0(ctx, 1, FALSE, 0, 0);
            if (!cleaned) printf("PERHATIAN: pemulihan state belum terkonfirmasi.\n");
        } else {
            printf("SYNC0 belum diaktifkan.\n");
        }
        if (motion && motion->changed()) {
            if (!dc_active) cleaned = state(ctx, EC_STATE_PRE_OP) && cleaned;
            if (cleaned && stop_ok) cleaned = motion->restore(ctx) && cleaned;
            else { printf("Parameter belum dipulihkan: stop/PRE-OP belum terkonfirmasi.\n"); cleaned = false; }
        }
        printf("Assignment PDO tidak diubah oleh uji ini.\n");
        if (!state(ctx, EC_STATE_INIT)) cleaned = false;
    }
    ecx_close(ctx);
    if (motion) printf("Selesai uji putar; tanpa fault reset/re-enable. Hasil=%s\n", ok && cleaned ? "LULUS" : "BELUM LULUS");
    else printf("Selesai; tidak mengirim enable atau fault reset. Hasil=%s\n",
                ok && cleaned ? "LULUS" : "BELUM LULUS");
    return ok && cleaned ? 0 : 1;
}

int check_pdo(const char *ifname, const volatile sig_atomic_t *running, bool use_esi)
{
    return run_bus(ifname, running, use_esi, nullptr);
}

int test_rotate(const char *ifname, const volatile sig_atomic_t *running)
{
    MotionTrial motion;
    return run_bus(ifname, running, true, &motion);
}
