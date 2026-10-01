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
#include <sched.h>
#include <time.h>
#include "soem/soem.h"
#include "sdo_trace.h"
#include "lc10e_esi.h"
#include "pdo_iomap.h"
#include "motion_trial.h"
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sstream>

namespace {
constexpr int64_t period_ns = 4000000; // 4ms: lebih toleran untuk kernel non-RT.
constexpr int test_cycles = 5000;

void errors(ecx_contextt *ctx)
{
    while (ecx_iserror(ctx)) printf("SOEM: %s", ecx_elist2string(ctx));
}

template<typename T>
bool read_sdo(ecx_contextt *ctx, uint16_t slave, uint16_t index, uint8_t sub, T &value)
{
    int size = sizeof(value);
    begin_sdo_trace(index, sub, sizeof(value));
    const int wkc = ecx_SDOread(ctx, slave, index, sub, FALSE, &size, &value, EC_TIMEOUTRXM);
    if (!finish_sdo_trace(wkc, size)) {
        printf("Slave %d: Gagal baca SDO %04X:%02X (ukuran %d, expected %zu).\n",
               slave, index, sub, size, sizeof(value));
        errors(ctx);
        return false;
    }
    return true;
}

template<typename T>
bool read_sdo(ecx_contextt *ctx, uint16_t index, uint8_t sub, T &value)
{
    return read_sdo(ctx, 1, index, sub, value);
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
        int wkc = ecx_writestate(ctx, i);
        if (wkc <= 0) {
            osal_usleep(10000);
            wkc = ecx_writestate(ctx, i);
        }
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

    bool load(ecx_contextt *ctx, uint16_t slave = 1, bool verbose = true) {
        uint8_t count = 0;
        if (!read_sdo(ctx, slave, assignment, 0, count)) return false;
        if (count != 1) {
            printf("Slave %d: Assignment %04X jumlah=%u; uji mendukung satu PDO per arah.\n", slave, assignment, count);
            return false;
        }
        uint16_t wire_mapping = 0;
        if (!read_sdo(ctx, slave, assignment, 1, wire_mapping)) return false;
        mapping = etohs(wire_mapping);
        const bool is_rx = (assignment == 0x1c12);
        if (mapping < (is_rx ? 0x1600u : 0x1a00u) || mapping > (is_rx ? 0x17ffu : 0x1bffu)) {
            printf("Slave %d: Assignment %04X menunjuk PDO tidak valid: %04X.\n", slave, assignment, mapping);
            return false;
        }
        if (!read_sdo(ctx, slave, mapping, 0, count)) return false;
        if (verbose) printf("Slave %d: Assignment %04X -> PDO %04X: %u entri aktual\n", slave, assignment, mapping, count);
        if (!count || count > 32) {
            printf("Jumlah entri PDO di luar batas uji (1..32).\n");
            return false;
        }

        entries.clear();
        if (from_esi) {
            const uint16_t expected = is_rx ? lc10e_esi::rx_index : lc10e_esi::tx_index;
            const size_t expected_count = is_rx ? std::size(lc10e_esi::rx) : std::size(lc10e_esi::tx);
            if (!lc10e_esi::matches_slave(ctx, slave) || mapping != expected || count != expected_count) {
                printf("Identitas/assignment/jumlah entri tidak cocok dengan ESI; dibatalkan.\n");
                return false;
            }
            if (is_rx) entries.assign(std::begin(lc10e_esi::rx), std::end(lc10e_esi::rx));
            else entries.assign(std::begin(lc10e_esi::tx), std::end(lc10e_esi::tx));
            if (verbose) printf("  Descriptor bersumber dari LC10E V1.04.xml (fixed PDO).\n");
        } else {
            for (uint8_t i = 1; i <= count; ++i) {
                uint32_t wire = 0;
                if (!read_sdo(ctx, slave, mapping, i, wire)) return false;
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

    bool unchanged(ecx_contextt *ctx, uint16_t slave = 1) const {
        Layout current(assignment);
        current.from_esi = from_esi;
        if (!current.load(ctx, slave, false) || current.mapping != mapping || current.entries != entries) {
            printf("Slave %d: Assignment/mapping berubah selama setup; uji dibatalkan.\n", slave);
            return false;
        }
        return true;
    }
};

int64_t ns(const timespec &t) { return int64_t(t.tv_sec) * 1000000000 + t.tv_nsec; }
timespec as_time(int64_t t) { return {time_t(t / 1000000000), long(t % 1000000000)}; }
int64_t now_ns() { timespec t{}; clock_gettime(CLOCK_MONOTONIC, &t); return ns(t); }

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
    for (int i = 1; i <= ctx->slavecount; ++i) {
        auto &s = ctx->slavelist[i];
        // All other outputs retain their verified zero/preserved initialization.
        memset(s.outputs + rx.control, 0, 2);
    }
    ecx_send_processdata(ctx);
    const int wkc = ecx_receive_processdata(ctx, EC_TIMEOUTRET);
    if (check_wkc && wkc != expected) {
        printf("GAGAL: WKC=%d expected=%d; data tidak digunakan.\n", wkc, expected);
        return false;
    }
    if (wkc == expected) {
        for (int i = 1; i <= ctx->slavecount; ++i) {
            auto &s = ctx->slavelist[i];
            const uint16_t sw = get16(s.inputs + tx.status);
            if (sw & 0x0004) {
                printf("GAGAL: Slave %d status servo masih enabled (SW=%04X).\n", i, sw);
                return false;
            }
            // Hold the reported position even though operation stays disabled.
            memcpy(s.outputs + rx.target, s.inputs + tx.position, 4);
        }
    }
    return true;
}
} // namespace

static int run_bus(const char *ifname, const volatile sig_atomic_t *running, bool use_esi, MotionTrial *motion, bool monitor_mode = false, bool control_mode = false, int initial_rpm = 25)
{
    /* Naikkan prioritas scheduling ke SCHED_FIFO agar jitter timer lebih kecil.
     * Memerlukan sudo/root. Gagal tidak fatal — hanya memberikan peringatan. */
    {
        struct sched_param sp{};
        sp.sched_priority = 50;
        if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0)
            printf("PERINGATAN: Gagal set SCHED_FIFO (errno=%d); timing mungkin tidak presisi.\n", errno);
        else
            printf("SCHED_FIFO priority=50 aktif (mengurangi jitter timer).\n");
    }
    if (motion) printf("Mode: uji putar PP 4×360 derajat +524288 unit, poros bebas; enable hanya setelah validasi.\n");
    else if (control_mode) printf("Mode: Kontrol Interaktif 3-Servo PP Mode (Kecepatan Awal: %d RPM).\n", initial_rpm);
    else if (monitor_mode) printf("Mode: Monitor Real-Time Encoder Internal (Tanpa Gerakan/Enable Motor).\n");
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
        printf("Slave count=%d\n", ctx->slavecount);
        for (int i = 1; i <= ctx->slavecount; ++i) {
            auto &s = ctx->slavelist[i];
            printf("Slave %d: VendorID=%08X ProductCode=%08X DC=%d\n",
                   i, s.eep_man, s.eep_id, s.hasdc);
            printf("Identitas EEPROM Slave %d: name=%s revision=%08X\n", i, s.name, s.eep_rev);
        }
        if (ctx->slavecount < 1 || ctx->slavecount > 3) {
            printf("Uji mendukung 1 hingga 3 drive yang telah teridentifikasi (766/402).\n");
            return false;
        }
        for (int i = 1; i <= ctx->slavecount; ++i) {
            auto &s = ctx->slavelist[i];
            if (s.eep_man != 0x766 || s.eep_id != 0x402) {
                printf("Slave %d bukan Lichuan LC10E yang telah teridentifikasi (766/402).\n", i);
                return false;
            }
            if (!s.hasdc) {
                printf("Slave %d tidak menyediakan DC.\n", i);
                return false;
            }
        }
        if (use_esi && !lc10e_esi::matches(ctx)) {
            printf("Identitas/revision berbeda dari XML; uji ESI dibatalkan.\n");
            return false;
        }
        // Scan previously reported INIT: wait explicitly for PRE-OP before SDO.
        if (!state(ctx, EC_STATE_PRE_OP)) return false;

        std::vector<int32_t> initial_positions(ctx->slavecount + 1, 0);
        for (int i = 1; i <= ctx->slavecount; ++i) {
            uint16_t sw = 0, error = 0;
            uint32_t position = 0;
            int8_t mode = 0;
            if (!read_sdo(ctx, i, 0x6041, 0, sw) || !read_sdo(ctx, i, 0x603f, 0, error) ||
                !read_sdo(ctx, i, 0x6064, 0, position) || !read_sdo(ctx, i, 0x6061, 0, mode)) return false;
            initial_positions[i] = int32_t(etohl(position));
            printf("Slave %d Awal: SW=%04X ERR=%04X posisi=%d mode=%d\n",
                   i, etohs(sw), etohs(error), initial_positions[i], mode);
            /* Jika drive fault (dari sesi sebelumnya yang crash), coba fault reset
             * sebelum batal. Hanya untuk mode motion/control (rem sudah dilepas, aman). */
            const bool has_fault = (etohs(sw) & 8) || etohs(error);
            if ((etohs(sw) & 0x0004) || ((motion || control_mode) && has_fault)) {
                if ((motion || control_mode) && has_fault && !(etohs(sw) & 0x0004)) {
                    printf("Slave %d fault terdeteksi (SW=%04X ERR=%04X); mencoba fault reset...\n",
                           i, etohs(sw), etohs(error));
                    /* CiA402 Fault Reset: bit 7 rising edge pada 6040h */
                    uint16_t cw_reset = htoes(0x0080); int sz = 2;
                    ecx_SDOwrite(ctx, i, 0x6040, 0, FALSE, sz, &cw_reset, EC_TIMEOUTRXM);
                    osal_usleep(100000);  /* 100ms: drive proses reset */
                    uint16_t cw_zero = 0; sz = 2;
                    ecx_SDOwrite(ctx, i, 0x6040, 0, FALSE, sz, &cw_zero, EC_TIMEOUTRXM);
                    osal_usleep(100000);
                    /* Baca ulang status setelah reset */
                    if (!read_sdo(ctx, i, 0x6041, 0, sw) || !read_sdo(ctx, i, 0x603f, 0, error)) return false;
                    printf("Slave %d setelah reset: SW=%04X ERR=%04X\n", i, etohs(sw), etohs(error));
                    if (etohs(error)) {
                        printf("Slave %d fault aktif setelah reset (ERR=%04X); matikan dan hidupkan drive.\n", i, etohs(error));
                        return false;
                    }
                    if (etohs(sw) & 8) {
                        /* ERR=0000 tapi fault bit masih set: kemungkinan sisa sesi sebelumnya
                         * yang crash (sync error). SDO reset pada LC10E tidak selalu efektif
                         * di PRE-OP. PDO rising-edge fault reset akan dicoba saat OP aktif. */
                        printf("Slave %d SW=%04X: fault bit sisa (ERR=0000); "
                               "akan direset via PDO saat OP.\n", i, etohs(sw));
                    } else {
                        printf("Slave %d fault berhasil direset via SDO; melanjutkan uji.\n", i);
                    }
                } else {
                    printf("Slave %d servo sudah enabled atau fault permanen; uji dibatalkan.\n", i);
                    return false;
                }
            }
        }

        // Dump both active maps completely before deciding if their fields are safe.
        const bool rx_read = rx.load(ctx, 1), tx_read = tx.load(ctx, 1);
        if (!rx_read || !tx_read || !rx.validate() || !tx.validate() || !*running) return false;
        if (ctx->slavecount > 1) {
            for (int i = 2; i <= ctx->slavecount; ++i) {
                if (!rx.unchanged(ctx, i) || !tx.unchanged(ctx, i)) return false;
            }
        }
        if (motion) printf("Assignment aktif dipertahankan; parameter profil PP sementara akan diverifikasi.\n");
        else if (control_mode) printf("Assignment aktif dipertahankan; menyiapkan PP mode untuk kontrol interaktif.\n");
        else printf("Menggunakan assignment aktif tanpa menulis SDO/mengganti PDO.\n");
        int8_t commanded_mode = 0;
        uint32_t maximum_speed = 0;
        if (rx.mode >= 0 && !read_sdo(ctx, 1, 0x6060, 0, commanded_mode)) return false;
        if (rx.max_speed >= 0 && !read_sdo(ctx, 1, 0x607f, 0, maximum_speed)) return false;

        if (motion) {
            if (!motion->prepare(ctx)) return false;
            commanded_mode = 1;
            maximum_speed = htoel(MotionTrial::speed);
        }
        if (control_mode) {
            printf("Mode Kontrol Interaktif: Menyiapkan PP mode dan rentang kecepatan 0-3000 RPM untuk %d slave...\n",
                   ctx->slavecount);
            uint32_t speed = (initial_rpm > 0) ? (uint32_t)((uint64_t)initial_rpm * 131072 / 60) : 0;
            if (initial_rpm > 0 && speed < 2184) speed = 2184;
            // 8,000,000 pulses/s = ~3662 RPM (mencakup batas penuh 3000 RPM = 6,553,600 pulses/s)
            const uint32_t max_profile_vel = 8000000;
            // Akselerasi 5,000,000 pulses/s^2 (halus, stabil, dan responsif dari 0 hingga 3000 RPM)
            const uint32_t accel = 5000000;
            const uint32_t quick_stop_accel = 10000000;

            auto write_sdo_checked = [&](int slave, uint16_t index, uint8_t sub, int size, uint32_t value) -> bool {
                uint8_t bytes[4]{};
                for (int i = 0; i < size; ++i) bytes[i] = uint8_t(value >> (i * 8));
                int wkc = ecx_SDOwrite(ctx, slave, index, sub, FALSE, size, bytes, EC_TIMEOUTRXM);
                uint8_t rb_bytes[4]{};
                int actual_sz = size;
                int r_wkc = ecx_SDOread(ctx, slave, index, sub, FALSE, &actual_sz, rb_bytes, EC_TIMEOUTRXM);
                uint32_t readback = 0;
                for (int i = 0; i < size; ++i) readback |= uint32_t(rb_bytes[i]) << (i * 8);
                bool ok = (wkc > 0 && r_wkc > 0 && readback == value);
                printf("Slave %d SDO 0x%04X:%02X = %u -> WKC=%d readback=%u [%s]\n",
                       slave, index, sub, value, wkc, readback, ok ? "OK" : "WARN");
                return wkc > 0;
            };

            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                // 1. Modes of Operation: PP (1)
                write_sdo_checked(slv, 0x6060, 0, 1, 1);
                // 2. Profile Velocity (6081h) -> Set to max profile velocity ceiling
                write_sdo_checked(slv, 0x6081, 0, 4, max_profile_vel);
                // 3. Max Profile Velocity (607Fh) -> Set to max profile velocity ceiling
                write_sdo_checked(slv, 0x607F, 0, 4, max_profile_vel);
                // 4. Profile Acceleration (6083h)
                write_sdo_checked(slv, 0x6083, 0, 4, accel);
                // 5. Profile Deceleration (6084h)
                write_sdo_checked(slv, 0x6084, 0, 4, accel);
                // 6. Quick Stop Deceleration (6085h)
                write_sdo_checked(slv, 0x6085, 0, 4, quick_stop_accel);
                // 7. Quick Stop Option Code (605Ah = 2: slow down on quick stop ramp)
                write_sdo_checked(slv, 0x605A, 0, 2, 2);
                // 8. Max Torque (6072h = 1000: 100.0% rated torque limit)
                write_sdo_checked(slv, 0x6072, 0, 2, 1000);
            }
            commanded_mode = 1;
            maximum_speed = htoel(speed);
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
        for (int i = 1; i <= ctx->slavecount; ++i) {
            printf("Slave %d SOEM config_map: Output=%u Input=%u byte\n",
                   i, ctx->slavelist[i].Obytes, ctx->slavelist[i].Ibytes);
        }
        if (!validate_pdo_iomap(ctx, iomap, sizeof(iomap), rx.bytes, tx.bytes, mapped_bytes))
            return false;
        if (use_esi) {
            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                auto &s = ctx->slavelist[slv];
                for (int i = 2; i <= 3; ++i) {
                    ec_smt actual{};
                    const auto &expected_sm = s.SM[i];
                    if (ecx_FPRD(&ctx->port, s.configadr, ECT_REG_SM0 + i * sizeof(ec_smt),
                                 sizeof(actual), &actual, EC_TIMEOUTRET) <= 0 ||
                        actual.StartAddr != expected_sm.StartAddr || actual.SMlength != expected_sm.SMlength ||
                        (etohl(actual.SMflags) & 0x000100ffu) != (etohl(expected_sm.SMflags) & 0x000100ffu)) {
                        printf("Slave %d: Readback SM%d tidak sesuai XML; dibatalkan.\n", slv, i);
                        return false;
                    }
                    printf("Slave %d SM%d readback: alamat=%04X ukuran=%u control=%02X OK\n", slv, i,
                           etohs(actual.StartAddr), etohs(actual.SMlength), etohl(actual.SMflags) & 0xff);
                }
            }
        }

        for (int i = 1; i <= ctx->slavecount; ++i) {
            if (!rx.unchanged(ctx, i) || !tx.unchanged(ctx, i)) return false;
            auto &s = ctx->slavelist[i];
            memset(s.outputs, 0, s.Obytes);
            int32_t pos = initial_positions[i];
            memcpy(s.outputs + rx.target, &pos, 4);
            if (rx.mode >= 0) memcpy(s.outputs + rx.mode, &commanded_mode, 1);
            if (rx.max_speed >= 0) memcpy(s.outputs + rx.max_speed, &maximum_speed, 4);
        }
        if (!ecx_configdc(ctx)) { printf("Konfigurasi DC gagal.\n"); return false; }
        for (int i = 1; i <= ctx->slavecount; ++i) {
            ecx_dcsync0(ctx, i, TRUE, period_ns, 0);
        }
        dc_active = true;
        if (!state(ctx, EC_STATE_SAFE_OP)) return false;

        /* Verifikasi DC register setelah slave masuk SAFE-OP — register baru
         * aktif dan dapat dibaca setelah transisi ke SAFE-OP selesai.        */
        for (int i = 1; i <= ctx->slavecount; ++i) {
            auto &s = ctx->slavelist[i];
            uint8_t activation = 0;
            uint32_t cycle_time = 0;
            if (ecx_FPRD(&ctx->port, s.configadr, ECT_REG_DCSYNCACT, 1, &activation, EC_TIMEOUTRET) <= 0 ||
                ecx_FPRD(&ctx->port, s.configadr, ECT_REG_DCCYCLE0, 4, &cycle_time, EC_TIMEOUTRET) <= 0) {
                printf("Slave %d: Readback DC register gagal (FPRD error).\n", i);
                return false;
            }
            printf("Slave %d DC readback: activation=0x%02X cycle=%u ns (expect %lld ns).\n",
                   i, activation, etohl(cycle_time), (long long)period_ns);
            if ((activation & 3) != 3 || etohl(cycle_time) != (uint32_t)period_ns) {
                printf("Slave %d DC/SYNC0 tidak cocok: activation=0x%02X (expect 0x03), "
                       "cycle=%u (expect %lld).\n",
                       i, activation, etohl(cycle_time), (long long)period_ns);
                return false;
            }
            printf("Slave %d DC SYNC0 aktif: %u ns (readback OK).\n", i, etohl(cycle_time));
        }

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
            bool all_reached = true;
            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                if (!reached(ctx, slv, EC_STATE_OPERATIONAL, 0)) {
                    all_reached = false;
                    break;
                }
            }
            if (all_reached) {
                operational = true;
                break;
            }
        }
        if (!operational) { printf("Gagal masuk OP.\n"); states(ctx); return false; }
        printf("EtherCAT OP tercapai; control word tetap 0x0000.\n");
        if (motion) {
            /* Warm-up: tunggu DC SYNC0 stabil. Kernel non-RT memerlukan beberapa
             * detik sebelum slave benar-benar sinkron. Kirim frame terus menerus
             * (CW=0, target=posisi saat ini) dan hitung frame WKC baik.
             * Cycle 4ms × 500 = 2 detik; cukup toleran tanpa terlalu lama. */
            printf("Warm-up DC SYNC0 (4ms cycle, 500 siklus = 2 detik)...\n");
            int good_frames = 0, last_wkc = 0;
            for (int i = 0; i < 500 && *running; ++i) {
                if (!clock.wait(ctx->DCtime, running)) return false;
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    auto &s = ctx->slavelist[slv];
                    memset(s.outputs + rx.control, 0, 2);
                    memcpy(s.outputs + rx.target, s.inputs + tx.position, 4);
                }
                ecx_send_processdata(ctx);
                last_wkc = ecx_receive_processdata(ctx, EC_TIMEOUTRET);
                if (last_wkc == expected) ++good_frames;
            }
            printf("Warm-up selesai: %d/500 frame WKC baik (WKC terakhir=%d expected=%d).\n",
                   good_frames, last_wkc, expected);
            /* Tolak hanya jika benar-benar tidak ada frame yang berhasil. */
            if (good_frames < 10) {
                printf("GAGAL: Hanya %d frame WKC baik dari 500; link EtherCAT bermasalah.\n", good_frames);
                return false;
            }
            return motion->run_cycle(ctx, running);
        }
        if (control_mode) {
            // 1. Verifikasi mode display 0x6061 (harus 1 untuk Profile Position)
            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                uint8_t mode_disp = 0;
                int sz = 1;
                ecx_SDOread(ctx, slv, 0x6061, 0, FALSE, &sz, &mode_disp, EC_TIMEOUTRXM);
                printf("Slave %d: Modes of Operation Display (0x6061) = %d [%s]\n",
                       slv, mode_disp, mode_disp == 1 ? "PP OK" : "BUKAN PP");
            }

            // 2. Cek apakah ada slave dalam status FAULT (sw & 0x0008). Jika ada, lakukan auto-reset.
            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                uint16_t sw = get16(ctx->slavelist[slv].inputs + tx.status);
                if (sw & 0x0008) {
                    printf("Slave %d terdeteksi Fault (SW=%04X); melakukan auto-reset via PDO...\n", slv, sw);
                    for (int i = 0; i < 75 && *running; ++i) {
                        clock.wait(ctx->DCtime, running);
                        uint16_t cw = htoes(0x0080);
                        memcpy(ctx->slavelist[slv].outputs + rx.control, &cw, 2);
                        ecx_send_processdata(ctx);
                        ecx_receive_processdata(ctx, EC_TIMEOUTRET);
                    }
                    for (int i = 0; i < 25 && *running; ++i) {
                        clock.wait(ctx->DCtime, running);
                        uint16_t cw = 0x0000;
                        memcpy(ctx->slavelist[slv].outputs + rx.control, &cw, 2);
                        ecx_send_processdata(ctx);
                        ecx_receive_processdata(ctx, EC_TIMEOUTRET);
                    }
                }
            }

            // 3. CiA402 Enable Sequence (0x0106 -> 0x0107 -> 0x010F)
            printf("Mengaktifkan CiA402 Enable sequence untuk %d servo...\n", ctx->slavecount);
            const uint16_t commands[] = {0x0106, 0x0107, 0x010F};
            const uint16_t statuses[] = {0x0021, 0x0023, 0x0027};
            for (int step = 0; step < 3; ++step) {
                const int64_t end_t = now_ns() + 3000000000LL;
                bool step_ok = false;
                while (now_ns() < end_t && *running) {
                    if (!clock.wait(ctx->DCtime, running)) return false;
                    for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                        auto &s = ctx->slavelist[slv];
                        uint16_t cw = htoes(commands[step]);
                        memcpy(s.outputs + rx.control, &cw, 2);
                        memcpy(s.outputs + rx.target, s.inputs + tx.position, 4);
                    }
                    ecx_send_processdata(ctx);
                    ecx_receive_processdata(ctx, EC_TIMEOUTRET);
                    bool all_reached = true;
                    for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                        uint16_t sw = get16(ctx->slavelist[slv].inputs + tx.status);
                        if ((sw & 0x006F) != statuses[step]) {
                            all_reached = false;
                            break;
                        }
                    }
                    if (all_reached) { step_ok = true; break; }
                }
                if (!step_ok) {
                    printf("Timeout CiA402 enable step %d\n", step);
                    return false;
                }
            }

            // 4. Masuk ke mode STANDBY: kirim 0x000F (Bit 8 HALT dilepas, Bit 4 New Setpoint = 0)
            //    sehingga servo siap menerima setpoint baru dan Bit 12 (Set-point Ack) nol.
            for (int i = 0; i < 50 && *running; ++i) {
                clock.wait(ctx->DCtime, running);
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    auto &s = ctx->slavelist[slv];
                    uint16_t cw = htoes(0x000F);
                    memcpy(s.outputs + rx.control, &cw, 2);
                    memcpy(s.outputs + rx.target, s.inputs + tx.position, 4);
                }
                ecx_send_processdata(ctx);
                ecx_receive_processdata(ctx, EC_TIMEOUTRET);
            }

            printf("READY_FOR_CONTROL: slaves=%d rpm=%d\n", ctx->slavecount, initial_rpm);
            fflush(stdout);

            int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

            std::string cmd_buffer;
            int current_rpm = initial_rpm;
            uint32_t current_speed = (current_rpm > 0) ? (uint32_t)((uint64_t)current_rpm * 131072 / 60) : 0;
            if (current_rpm > 0 && current_speed < 2184) current_speed = 2184;

            enum class MotionState {
                IDLE,               // Holding position, CW = 0x000F
                WAIT_CLEAR_ACK,     // Menunggu SW bit 12 nol sebelum menaikkan bit 4
                SEND_NEW_SETPOINT,  // Mengirim CW = 0x003F sampai SW bit 12 == 1
                MOVING,             // CW = 0x002F, motor berputar halus terus menerus
                HALTING             // CW = 0x010F, deselerasi halus sampai berhenti
            };

            struct SlaveControlState {
                MotionState state = MotionState::IDLE;
                int32_t target_pos = 0;
                uint16_t control_word = 0x000F;
                int jog_dir = 0; // +1, -1, atau 0 (untuk langkah/step)
                int64_t state_start_ns = 0;
            };

            std::vector<SlaveControlState> states(ctx->slavecount + 1);
            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                states[slv].target_pos = get32(ctx->slavelist[slv].inputs + tx.position);
                states[slv].control_word = 0x000F;
                states[slv].state = MotionState::IDLE;
                states[slv].jog_dir = 0;
                states[slv].state_start_ns = now_ns();
            }

            int64_t next_stream = now_ns();
            int wkc_fail_count = 0;

            while (*running) {
                if (!clock.wait(ctx->DCtime, running)) break;
                const int64_t now = now_ns();

                // 1. Baca perintah stdin secara non-blocking
                char in_buf[512];
                ssize_t n_read = read(STDIN_FILENO, in_buf, sizeof(in_buf) - 1);
                if (n_read > 0) {
                    in_buf[n_read] = '\0';
                    cmd_buffer += in_buf;
                    size_t newline_pos;
                    while ((newline_pos = cmd_buffer.find('\n')) != std::string::npos) {
                        std::string line = cmd_buffer.substr(0, newline_pos);
                        cmd_buffer.erase(0, newline_pos + 1);
                        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
                        if (line.empty()) continue;

                        char verb[32]{};
                        int target_slv = 0;
                        if (sscanf(line.c_str(), "%31s", verb) == 1) {
                            if (strcmp(verb, "JOG") == 0) {
                                char dir_ch = '+';
                                int req_rpm = current_rpm;
                                if (sscanf(line.c_str(), "JOG %d %c %d", &target_slv, &dir_ch, &req_rpm) >= 2) {
                                    int slv_start = (target_slv == 0) ? 1 : target_slv;
                                    int slv_end   = (target_slv == 0) ? ctx->slavecount : target_slv;
                                    int dir = (dir_ch == '-') ? -1 : 1;
                                    if (req_rpm >= 0 && req_rpm != current_rpm) {
                                        current_rpm = std::min(req_rpm, 3000);
                                        current_speed = (current_rpm > 0) ? (uint32_t)((uint64_t)current_rpm * 131072 / 60) : 0;
                                        if (current_rpm > 0 && current_speed < 2184) current_speed = 2184;
                                        printf("[CMD] SET_RPM=%d (speed=%u)\n", current_rpm, current_speed);
                                    }
                                    if (current_rpm == 0) {
                                        printf("[CMD] JOG diabaikan karena RPM = 0 (motor diam)\n");
                                        fflush(stdout);
                                        continue;
                                    }
                                    int32_t jog_chunk = std::max(int32_t(current_speed * 3), 1310720);
                                    for (int s_idx = slv_start; s_idx <= slv_end; ++s_idx) {
                                        if (s_idx >= 1 && s_idx <= ctx->slavecount) {
                                            int32_t cur = get32(ctx->slavelist[s_idx].inputs + tx.position);
                                            uint16_t sw = get16(ctx->slavelist[s_idx].inputs + tx.status);
                                            states[s_idx].jog_dir = dir;
                                            states[s_idx].target_pos = cur + (dir * jog_chunk);
                                            states[s_idx].state_start_ns = now;
                                            if (sw & 0x1000) {
                                                // Bit 12 masih 1, lepas bit 4 dulu
                                                states[s_idx].control_word = 0x000F;
                                                states[s_idx].state = MotionState::WAIT_CLEAR_ACK;
                                            } else {
                                                // Bit 12 sudah 0, picu setpoint baru
                                                states[s_idx].control_word = 0x003F;
                                                states[s_idx].state = MotionState::SEND_NEW_SETPOINT;
                                            }
                                            printf("[CMD] JOG slave=%d dir=%c rpm=%d target=%d\n",
                                                   s_idx, dir_ch, current_rpm, states[s_idx].target_pos);
                                        }
                                    }
                                    fflush(stdout);
                                }
                            } else if (strcmp(verb, "HALT") == 0 || strcmp(verb, "STOP") == 0) {
                                if (strcmp(verb, "STOP") == 0) target_slv = 0;
                                else sscanf(line.c_str(), "HALT %d", &target_slv);
                                int slv_start = (target_slv == 0) ? 1 : target_slv;
                                int slv_end   = (target_slv == 0) ? ctx->slavecount : target_slv;
                                for (int s_idx = slv_start; s_idx <= slv_end; ++s_idx) {
                                    if (s_idx >= 1 && s_idx <= ctx->slavecount) {
                                        states[s_idx].control_word = 0x010F; // Bit 8 HALT = 1
                                        states[s_idx].state = MotionState::HALTING;
                                        states[s_idx].jog_dir = 0;
                                        states[s_idx].state_start_ns = now;
                                        printf("[CMD] HALT slave=%d\n", s_idx);
                                    }
                                }
                                fflush(stdout);
                            } else if (strcmp(verb, "STEP") == 0) {
                                int32_t delta = 0;
                                int req_rpm = current_rpm;
                                if (sscanf(line.c_str(), "STEP %d %d %d", &target_slv, &delta, &req_rpm) >= 2) {
                                    int slv_start = (target_slv == 0) ? 1 : target_slv;
                                    int slv_end   = (target_slv == 0) ? ctx->slavecount : target_slv;
                                    if (req_rpm >= 0 && req_rpm != current_rpm) {
                                        current_rpm = std::min(req_rpm, 3000);
                                        current_speed = (current_rpm > 0) ? (uint32_t)((uint64_t)current_rpm * 131072 / 60) : 0;
                                        if (current_rpm > 0 && current_speed < 2184) current_speed = 2184;
                                        printf("[CMD] SET_RPM=%d (speed=%u)\n", current_rpm, current_speed);
                                    }
                                    if (current_rpm == 0) {
                                        printf("[CMD] STEP diabaikan karena RPM = 0 (motor diam)\n");
                                        fflush(stdout);
                                        continue;
                                    }
                                    for (int s_idx = slv_start; s_idx <= slv_end; ++s_idx) {
                                        if (s_idx >= 1 && s_idx <= ctx->slavecount) {
                                            int32_t cur = get32(ctx->slavelist[s_idx].inputs + tx.position);
                                            uint16_t sw = get16(ctx->slavelist[s_idx].inputs + tx.status);
                                            states[s_idx].jog_dir = 0;
                                            states[s_idx].target_pos = cur + delta;
                                            states[s_idx].state_start_ns = now;
                                            if (sw & 0x1000) {
                                                states[s_idx].control_word = 0x000F;
                                                states[s_idx].state = MotionState::WAIT_CLEAR_ACK;
                                            } else {
                                                states[s_idx].control_word = 0x003F;
                                                states[s_idx].state = MotionState::SEND_NEW_SETPOINT;
                                            }
                                            printf("[CMD] STEP slave=%d delta=%d target=%d\n",
                                                   s_idx, delta, states[s_idx].target_pos);
                                        }
                                    }
                                    fflush(stdout);
                                }
                            } else if (strcmp(verb, "SET_RPM") == 0) {
                                int req_rpm = 25;
                                if (sscanf(line.c_str(), "SET_RPM %d", &req_rpm) == 1 && req_rpm >= 0) {
                                    current_rpm = std::min(req_rpm, 3000);
                                    current_speed = (current_rpm > 0) ? (uint32_t)((uint64_t)current_rpm * 131072 / 60) : 0;
                                    if (current_rpm > 0 && current_speed < 2184) current_speed = 2184;
                                    printf("[CMD] SET_RPM=%d (speed=%u)\n", current_rpm, current_speed);
                                    fflush(stdout);
                                }
                            } else if (strcmp(verb, "FAULT_RESET") == 0) {
                                printf("[CMD] Fault reset pada semua slave...\n");
                                for (int s_idx = 1; s_idx <= ctx->slavecount; ++s_idx) {
                                    states[s_idx].control_word = 0x0080;
                                    states[s_idx].state = MotionState::IDLE;
                                    states[s_idx].jog_dir = 0;
                                    states[s_idx].state_start_ns = now;
                                }
                                fflush(stdout);
                            } else if (strcmp(verb, "QUIT") == 0) {
                                printf("[CMD] QUIT diterima.\n");
                                fflush(stdout);
                                break;
                            }
                        }
                    }
                }

                // 2. Pembaruan State Machine CiA402 per slave
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    uint16_t sw = get16(ctx->slavelist[slv].inputs + tx.status);
                    int32_t act_pos = get32(ctx->slavelist[slv].inputs + tx.position);

                    // Deteksi fault pada driver
                    if (sw & 0x0008) {
                        if (states[slv].state != MotionState::IDLE) {
                            printf("[FAULT] Slave %d FAULT! SW=%04X\n", slv, sw);
                            fflush(stdout);
                            states[slv].state = MotionState::IDLE;
                            states[slv].control_word = 0x0000;
                            states[slv].jog_dir = 0;
                        }
                        continue;
                    }

                    // Selesaikan pulse fault reset jika sudah lewat 200 ms
                    if (states[slv].control_word == 0x0080) {
                        if (now - states[slv].state_start_ns > 200000000LL) {
                            states[slv].control_word = 0x000F;
                            states[slv].target_pos = act_pos;
                            states[slv].state = MotionState::IDLE;
                        }
                        continue;
                    }

                    switch (states[slv].state) {
                    case MotionState::IDLE:
                        states[slv].control_word = 0x000F;
                        states[slv].target_pos = act_pos;
                        break;

                    case MotionState::WAIT_CLEAR_ACK:
                        states[slv].control_word = 0x000F;
                        if ((sw & 0x1000) == 0) {
                            // Ack telah nol! Picu setpoint sekarang
                            states[slv].control_word = 0x003F;
                            states[slv].state = MotionState::SEND_NEW_SETPOINT;
                            states[slv].state_start_ns = now;
                        } else if (now - states[slv].state_start_ns > 500000000LL) {
                            printf("[WARN] Slave %d timeout tunggu ack nol (SW=%04X)\n", slv, sw);
                            fflush(stdout);
                            states[slv].state = MotionState::IDLE;
                            states[slv].jog_dir = 0;
                        }
                        break;

                    case MotionState::SEND_NEW_SETPOINT:
                        states[slv].control_word = 0x003F;
                        if (sw & 0x1000) {
                            // Driver telah konfirmasi setpoint! Ubah ke 0x002F (Bit 4=0, Bit 5=1)
                            states[slv].control_word = 0x002F;
                            states[slv].state = MotionState::MOVING;
                            states[slv].state_start_ns = now;
                        } else if (now - states[slv].state_start_ns > 1000000000LL) {
                            printf("[WARN] Slave %d timeout tunggu konfirmasi setpoint (SW=%04X)\n", slv, sw);
                            fflush(stdout);
                            states[slv].control_word = 0x000F;
                            states[slv].state = MotionState::IDLE;
                            states[slv].jog_dir = 0;
                        }
                        break;

                    case MotionState::MOVING:
                        states[slv].control_word = 0x002F;
                        if (states[slv].jog_dir != 0) {
                            // Hold-to-Jog berkelanjutan: perpanjang target saat mendekati target
                            int64_t remaining = (int64_t)(states[slv].target_pos - act_pos) * states[slv].jog_dir;
                            int32_t jog_chunk = std::max(int32_t(current_speed * 3), 1310720);
                            if (remaining < (jog_chunk / 2)) {
                                states[slv].target_pos += states[slv].jog_dir * jog_chunk;
                                states[slv].control_word = 0x003F;
                                states[slv].state = MotionState::SEND_NEW_SETPOINT;
                                states[slv].state_start_ns = now;
                            }
                        } else {
                            // Step move diskrit
                            int32_t diff = std::abs(act_pos - states[slv].target_pos);
                            if ((sw & 0x0400) || diff < 256) {
                                states[slv].control_word = 0x000F;
                                states[slv].state = MotionState::IDLE;
                            }
                        }
                        break;

                    case MotionState::HALTING:
                        states[slv].control_word = 0x010F; // HALT bit = 1
                        if (now - states[slv].state_start_ns > 150000000LL) {
                            states[slv].target_pos = act_pos;
                            states[slv].control_word = 0x000F;
                            states[slv].state = MotionState::IDLE;
                            states[slv].jog_dir = 0;
                        }
                        break;
                    }
                }

                // 3. Tulis output PDO
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    auto &s = ctx->slavelist[slv];
                    uint16_t cw = htoes(states[slv].control_word);
                    int32_t tp = htoel(states[slv].target_pos);
                    memcpy(s.outputs + rx.control, &cw, 2);
                    memcpy(s.outputs + rx.target, &tp, 4);
                    if (rx.max_speed >= 0) {
                        uint32_t spd_le = htoel(current_speed);
                        memcpy(s.outputs + rx.max_speed, &spd_le, 4);
                    }
                }

                // 4. Kirim dan Terima data PDO EtherCAT
                ecx_send_processdata(ctx);
                const int wkc = ecx_receive_processdata(ctx, EC_TIMEOUTRET);
                if (wkc != expected) {
                    wkc_fail_count++;
                    if (wkc_fail_count > 12) {
                        printf("GAGAL: WKC=%d expected=%d selama 12 siklus berturut-turut.\n", wkc, expected);
                        break;
                    }
                } else {
                    wkc_fail_count = 0;
                }

                // 5. Siarkan telemetri 20 Hz (50 ms)
                if (now >= next_stream) {
                    next_stream = now + 50000000LL;
                    int32_t p1 = get32(ctx->slavelist[1].inputs + tx.position);
                    int32_t p2 = ctx->slavecount > 1 ? get32(ctx->slavelist[2].inputs + tx.position) : 0;
                    int32_t p3 = ctx->slavecount > 2 ? get32(ctx->slavelist[3].inputs + tx.position) : 0;
                    uint16_t s1 = get16(ctx->slavelist[1].inputs + tx.status);
                    uint16_t s2 = ctx->slavecount > 1 ? get16(ctx->slavelist[2].inputs + tx.status) : 0;
                    uint16_t s3 = ctx->slavecount > 2 ? get16(ctx->slavelist[3].inputs + tx.status) : 0;
                    int16_t t1 = get16(ctx->slavelist[1].inputs + 6);
                    int16_t t2 = ctx->slavecount > 1 ? get16(ctx->slavelist[2].inputs + 6) : 0;
                    int16_t t3 = ctx->slavecount > 2 ? get16(ctx->slavelist[3].inputs + 6) : 0;
                    printf("TELEMETRY: pos1=%d sw1=%04X torq1=%d pos2=%d sw2=%04X torq2=%d pos3=%d sw3=%04X torq3=%d wkc=%d\n",
                           p1, s1, t1, p2, s2, t2, p3, s3, t3, expected);
                    fflush(stdout);
                }
            }

            fcntl(STDIN_FILENO, F_SETFL, stdin_flags);

            // Shutdown: Quick stop then disable
            printf("Menghentikan servo...\n");
            for (int step = 0; step < 80; ++step) {
                clock.wait(ctx->DCtime, running);
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    uint16_t cw = htoes(0x0002);
                    memcpy(ctx->slavelist[slv].outputs + rx.control, &cw, 2);
                }
                ecx_send_processdata(ctx);
                ecx_receive_processdata(ctx, EC_TIMEOUTRET);
            }
            for (int step = 0; step < 40; ++step) {
                clock.wait(ctx->DCtime, running);
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    uint16_t cw = 0;
                    memcpy(ctx->slavelist[slv].outputs + rx.control, &cw, 2);
                }
                ecx_send_processdata(ctx);
                ecx_receive_processdata(ctx, EC_TIMEOUTRET);
            }
            return true;
        }
        if (monitor_mode) {
            printf("Mode Monitor Aktif: Menyiarkan posisi encoder 1 & 2 secara real-time (20 Hz)...\n");
            int64_t next_stream = now_ns();
            int wkc_fail_count = 0;
            while (*running) {
                if (!clock.wait(ctx->DCtime, running)) return false;
                for (int i = 1; i <= ctx->slavecount; ++i) {
                    auto &s = ctx->slavelist[i];
                    memset(s.outputs + rx.control, 0, 2);
                    memcpy(s.outputs + rx.target, s.inputs + tx.position, 4);
                }
                ecx_send_processdata(ctx);
                const int wkc = ecx_receive_processdata(ctx, EC_TIMEOUTRET);
                if (wkc != expected) {
                    wkc_fail_count++;
                    if (wkc_fail_count > 10) {
                        printf("GAGAL: WKC=%d expected=%d selama %d siklus berturut-turut; monitor berhenti.\n",
                               wkc, expected, wkc_fail_count);
                        return false;
                    }
                } else {
                    wkc_fail_count = 0;
                }
                for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                    auto &s = ctx->slavelist[slv];
                    const uint16_t status_word = get16(s.inputs + tx.status), error_code = get16(s.inputs + tx.error);
                    if ((status_word & 0x0008) || error_code) {
                        printf("Fault drive Slave %d: SW=%04X ERR=%04X\n", slv, status_word, error_code);
                        return false;
                    }
                }
                if (now_ns() >= next_stream) {
                    next_stream = now_ns() + 50000000LL; // 20 Hz (50 ms)
                    int32_t p1 = get32(ctx->slavelist[1].inputs + tx.position);
                    int32_t p2 = ctx->slavecount > 1 ? get32(ctx->slavelist[2].inputs + tx.position) : 0;
                    int32_t p3 = ctx->slavecount > 2 ? get32(ctx->slavelist[3].inputs + tx.position) : 0;
                    uint16_t s1 = get16(ctx->slavelist[1].inputs + tx.status);
                    uint16_t s2 = ctx->slavecount > 1 ? get16(ctx->slavelist[2].inputs + tx.status) : 0;
                    uint16_t s3 = ctx->slavecount > 2 ? get16(ctx->slavelist[3].inputs + tx.status) : 0;
                    int16_t t1 = get16(ctx->slavelist[1].inputs + 6);
                    int16_t t2 = ctx->slavecount > 1 ? get16(ctx->slavelist[2].inputs + 6) : 0;
                    int16_t t3 = ctx->slavecount > 2 ? get16(ctx->slavelist[3].inputs + 6) : 0;
                    printf("TELEMETRY: pos1=%d sw1=%04X torq1=%d pos2=%d sw2=%04X torq2=%d pos3=%d sw3=%04X torq3=%d wkc=%d\n",
                           p1, s1, t1, p2, s2, t2, p3, s3, t3, expected);
                    fflush(stdout);
                }
            }
            return true;
        }
        const int64_t start_dc = ctx->DCtime;
        for (int i = 1; i <= test_cycles; ++i) {
            if (!clock.wait(ctx->DCtime, running) || !exchange(ctx, expected, true, rx, tx)) return false;
            for (int slv = 1; slv <= ctx->slavecount; ++slv) {
                auto &s = ctx->slavelist[slv];
                const uint16_t status_word = get16(s.inputs + tx.status), error_code = get16(s.inputs + tx.error);
                if ((status_word & 0x0008) || error_code) {
                    printf("Fault drive Slave %d: SW=%04X ERR=%04X; tidak melakukan reset otomatis.\n",
                           slv, status_word, error_code);
                    return false;
                }
                if (i == 1 || i % 1000 == 0) {
                    printf("[%d/%d] Slave %d: WKC=%d/%d SW=%04X ERR=%04X posisi=%d DC=%lld ns\n",
                           i, test_cycles, slv, expected, expected, status_word, error_code,
                           get32(s.inputs + tx.position), static_cast<long long>(ctx->DCtime));
                    if (!reached(ctx, slv, EC_STATE_OPERATIONAL, 0)) {
                        printf("Slave %d kehilangan OP.\n", slv); return false;
                    }
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
            for (int slv = 1; slv <= ctx->slavecount; ++slv)
                ecx_dcsync0(ctx, slv, FALSE, 0, 0);
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
    else if (monitor_mode) printf("Selesai monitoring; servo tetap aman/disabled. Hasil=%s\n", ok && cleaned ? "LULUS" : "BELUM LULUS");
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

int monitor_encoders(const char *ifname, const volatile sig_atomic_t *running)
{
    return run_bus(ifname, running, true, nullptr, true);
}

int dance_trial(const char *ifname, const volatile sig_atomic_t *running)
{
    MotionTrial motion;
    motion.set_dance_mode();
    return run_bus(ifname, running, true, &motion);
}

int control_interactive(const char *ifname, const volatile sig_atomic_t *running, int initial_rpm)
{
    return run_bus(ifname, running, true, nullptr, false, true, initial_rpm);
}

