#include "motion_trial.h"
#include "sdo_trace.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <time.h>

namespace {
int64_t now_ns() {
    timespec t{}; clock_gettime(CLOCK_MONOTONIC, &t);
    return int64_t(t.tv_sec) * 1000000000 + t.tv_nsec;
}
uint16 sw(ecx_contextt *c) { uint16 v; memcpy(&v, c->slavelist[1].inputs + 2, 2); return etohs(v); }
int32 position(ecx_contextt *c) { uint32 v; memcpy(&v, c->slavelist[1].inputs + 4, 4); return int32(etohl(v)); }
void target(ecx_contextt *c, int32 p) { uint32 v = htoel(uint32(p)); memcpy(c->slavelist[1].outputs + 2, &v, 4); }
bool upload(ecx_contextt *c, uint16 index, uint8 sub, int size, uint32 &value) {
    uint8 bytes[4]{}; int actual = size;
    begin_sdo_trace(index, sub, size);
    int wkc = ecx_SDOread(c, 1, index, sub, FALSE, &actual, bytes, EC_TIMEOUTRXM);
    if (!finish_sdo_trace(wkc, actual)) return false;
    value = 0;
    for (int i = 0; i < size; ++i) value |= uint32(bytes[i]) << (i * 8);
    return true;
}
bool download(ecx_contextt *c, uint16 index, int size, uint32 value) {
    uint8 bytes[4]{};
    for (int i = 0; i < size; ++i) bytes[i] = uint8(value >> (i * 8));
    const int wkc = ecx_SDOwrite(c, 1, index, 0, FALSE, size, bytes, EC_TIMEOUTRXM);
    uint32 readback = 0;
    const bool ok = wkc > 0 && upload(c, index, 0, size, readback) && readback == value;
    printf("Parameter %04X: nilai=%u WKC=%d readback=%s\n", index, value, wkc, ok ? "OK" : "GAGAL");
    return ok;
}
}

bool MotionTrial::changed() const {
    for (const auto &p : saved) if (p.touched) return true;
    return false;
}
bool MotionTrial::set_parameter(ecx_contextt *c, uint16 index, uint32 value) {
    for (auto &p : saved) if (p.index == index) {
        // Even a missing write acknowledgement can mean a device was changed.
        p.touched = true;
        return download(c, index, p.size, value);
    }
    return false;
}
bool MotionTrial::prepare(ecx_contextt *c) {
    uint32 numerator, denominator, offset, polarity, lo, hi;
    if (!upload(c, 0x6091, 1, 4, numerator) || !upload(c, 0x6091, 2, 4, denominator) ||
        !upload(c, 0x60b0, 0, 4, offset) || !upload(c, 0x607e, 0, 1, polarity) ||
        !upload(c, 0x607d, 1, 4, lo) || !upload(c, 0x607d, 2, 4, hi)) return false;
    printf("Uji poros bebas: gear=%u/%u offset=%d polarity=%02X batas=%d..%d\n",
           numerator, denominator, int32(offset), polarity, int32(lo), int32(hi));
    if (!numerator || numerator != denominator || offset || (polarity & 0x80) || int32(lo) >= int32(hi)) {
        printf("Uji memerlukan gear 1:1, offset nol, polaritas posisi normal, dan batas posisi valid.\n");
        return false;
    }
    minimum = int32(lo); maximum = int32(hi);
    saved = {{0x6060, 1, 0}, {0x607f, 4, 0}, {0x6081, 4, 0}, {0x6083, 4, 0},
             {0x6084, 4, 0}, {0x6085, 4, 0}, {0x605a, 2, 0}, {0x6072, 2, 0}};
    for (auto &p : saved) if (!upload(c, p.index, 0, p.size, p.value)) return false;
    if (saved[1].value < speed || !saved[7].value) {
        printf("Batas kecepatan/torsi awal tidak mendukung profil uji; tidak dinaikkan otomatis.\n");
        return false;
    }
    // Only volatile profile/stop settings; never write gear ratio, assignments,
    // polarity, limits, EEPROM or save-to-flash objects.
    return set_parameter(c, 0x6060, 1) && set_parameter(c, 0x607f, speed) &&
        set_parameter(c, 0x6081, speed) && set_parameter(c, 0x6083, acceleration) &&
        set_parameter(c, 0x6084, acceleration) && set_parameter(c, 0x6085, acceleration) &&
        set_parameter(c, 0x605a, 2) && set_parameter(c, 0x6072, std::min(saved[7].value, uint32(200)));
}

bool MotionTrial::tick(const volatile sig_atomic_t *running, bool stopping) {
    if (!stopping && !*running) return false;
    if (!next_tick) next_tick = now_ns();
    next_tick += 1000000;
    timespec deadline{time_t(next_tick / 1000000000), long(next_tick % 1000000000)};
    int rc;
    do { rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr); }
    while (rc == EINTR && (stopping || *running));
    if (rc || (!stopping && !*running)) return false;
    const int64_t late = now_ns() - next_tick;
    /* Toleransi jitter NIC non-realtime (Realtek dll.) dan kernel non-RT:
     * Realtek 8611 + Debian desktop dapat spike latency 30-50ms.
     * Pada kecepatan 8192 unit/s, 50ms ≈ 410 unit drift — masih aman. */
    if (late > 50000000 && !stopping) {
        printf("Jeda kontrol melebihi 50 ms (%lld ms); uji dihentikan.\n",
               static_cast<long long>(late / 1000000)); return false;
    }
    if (late >= 1000000) next_tick = now_ns();
    return true;
}
bool MotionTrial::frame(ecx_contextt *c, uint16 control, bool check_fault) {
    auto &s = c->slavelist[1];
    const uint16 wire = htoes(control); memcpy(s.outputs, &wire, 2);
    ecx_send_processdata(c);
    const int wkc = ecx_receive_processdata(c, EC_TIMEOUTRET);
    const int expected = c->grouplist[0].outputsWKC * 2 + c->grouplist[0].inputsWKC;
    if (wkc != expected || expected != 3) {
        /* Toleransi jitter NIC non-realtime (misalnya Realtek): hitung kegagalan
         * berturut-turut; batalkan hanya setelah melewati ambang batas. */
        wkc_fail_count_++;
        if (check_fault && wkc_fail_count_ > WKC_FAIL_LIMIT) {
            printf("Uji gerak: WKC=%d expected=3 selama %d siklus berturut-turut; berhenti.\n",
                   wkc, wkc_fail_count_);
            return false;
        }
        /* Frame buruk tapi masih di bawah batas; terus dengan data PDO lama. */
        return check_fault ? true : false;
    }
    wkc_fail_count_ = 0;  /* reset hitungan saat WKC kembali baik */
    uint16 error; memcpy(&error, s.inputs, 2); error = etohs(error);
    if (check_fault && (error || (sw(c) & (0x0008 | 0x0800 | 0x2000)))) {
        printf("Uji gerak: fault/limit SW=%04X ERR=%04X; tanpa fault reset.\n", sw(c), error);
        return false;
    }
    return true;
}

bool MotionTrial::run(ecx_contextt *c, const volatile sig_atomic_t *running, int32 dist) {
    started_ = true;
    uint32 mode = 0;
    if (!upload(c, 0x6061, 0, 1, mode) || mode != 1) {
        printf("Mode display 6061 belum PP (1); enable dibatalkan.\n"); return false;
    }
    next_tick = 0;
    wkc_fail_count_ = 0;
    if (!tick(running) || !frame(c, 0)) return false;
    const int32 origin = position(c);
    const int64_t goal64 = int64_t(origin) + dist;
    if (origin < minimum || goal64 > maximum || goal64 > INT32_MAX ||
        goal64 < minimum || goal64 < INT32_MIN || (sw(c) & 4)) {
        printf("Target di luar batas atau servo sudah enabled; dibatalkan.\n"); return false;
    }
    const int32 goal = int32(goal64);
    target(c, origin); // Establish current-position target BEFORE any enable.
    printf("Profil PP: awal=%d target=%d delta=%+d, speed=%u unit/s, accel/decel=%u unit/s2.\n",
           origin, goal, dist, speed, acceleration);
    const auto sample = [&](uint16 cw, bool enabled, bool travelling) {
        if (!tick(running) || !frame(c, cw)) return false;
        /* Periksa posisi hanya saat WKC valid (wkc_fail_count_==0) untuk
         * menghindari keputusan salah dari data stale akibat jitter NIC. */
        if (wkc_fail_count_ == 0) {
            const int64_t pos = position(c);
            /* Batas gerak dihitung dari min/max origin & goal agar benar
             * untuk arah maju (distance > 0) maupun mundur (distance < 0). */
            const int64_t lo = std::min(int64_t(origin), goal64);
            const int64_t hi = std::max(int64_t(origin), goal64);
            if (pos < lo - 512 || pos > hi + 512) {
                printf("Posisi keluar rentang uji: %lld.\n", static_cast<long long>(pos)); return false;
            }
            if (enabled && (sw(c) & 0x006f) != 0x0027) {
                printf("Servo kehilangan enable; tidak melakukan re-enable.\n"); return false;
            }
        }
        /* Tidak memanggil ecx_statecheck() per siklus — pada NIC non-realtime
         * (Realtek dll.) fungsi ini mengirim AL request dan reply-nya sering
         * terlambat, sehingga salah dilaporkan sebagai "keluar OP".
         * Kegagalan WKC berturut-turut sudah ditangani di frame(). */
        return true;
    };
    // HALT remains set during enable. Only the single new-setpoint releases it.
    const uint16 commands[] = {0x0106, 0x0107, 0x010f};
    const uint16 statuses[] = {0x0021, 0x0023, 0x0027};
    for (int step = 0; step < 3; ++step) {
        const int64_t end = now_ns() + 2000000000;
        bool reached = false;
        while (now_ns() < end) {
            if (!sample(commands[step], false, false)) return false;
            if ((sw(c) & 0x006f) == statuses[step]) { reached = true; break; }
        }
        if (!reached) { printf("Timeout CiA402 step %d SW=%04X.\n", step, sw(c)); return false; }
    }
    printf("Servo enabled dalam PP; memulai satu gerakan.\n");
    if (sw(c) & 0x1000) { printf("Setpoint acknowledge belum nol; dibatalkan.\n"); return false; }
    target(c, goal);
    bool acknowledged = false, ack_cleared = false;
    const int64_t start = now_ns();
    int64_t settled_since = 0, next_report = start;
    /* Timeout total gerakan: distance/speed + ramp + toleransi.
     * Dengan speed 32768 unit/s dan encoder 131072 ppr, 32768 unit ≈ 0.25 rev
     * pada kecepatan efektif ~2100 unit/s nyata → ~15 s; pakai 30 s untuk aman. */
    while (now_ns() - start < 30000000000LL) {
        if (!sample(acknowledged ? 0x002f : 0x003f, true, true)) return false;
        const auto status = sw(c);
        if (status & 0x1000) acknowledged = true;
        else if (acknowledged) ack_cleared = true;
        if (!acknowledged && now_ns() - start > 2000000000) {
            printf("Timeout acknowledge setpoint; berhenti.\n"); return false;
        }
        if (acknowledged && !ack_cleared && now_ns() - start > 3000000000LL) {
            printf("Acknowledge tidak kembali nol; berhenti.\n"); return false;
        }
        const int64_t error = int64_t(position(c)) - goal;
        if (ack_cleared && (status & 0x0400) && error >= -64 && error <= 64) {
            if (!settled_since) settled_since = now_ns();
            if (now_ns() - settled_since >= 200000000) {
                printf("Target tercapai: posisi=%d delta_aktual=%lld.\n", position(c),
                       static_cast<long long>(int64_t(position(c)) - origin));
                return true;
            }
        } else settled_since = 0;
        if (now_ns() >= next_report) {
            printf("PP: posisi=%d target=%d SW=%04X WKC=3/3\n", position(c), goal, status);
            next_report = now_ns() + 500000000;
        }
    }
    printf("Timeout target 30 detik; berhenti.\n"); return false;
}

bool MotionTrial::stop(ecx_contextt *c) {
    // Do not use the cancelled run flag: transmit a bounded stop even on Ctrl+C.
    printf("Mengirim quick stop lalu disable; tidak melakukan re-enable.\n");
    volatile sig_atomic_t cleanup_running = 1;
    next_tick = 0;
    int64_t start = now_ns(), stable_since = 0;
    int32 anchor = position(c);
    bool stopped = false;
    while (now_ns() - start < 1500000000LL) {
        if (!tick(&cleanup_running, true)) break;
        if (!frame(c, 0x0002, false)) { stable_since = 0; continue; }
        const int64_t delta = int64_t(position(c)) - anchor;
        if (delta < -2 || delta > 2) { anchor = position(c); stable_since = 0; }
        else if (!stable_since) stable_since = now_ns();
        else if (now_ns() - stable_since >= 100000000) { stopped = true; break; }
    }
    bool disabled = false;
    start = now_ns(); stable_since = 0; anchor = position(c);
    while (now_ns() - start < 500000000) {
        if (!tick(&cleanup_running, true)) break;
        if (!frame(c, 0, false) || (sw(c) & 4)) { stable_since = 0; continue; }
        const int64_t delta = int64_t(position(c)) - anchor;
        if (delta < -2 || delta > 2) { anchor = position(c); stable_since = 0; }
        else if (!stable_since) stable_since = now_ns();
        else if (now_ns() - stable_since >= 100000000) { disabled = true; break; }
    }
    // Last output remains disabled even if no acknowledgement can be obtained.
    frame(c, 0, false);
    printf("Stop terkonfirmasi=%s; disable/posisi stabil=%s.\n", stopped ? "YA" : "TIDAK", disabled ? "YA" : "TIDAK");
    return stopped && disabled;
}
bool MotionTrial::restore(ecx_contextt *c) {
    bool ok = true;
    for (auto p = saved.rbegin(); p != saved.rend(); ++p)
        if (p->touched && !download(c, p->index, p->size, p->value)) ok = false;
    if (changed()) printf("Pemulihan parameter awal: %s.\n", ok ? "OK" : "BELUM TERKONFIRMASI");
    return ok;
}
bool MotionTrial::run_cycle(ecx_contextt *c, const volatile sig_atomic_t *running) {
    /* Langkah 1: gerak CW (+magnitude) */
    printf("=== Siklus CW: +%d unit (%d putaran) @ %u RPM ===\n",
           magnitude, magnitude / 131072,
           (unsigned)(uint64_t(speed) * 60 / 131072));
    if (!run(c, running, int32(magnitude))) return false;

    /* Langkah 2: jeda aktif — frame PDO tetap dikirim setiap 1ms.
     * Drive LC10E memakai DC SYNC0 1ms; jika frame berhenti selama
     * jeda (clock_nanosleep) slave masuk Synchronization error (0x001A).
     * Kirim Shutdown (0x0006) agar servo kembali ke Ready state,
     * EtherCAT tetap OP, dan bisa di-enable lagi di langkah 3. */
    if (!*running) return false;
    printf("Jeda %u ms (frame PDO terus dikirim, servo ke Ready)...\n", delay_ms);
    const int64_t end_delay = now_ns() + int64_t(delay_ms) * 1000000;
    next_tick = 0;
    wkc_fail_count_ = 0;
    while (now_ns() < end_delay && *running) {
        if (!tick(running, false)) break;
        frame(c, 0x0006, false);  /* Shutdown: servo Ready, tidak cut power */
    }
    if (!*running) return false;
    printf("Jeda selesai; memulai siklus CCW.\n");

    /* Langkah 3: gerak CCW (-magnitude) */
    printf("=== Siklus CCW: -%d unit (%d putaran) @ %u RPM ===\n",
           magnitude, magnitude / 131072,
           (unsigned)(uint64_t(speed) * 60 / 131072));
    return run(c, running, -int32(magnitude));
}
