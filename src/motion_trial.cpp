#include "motion_trial.h"
#include "sdo_trace.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <time.h>

namespace {
int64_t now_ns() {
    timespec t{}; clock_gettime(CLOCK_MONOTONIC, &t);
    return int64_t(t.tv_sec) * 1000000000 + t.tv_nsec;
}
uint16 sw(ecx_contextt *c, int slave = 1) {
    uint16 v; memcpy(&v, c->slavelist[slave].inputs + 2, 2); return etohs(v);
}
uint16 err_code(ecx_contextt *c, int slave = 1) {
    uint16 v; memcpy(&v, c->slavelist[slave].inputs + 0, 2); return etohs(v);
}
int32 position(ecx_contextt *c, int slave = 1) {
    uint32 v; memcpy(&v, c->slavelist[slave].inputs + 4, 4); return int32(etohl(v));
}
void target(ecx_contextt *c, int slave, int32 p) {
    uint32 v = htoel(uint32(p)); memcpy(c->slavelist[slave].outputs + 2, &v, 4);
}
void set_cw(ecx_contextt *c, int slave, uint16 cw) {
    uint16 v = htoes(cw); memcpy(c->slavelist[slave].outputs + 0, &v, 2);
}
bool upload(ecx_contextt *c, uint16 slave, uint16 index, uint8 sub, int size, uint32 &value) {
    uint8 bytes[4]{}; int actual = size;
    begin_sdo_trace(index, sub, size);
    int wkc = ecx_SDOread(c, slave, index, sub, FALSE, &actual, bytes, EC_TIMEOUTRXM);
    if (!finish_sdo_trace(wkc, actual)) return false;
    value = 0;
    for (int i = 0; i < size; ++i) value |= uint32(bytes[i]) << (i * 8);
    return true;
}
bool download(ecx_contextt *c, uint16 slave, uint16 index, int size, uint32 value) {
    uint8 bytes[4]{};
    for (int i = 0; i < size; ++i) bytes[i] = uint8(value >> (i * 8));
    const int wkc = ecx_SDOwrite(c, slave, index, 0, FALSE, size, bytes, EC_TIMEOUTRXM);
    uint32 readback = 0;
    const bool ok = wkc > 0 && upload(c, slave, index, 0, size, readback) && readback == value;
    printf("Slave %d Parameter %04X: nilai=%u WKC=%d readback=%s\n",
           slave, index, value, wkc, ok ? "OK" : "GAGAL");
    return ok;
}
}

bool MotionTrial::changed() const {
    for (const auto &p : saved) if (p.touched) return true;
    return false;
}
bool MotionTrial::set_parameter(ecx_contextt *c, uint16 slave, uint16 index, uint32 value) {
    for (auto &p : saved) if (p.slave == slave && p.index == index) {
        // Even a missing write acknowledgement can mean a device was changed.
        p.touched = true;
        return download(c, slave, index, p.size, value);
    }
    return false;
}
bool MotionTrial::prepare(ecx_contextt *c) {
    saved.clear();
    minimum = INT32_MIN;
    maximum = INT32_MAX;
    for (int slv = 1; slv <= c->slavecount; ++slv) {
        uint32 numerator, denominator, offset, polarity, lo, hi;
        if (!upload(c, slv, 0x6091, 1, 4, numerator) || !upload(c, slv, 0x6091, 2, 4, denominator) ||
            !upload(c, slv, 0x60b0, 0, 4, offset) || !upload(c, slv, 0x607e, 0, 1, polarity) ||
            !upload(c, slv, 0x607d, 1, 4, lo) || !upload(c, slv, 0x607d, 2, 4, hi)) return false;
        printf("Slave %d poros bebas: gear=%u/%u offset=%d polarity=%02X batas=%d..%d\n",
               slv, numerator, denominator, int32(offset), polarity, int32(lo), int32(hi));
        if (!numerator || numerator != denominator || offset || (polarity & 0x80) || int32(lo) >= int32(hi)) {
            printf("Slave %d: Uji memerlukan gear 1:1, offset nol, polaritas posisi normal, dan batas posisi valid.\n", slv);
            return false;
        }
        minimum = std::max(minimum, int32(lo));
        maximum = std::min(maximum, int32(hi));

        const struct { uint16 index; int size; } params[] = {
            {0x6060, 1}, {0x607f, 4}, {0x6081, 4}, {0x6083, 4},
            {0x6084, 4}, {0x6085, 4}, {0x605a, 2}, {0x6072, 2}
        };
        const size_t base_idx = saved.size();
        for (const auto &pr : params) {
            uint32 val = 0;
            if (!upload(c, slv, pr.index, 0, pr.size, val)) return false;
            saved.push_back({uint16(slv), pr.index, pr.size, val, false});
        }
        if (saved[base_idx + 1].value < speed || !saved[base_idx + 7].value) {
            printf("Slave %d: Batas kecepatan/torsi awal tidak mendukung profil uji; tidak dinaikkan otomatis.\n", slv);
            return false;
        }
        const uint32 safe_torque = std::min(saved[base_idx + 7].value, uint32(200));
        if (!set_parameter(c, slv, 0x6060, 1) ||
            !set_parameter(c, slv, 0x607f, speed) ||
            !set_parameter(c, slv, 0x6081, speed) ||
            !set_parameter(c, slv, 0x6083, acceleration) ||
            !set_parameter(c, slv, 0x6084, acceleration) ||
            !set_parameter(c, slv, 0x6085, acceleration) ||
            !set_parameter(c, slv, 0x605a, 2) ||
            !set_parameter(c, slv, 0x6072, safe_torque)) {
            return false;
        }
    }
    return true;
}

bool MotionTrial::tick(const volatile sig_atomic_t *running, bool stopping) {
    if (!stopping && !*running) return false;
    if (!next_tick) next_tick = now_ns();
    next_tick += 4000000; // 4ms: sesuai period_ns di pdo_check.cpp
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
    if (late >= 4000000) next_tick = now_ns();
    return true;
}

bool MotionTrial::frame(ecx_contextt *c, const uint16 *controls, bool check_fault) {
    for (int slv = 1; slv <= c->slavecount; ++slv) {
        set_cw(c, slv, controls[slv]);
    }
    ecx_send_processdata(c);
    const int wkc = ecx_receive_processdata(c, EC_TIMEOUTRET);
    const int expected = c->grouplist[0].outputsWKC * 2 + c->grouplist[0].inputsWKC;
    if (wkc != expected) {
        /* Toleransi jitter NIC non-realtime (misalnya Realtek): hitung kegagalan
         * berturut-turut; batalkan hanya setelah melewati ambang batas. */
        wkc_fail_count_++;
        if (check_fault && wkc_fail_count_ > WKC_FAIL_LIMIT) {
            printf("Uji gerak: WKC=%d expected=%d selama %d siklus berturut-turut; berhenti.\n",
                   wkc, expected, wkc_fail_count_);
            return false;
        }
        /* Frame buruk tapi masih di bawah batas; terus dengan data PDO lama. */
        return check_fault ? true : false;
    }
    wkc_fail_count_ = 0;  /* reset hitungan saat WKC kembali baik */
    if (check_fault) {
        for (int slv = 1; slv <= c->slavecount; ++slv) {
            const uint16 error = err_code(c, slv);
            const uint16 s = sw(c, slv);
            if (error || (s & (0x0008 | 0x0800 | 0x2000))) {
                printf("Uji gerak: Slave %d fault/limit SW=%04X ERR=%04X; tanpa fault reset.\n", slv, s, error);
                return false;
            }
        }
    }
    return true;
}

bool MotionTrial::frame(ecx_contextt *c, uint16 control, bool check_fault) {
    std::vector<uint16> controls(c->slavecount + 1, control);
    return frame(c, controls.data(), check_fault);
}

bool MotionTrial::run(ecx_contextt *c, const volatile sig_atomic_t *running, int32 dist) {
    started_ = true;
    for (int slv = 1; slv <= c->slavecount; ++slv) {
        uint32 mode = 0;
        if (!upload(c, slv, 0x6061, 0, 1, mode) || mode != 1) {
            printf("Slave %d: Mode display 6061 belum PP (1); enable dibatalkan.\n", slv); return false;
        }
    }
    next_tick = 0;
    wkc_fail_count_ = 0;
    if (!tick(running) || !frame(c, uint16(0))) return false;

    std::vector<int32> origins(c->slavecount + 1, 0);
    std::vector<int64_t> goals64(c->slavecount + 1, 0);
    std::vector<int32> goals(c->slavecount + 1, 0);

    // Slave 1 moves +dist
    origins[1] = position(c, 1);
    goals64[1] = int64_t(origins[1]) + dist;

    // Slave 2 moves -dist (berlawanan dengan S1)
    if (c->slavecount > 1) {
        origins[2] = position(c, 2);
        goals64[2] = int64_t(origins[2]) - dist;
    }

    // Slave 3 moves +dist (searah S1, berlawanan S2 — pola zig-zag)
    if (c->slavecount > 2) {
        origins[3] = position(c, 3);
        goals64[3] = int64_t(origins[3]) + dist;
    }

    for (int slv = 1; slv <= c->slavecount; ++slv) {
        const int32 orig = origins[slv];
        const int64_t g64 = goals64[slv];
        if (orig < minimum || g64 > maximum || g64 > INT32_MAX ||
            g64 < minimum || g64 < INT32_MIN || (sw(c, slv) & 4)) {
            printf("Slave %d: Target di luar batas atau servo sudah enabled; dibatalkan.\n", slv); return false;
        }
        goals[slv] = int32(g64);
        target(c, slv, orig); // Establish current-position target BEFORE any enable.
        printf("Slave %d Profil PP: awal=%d target=%d delta=%+d, speed=%u unit/s, accel/decel=%u unit/s2.\n",
               slv, orig, goals[slv], int32(g64 - orig), speed, acceleration);
    }

    const auto sample = [&](uint16 cw, bool enabled) {
        if (!tick(running) || !frame(c, cw)) return false;
        /* Periksa posisi hanya saat WKC valid (wkc_fail_count_==0) untuk
         * menghindari keputusan salah dari data stale akibat jitter NIC. */
        if (wkc_fail_count_ == 0) {
            for (int slv = 1; slv <= c->slavecount; ++slv) {
                const int64_t pos = position(c, slv);
                /* Batas gerak dihitung dari min/max origin & goal agar benar
                 * untuk arah maju maupun mundur. Beri toleransi wajar (1/8 putaran = 16384 unit). */
                const int64_t lo = std::min(int64_t(origins[slv]), goals64[slv]);
                const int64_t hi = std::max(int64_t(origins[slv]), goals64[slv]);
                if (pos < lo - 16384 || pos > hi + 16384) {
                    printf("Slave %d: Posisi keluar rentang uji: %lld (rentang: %lld..%lld).\n",
                           slv, static_cast<long long>(pos),
                           static_cast<long long>(lo - 16384),
                           static_cast<long long>(hi + 16384)); return false;
                }
                if (enabled && (sw(c, slv) & 0x006f) != 0x0027) {
                    printf("Slave %d: Servo kehilangan enable; tidak melakukan re-enable.\n", slv); return false;
                }
            }
        }
        return true;
    };

    // Fault reset via PDO: jika drive masih dalam Fault state (SW bit 3) dari sesi sebelumnya,
    // kirim rising edge bit 7 pada control word untuk membersihkannya sebelum enable.
    {
        bool any_fault = false;
        for (int slv = 1; slv <= c->slavecount; ++slv)
            if (sw(c, slv) & 0x0008) { any_fault = true; break; }
        if (any_fault) {
            printf("Fault sisa terdeteksi (SW bit 3); kirim fault reset via PDO...\n");
            // Phase 1: assert bit 7 selama ~300ms (rising edge fault reset)
            const int64_t t_assert = now_ns() + 300000000LL;
            while (now_ns() < t_assert) {
                if (!sample(uint16(0x0080), false)) return false;
            }
            // Phase 2: turunkan bit 7, tunggu fault bit hilang (maks 2 detik)
            const int64_t t_clear = now_ns() + 2000000000LL;
            bool cleared = false;
            while (now_ns() < t_clear) {
                if (!sample(uint16(0x0000), false)) return false;
                bool all_ok = true;
                for (int slv = 1; slv <= c->slavecount; ++slv)
                    if (sw(c, slv) & 0x0008) { all_ok = false; break; }
                if (all_ok) { cleared = true; break; }
            }
            if (!cleared) {
                printf("Fault tidak dapat direset via PDO");
                for (int slv = 1; slv <= c->slavecount; ++slv)
                    printf(" SW%d=%04X", slv, sw(c, slv));
                printf(". Matikan dan hidupkan drive lalu coba lagi.\n");
                return false;
            }
            printf("Fault berhasil direset via PDO; melanjutkan enable.\n");
        }
    }

    // HALT remains set during enable. Only the single new-setpoint releases it.
    const uint16 commands[] = {0x0106, 0x0107, 0x010f};
    const uint16 statuses[] = {0x0021, 0x0023, 0x0027};
    for (int step = 0; step < 3; ++step) {
        const int64_t end = now_ns() + 2000000000LL;
        bool reached = false;
        while (now_ns() < end) {
            if (!sample(commands[step], false)) return false;
            bool all_reached = true;
            for (int slv = 1; slv <= c->slavecount; ++slv) {
                if ((sw(c, slv) & 0x006f) != statuses[step]) { all_reached = false; break; }
            }
            if (all_reached) { reached = true; break; }
        }
        if (!reached) {
            printf("Timeout CiA402 step %d.\n", step);
            for (int slv = 1; slv <= c->slavecount; ++slv)
                printf("  Slave %d SW=%04X\n", slv, sw(c, slv));
            return false;
        }
    }

    printf("Servo enabled dalam PP; memulai gerakan.");
    for (int slv = 1; slv <= c->slavecount; ++slv) printf(" S%d", slv);
    printf(".\n");
    for (int slv = 1; slv <= c->slavecount; ++slv) {
        if (sw(c, slv) & 0x1000) { printf("Slave %d: Setpoint acknowledge belum nol; dibatalkan.\n", slv); return false; }
        target(c, slv, goals[slv]);
    }

    bool acknowledged = false, ack_cleared = false;
    const int64_t start = now_ns();
    int64_t settled_since = 0, next_report = start, next_terminal_log = start;
    /* Timeout total gerakan: distance/speed + ramp + toleransi aman (45 detik). */
    while (now_ns() - start < 45000000000LL) {
        if (!sample(acknowledged ? 0x002f : 0x003f, true)) return false;
        bool all_ack = true;
        for (int slv = 1; slv <= c->slavecount; ++slv) {
            if (!(sw(c, slv) & 0x1000)) { all_ack = false; break; }
        }
        if (all_ack) acknowledged = true;
        else if (acknowledged) {
            bool any_ack = false;
            for (int slv = 1; slv <= c->slavecount; ++slv) {
                if (sw(c, slv) & 0x1000) { any_ack = true; break; }
            }
            if (!any_ack) ack_cleared = true;
        }

        if (!acknowledged && now_ns() - start > 2000000000LL) {
            printf("Timeout acknowledge setpoint; berhenti.\n"); return false;
        }
        if (acknowledged && !ack_cleared && now_ns() - start > 3000000000LL) {
            printf("Acknowledge tidak kembali nol; berhenti.\n"); return false;
        }

        bool all_settled = ack_cleared;
        for (int slv = 1; slv <= c->slavecount; ++slv) {
            const auto status = sw(c, slv);
            const int64_t error = int64_t(position(c, slv)) - goals[slv];
            if (!(status & 0x0400) || error < -256 || error > 256) {
                all_settled = false;
                break;
            }
        }

        if (all_settled) {
            if (!settled_since) settled_since = now_ns();
            if (now_ns() - settled_since >= 200000000LL) {
                for (int slv = 1; slv <= c->slavecount; ++slv) {
                    printf("Slave %d: Target tercapai: posisi=%d delta_aktual=%lld.\n",
                           slv, position(c, slv),
                           static_cast<long long>(int64_t(position(c, slv)) - origins[slv]));
                }
                return true;
            }
        } else settled_since = 0;

        if (now_ns() >= next_report) {
            const int expected_wkc = c->grouplist[0].outputsWKC * 2 + c->grouplist[0].inputsWKC;
            int16_t t1 = 0, t2 = 0, t3 = 0;
            memcpy(&t1, c->slavelist[1].inputs + 6, 2);
            if (c->slavecount > 1) memcpy(&t2, c->slavelist[2].inputs + 6, 2);
            if (c->slavecount > 2) memcpy(&t3, c->slavelist[3].inputs + 6, 2);
            printf("TELEMETRY: pos1=%d sw1=%04X torq1=%d pos2=%d sw2=%04X torq2=%d pos3=%d sw3=%04X torq3=%d wkc=%d\n",
                   position(c, 1), sw(c, 1), (int)etohs(t1),
                   c->slavecount > 1 ? position(c, 2) : 0,
                   c->slavecount > 1 ? sw(c, 2) : 0, (int)etohs(t2),
                   c->slavecount > 2 ? position(c, 3) : 0,
                   c->slavecount > 2 ? sw(c, 3) : 0, (int)etohs(t3),
                   expected_wkc);
            if (now_ns() >= next_terminal_log) {
                printf("PP S1: pos=%d tgt=%d SW=%04X", position(c,1), goals[1], sw(c,1));
                for (int slv = 2; slv <= c->slavecount; ++slv)
                    printf(" | S%d: pos=%d tgt=%d SW=%04X", slv, position(c,slv), goals[slv], sw(c,slv));
                printf(" WKC=%d/%d\n", expected_wkc, expected_wkc);
                next_terminal_log = now_ns() + 500000000LL;
            }
            fflush(stdout);
            next_report = now_ns() + 50000000LL; // 20 Hz
        }
    }
    printf("Timeout target 45 detik; berhenti.\n"); return false;
}

bool MotionTrial::stop(ecx_contextt *c) {
    // Do not use the cancelled run flag: transmit a bounded stop even on Ctrl+C.
    printf("Mengirim quick stop lalu disable; tidak melakukan re-enable.\n");
    volatile sig_atomic_t cleanup_running = 1;
    next_tick = 0;
    int64_t start = now_ns(), stable_since = 0;
    std::vector<int32> anchors(c->slavecount + 1, 0);
    for (int slv = 1; slv <= c->slavecount; ++slv) anchors[slv] = position(c, slv);
    bool stopped = false;
    while (now_ns() - start < 1500000000LL) {
        if (!tick(&cleanup_running, true)) break;
        if (!frame(c, uint16(0x0002), false)) { stable_since = 0; continue; }
        bool any_moving = false;
        for (int slv = 1; slv <= c->slavecount; ++slv) {
            const int64_t delta = int64_t(position(c, slv)) - anchors[slv];
            if (delta < -2 || delta > 2) {
                anchors[slv] = position(c, slv);
                any_moving = true;
            }
        }
        if (any_moving) stable_since = 0;
        else if (!stable_since) stable_since = now_ns();
        else if (now_ns() - stable_since >= 100000000LL) { stopped = true; break; }
    }
    bool disabled = false;
    start = now_ns(); stable_since = 0;
    for (int slv = 1; slv <= c->slavecount; ++slv) anchors[slv] = position(c, slv);
    while (now_ns() - start < 500000000LL) {
        if (!tick(&cleanup_running, true)) break;
        if (!frame(c, uint16(0), false)) { stable_since = 0; continue; }
        bool any_enabled = false;
        for (int slv = 1; slv <= c->slavecount; ++slv) {
            if (sw(c, slv) & 4) any_enabled = true;
            const int64_t delta = int64_t(position(c, slv)) - anchors[slv];
            if (delta < -2 || delta > 2) {
                anchors[slv] = position(c, slv);
                any_enabled = true;
            }
        }
        if (any_enabled) stable_since = 0;
        else if (!stable_since) stable_since = now_ns();
        else if (now_ns() - stable_since >= 100000000LL) { disabled = true; break; }
    }
    // Last output remains disabled even if no acknowledgement can be obtained.
    frame(c, uint16(0), false);
    printf("Stop terkonfirmasi=%s; disable/posisi stabil=%s.\n", stopped ? "YA" : "TIDAK", disabled ? "YA" : "TIDAK");
    return stopped && disabled;
}

bool MotionTrial::restore(ecx_contextt *c) {
    bool ok = true;
    for (auto p = saved.rbegin(); p != saved.rend(); ++p) {
        if (p->touched && !download(c, p->slave, p->index, p->size, p->value)) ok = false;
    }
    if (changed()) printf("Pemulihan parameter awal: %s.\n", ok ? "OK" : "BELUM TERKONFIRMASI");
    return ok;
}

bool MotionTrial::run_cycle(ecx_contextt *c, const volatile sig_atomic_t *running) {
    if (dance_mode_) return dance_cycle(c, running);

    /* Langkah 1: gerak maju (Slave 1 +magnitude, Slave 2 -magnitude) */
    if (c->slavecount > 1) {
        printf("=== Siklus 1: Slave 1 CW (+%d), Slave 2 CCW (-%d) [%d putaran] @ %u RPM ===\n",
               magnitude, magnitude, magnitude / 131072,
               (unsigned)(uint64_t(speed) * 60 / 131072));
    } else {
        printf("=== Siklus CW: +%d unit (%d putaran) @ %u RPM ===\n",
               magnitude, magnitude / 131072,
               (unsigned)(uint64_t(speed) * 60 / 131072));
    }
    if (!run(c, running, int32(magnitude))) return false;

    /* Langkah 2: jeda aktif — frame PDO tetap dikirim setiap 4ms. */
    if (!*running) return false;
    printf("Jeda %u ms (frame PDO terus dikirim, servo ke Ready)...\n", delay_ms);
    const int64_t end_delay = now_ns() + int64_t(delay_ms) * 1000000LL;
    next_tick = 0;
    wkc_fail_count_ = 0;
    while (now_ns() < end_delay && *running) {
        if (!tick(running, false)) break;
        frame(c, uint16(0x0006), false);
    }
    if (!*running) return false;
    printf("Jeda selesai; memulai siklus balik arah.\n");

    /* Langkah 3: gerak mundur (Slave 1 -magnitude, Slave 2 +magnitude) */
    if (c->slavecount > 1) {
        printf("=== Siklus 2: Slave 1 CCW (-%d), Slave 2 CW (+%d) [%d putaran] @ %u RPM ===\n",
               magnitude, magnitude, magnitude / 131072,
               (unsigned)(uint64_t(speed) * 60 / 131072));
    } else {
        printf("=== Siklus CCW: -%d unit (%d putaran) @ %u RPM ===\n",
               magnitude, magnitude / 131072,
               (unsigned)(uint64_t(speed) * 60 / 131072));
    }
    return run(c, running, -int32(magnitude));
}

/* ── Gerakan per-slave fleksibel: dists[0]=Slave1, dists[1]=Slave2 (0=tahan) ── */
bool MotionTrial::run_custom(ecx_contextt *c, const volatile sig_atomic_t *running,
                              const std::vector<int32_t>& dists) {
    started_ = true;
    const int n = c->slavecount;
    for (int slv = 1; slv <= n; ++slv) {
        uint32 mode = 0;
        if (!upload(c, slv, 0x6061, 0, 1, mode) || mode != 1) {
            printf("Slave %d: Mode PP (6061) belum 1; enable dibatalkan.\n", slv);
            return false;
        }
    }
    next_tick = 0; wkc_fail_count_ = 0;
    if (!tick(running) || !frame(c, uint16(0))) return false;

    std::vector<int32_t> origins(n + 1, 0);
    std::vector<int64_t> goals64(n + 1, 0);
    std::vector<int32_t> goals(n + 1, 0);
    std::vector<bool>    active(n + 1, false);

    for (int slv = 1; slv <= n; ++slv) {
        origins[slv] = position(c, slv);
        const int32_t d = (slv - 1 < (int)dists.size()) ? dists[slv - 1] : 0;
        active[slv] = (d != 0);
        goals64[slv] = int64_t(origins[slv]) + d;
        if (goals64[slv] > INT32_MAX) goals64[slv] = INT32_MAX;
        if (goals64[slv] < INT32_MIN) goals64[slv] = INT32_MIN;
        goals[slv] = int32_t(goals64[slv]);
        target(c, slv, origins[slv]);
        printf("  Slave %d: %s awal=%-10d target=%-10d (delta=%+d)\n", slv,
               active[slv] ? "GERAK" : "TAHAN", origins[slv], goals[slv], goals[slv] - origins[slv]);
        if ((sw(c, slv) & 4)) { printf("Slave %d sudah enabled; dibatalkan.\n", slv); return false; }
    }

    const auto sample = [&](uint16 cw, bool enabled) {
        if (!tick(running) || !frame(c, cw)) return false;
        if (wkc_fail_count_ == 0) {
            for (int slv = 1; slv <= n; ++slv) {
                if (!active[slv]) continue;
                const int64_t pos = position(c, slv);
                const int64_t lo = std::min(int64_t(origins[slv]), goals64[slv]);
                const int64_t hi = std::max(int64_t(origins[slv]), goals64[slv]);
                if (pos < lo - 16384 || pos > hi + 16384) {
                    printf("Slave %d: Posisi %lld keluar rentang [%lld..%lld].\n",
                           slv, (long long)pos, (long long)(lo-16384), (long long)(hi+16384));
                    return false;
                }
                if (enabled && (sw(c, slv) & 0x006f) != 0x0027) {
                    printf("Slave %d: Servo kehilangan enable (SW=%04X).\n", slv, sw(c, slv));
                    return false;
                }
            }
        }
        return true;
    };

    // PDO fault reset jika perlu
    {
        bool any_fault = false;
        for (int slv = 1; slv <= n; ++slv)
            if (sw(c, slv) & 0x0008) { any_fault = true; break; }
        if (any_fault) {
            printf("  Fault sisa; reset via PDO...\n");
            const int64_t ta = now_ns() + 300000000LL;
            while (now_ns() < ta) if (!sample(uint16(0x0080), false)) return false;
            const int64_t tc = now_ns() + 2000000000LL;
            bool cleared = false;
            while (now_ns() < tc) {
                if (!sample(uint16(0x0000), false)) return false;
                bool ok = true;
                for (int slv = 1; slv <= n; ++slv)
                    if (sw(c, slv) & 0x0008) { ok = false; break; }
                if (ok) { cleared = true; break; }
            }
            if (!cleared) { printf("Fault tidak dapat direset.\n"); return false; }
        }
    }

    // CiA402 enable sequence (semua slave)
    const uint16 commands[] = {0x0106, 0x0107, 0x010f};
    const uint16 statuses[]  = {0x0021, 0x0023, 0x0027};
    for (int step = 0; step < 3; ++step) {
        const int64_t end = now_ns() + 2000000000LL;
        bool ok = false;
        while (now_ns() < end) {
            if (!sample(commands[step], false)) return false;
            bool all = true;
            for (int slv = 1; slv <= n; ++slv)
                if ((sw(c, slv) & 0x006f) != statuses[step]) { all = false; break; }
            if (all) { ok = true; break; }
        }
        if (!ok) {
            printf("Timeout CiA402 step %d.\n", step);
            for (int slv = 1; slv <= n; ++slv) printf("  Slave %d SW=%04X\n", slv, sw(c, slv));
            return false;
        }
    }

    for (int slv = 1; slv <= n; ++slv) {
        if (sw(c, slv) & 0x1000) { printf("Slave %d: Setpoint ack belum nol.\n", slv); return false; }
        target(c, slv, goals[slv]);
    }

    bool acknowledged = false, ack_cleared = false;
    const int64_t start = now_ns();
    int64_t settled_since = 0, next_report = start;

    while (now_ns() - start < 30000000000LL) {
        if (!sample(acknowledged ? 0x002f : 0x003f, true)) return false;

        bool all_ack = true;
        for (int slv = 1; slv <= n; ++slv)
            if (!(sw(c, slv) & 0x1000)) { all_ack = false; break; }
        if (all_ack) acknowledged = true;
        else if (acknowledged) {
            bool any = false;
            for (int slv = 1; slv <= n; ++slv)
                if (sw(c, slv) & 0x1000) { any = true; break; }
            if (!any) ack_cleared = true;
        }
        if (!acknowledged && now_ns() - start > 2000000000LL) {
            printf("Timeout acknowledge setpoint.\n"); return false;
        }

        bool all_settled = ack_cleared;
        for (int slv = 1; slv <= n && all_settled; ++slv) {
            if (!active[slv]) continue;
            const int64_t err = int64_t(position(c, slv)) - goals[slv];
            if (!(sw(c, slv) & 0x0400) || err < -256 || err > 256) all_settled = false;
        }
        if (all_settled) {
            if (!settled_since) settled_since = now_ns();
            if (now_ns() - settled_since >= 200000000LL) {
                for (int slv = 1; slv <= n; ++slv)
                    printf("  → Slave %d: pos=%d target=%d\n", slv, position(c, slv), goals[slv]);
                return true;
            }
        } else settled_since = 0;

        if (now_ns() >= next_report) {
            const int ewkc = c->grouplist[0].outputsWKC * 2 + c->grouplist[0].inputsWKC;
            int16_t t1 = 0, t2 = 0, t3 = 0;
            memcpy(&t1, c->slavelist[1].inputs + 6, 2);
            if (n > 1) memcpy(&t2, c->slavelist[2].inputs + 6, 2);
            if (n > 2) memcpy(&t3, c->slavelist[3].inputs + 6, 2);
            printf("TELEMETRY: pos1=%d sw1=%04X torq1=%d pos2=%d sw2=%04X torq2=%d pos3=%d sw3=%04X torq3=%d wkc=%d\n",
                   position(c,1), sw(c,1), (int)etohs(t1),
                   n > 1 ? position(c,2) : 0, n > 1 ? sw(c,2) : 0, (int)etohs(t2),
                   n > 2 ? position(c,3) : 0, n > 2 ? sw(c,3) : 0, (int)etohs(t3), ewkc);
            fflush(stdout);
            next_report = now_ns() + 50000000LL;
        }
    }
    printf("Timeout 30s; target belum tercapai.\n");
    return false;
}



bool MotionTrial::dance_cycle(ecx_contextt *c, const volatile sig_atomic_t *running) {
    /* 1 putaran = 131072 unit */
    const int32_t R2 = 131072 * 2;  // 2 putaran
    const int32_t R3 = 131072 * 3;  // 3 putaran
    const int32_t R4 = 131072 * 4;  // 4 putaran
    const int n = c->slavecount;

    struct Phase { const char *nama; std::vector<int32_t> dists; };

    /* Pola untuk 1 servo (fallback) */
    const Phase phases1[] = {
        {"1/4  ➤ MAJU  (+4rot)",  { R4}},
        {"2/4  ◀ BALIK (-4rot)",  {-R4}},
        {"3/4  ➤ MAJU  (+4rot)",  { R4}},
        {"4/4  ◀ BALIK (-4rot)",  {-R4}},
    };

    /* Pola untuk 2 servo */
    const Phase phases2[] = {
        {"1/8  ➤ SEREMPAK MAJU      (S1:+2rot, S2:+2rot)",      { R2,  R2}},
        {"2/8  ◀ SEREMPAK BALIK     (S1:-2rot, S2:-2rot)",       {-R2, -R2}},
        {"3/8  ➤ SOLO Slave 1 MAJU  (S1:+3rot, S2:tahan)",       { R3,    0}},
        {"4/8  ➤ SOLO Slave 2 MAJU  (S1:tahan, S2:+3rot)",       {   0,  R3}},
        {"5/8  ◀ SOLO Slave 1 BALIK (S1:-3rot, S2:tahan)",       {-R3,    0}},
        {"6/8  ◀ SOLO Slave 2 BALIK (S1:tahan, S2:-3rot)",       {   0, -R3}},
        {"7/8  ↔ BERLAWANAN CERMIN  (S1:+4rot, S2:-4rot)",       { R4, -R4}},
        {"8/8  ↔ BALIK CERMIN       (S1:-4rot, S2:+4rot)",       {-R4,  R4}},
    };

    /* Pola untuk 3 servo — koreografi lebih kaya */
    const Phase phases3[] = {
        {"1/8  ➤ SEREMPAK MAJU      (S1:+2, S2:+2, S3:+2)",     { R2,  R2,  R2}},
        {"2/8  ◀ SEREMPAK BALIK     (S1:-2, S2:-2, S3:-2)",      {-R2, -R2, -R2}},
        {"3/8  ➤ SOLO S1 MAJU       (S1:+3, S2:tahan, S3:tahan)",{ R3,    0,    0}},
        {"4/8  ➤ SOLO S2 MAJU       (S1:tahan, S2:+3, S3:tahan)",{   0,  R3,    0}},
        {"5/8  ➤ SOLO S3 MAJU       (S1:tahan, S2:tahan, S3:+3)",{   0,    0,  R3}},
        {"6/8  ◀ BALIK SEMUA        (S1:-3, S2:-3, S3:-3)",      {-R3, -R3, -R3}},
        {"7/8  ↔ ZIG-ZAG A          (S1:+4, S2:-4, S3:+4)",      { R4, -R4,  R4}},
        {"8/8  ↔ ZIG-ZAG BALIK      (S1:-4, S2:+4, S3:-4)",      {-R4,  R4, -R4}},
    };

    const Phase *phases;
    int np;
    const char *mode_str;
    if (n >= 3) {
        phases = phases3; np = (int)(sizeof(phases3)/sizeof(phases3[0]));
        mode_str = "TARIAN TRIPLE SERVO — 8 FASE @ ~240 RPM          ";
    } else if (n == 2) {
        phases = phases2; np = (int)(sizeof(phases2)/sizeof(phases2[0]));
        mode_str = "TARIAN DUAL SERVO — 8 FASE @ ~240 RPM            ";
    } else {
        phases = phases1; np = (int)(sizeof(phases1)/sizeof(phases1[0]));
        mode_str = "TARIAN SOLO SERVO — 4 FASE @ ~240 RPM            ";
    }

    printf("\n\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n");
    printf("\xe2\x95\x91  %s\xe2\x95\x91\n", mode_str);
    printf("\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n\n");

    for (int ph = 0; ph < np && *running; ++ph) {
        printf("\xe2\x96\xb6 Fase %s\n", phases[ph].nama);
        if (!run_custom(c, running, phases[ph].dists)) return false;
        if (ph < np - 1 && *running) {
            printf("  \xe2\x9c\x93 Selesai. Jeda 1 detik...\n\n");
            const int64_t end_p = now_ns() + 1000000000LL;
            next_tick = 0; wkc_fail_count_ = 0;
            while (now_ns() < end_p && *running) {
                if (!tick(running, false)) break;
                frame(c, uint16(0x0006), false);
            }
        }
    }

    if (*running) {
        printf("\n\xe2\x95\x94\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x97\n");
        printf("\xe2\x95\x91            TARIAN SELESAI!  \xe2\x9c\x93  LULUS                 \xe2\x95\x91\n");
        printf("\xe2\x95\x9a\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x90\xe2\x95\x9d\n");
    }
    return true;
}
