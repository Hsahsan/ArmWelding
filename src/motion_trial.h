#pragma once
#include "soem/soem.h"
#include <signal.h>
#include <cstdint>
#include <vector>

// One slow, positive point-to-point move. For the user's unloaded R17 motor.
// XML 1702/1B02 only; offsets below are verified by the shared bus setup.
class MotionTrial {
    struct Saved { uint16 index; int size; uint32 value; bool touched = false; };
    std::vector<Saved> saved;
    int32 minimum = 0, maximum = 0;
    int64_t next_tick = 0;
    bool started_ = false;
    /* Toleransi jitter NIC non-realtime (Realtek, dll.):
     * Izinkan hingga N siklus WKC buruk berturut-turut sebelum berhenti.
     * Realtek 8611 kadang drop 1-3 frame berturut-turut; batas 8 cukup. */
    static constexpr int WKC_FAIL_LIMIT = 8;
    int wkc_fail_count_ = 0;
    bool frame(ecx_contextt *, uint16 control, bool check_fault = true);
    bool tick(const volatile sig_atomic_t *, bool stop = false);
    bool set_parameter(ecx_contextt *, uint16 index, uint32 value);
public:
    /* Magnitude gerakan: 100 putaran = 100 × 131072 unit.
     * Arah ditentukan saat run_cycle(): maju (+magnitude) lalu mundur (-magnitude). */
    static constexpr int32  magnitude  = 13107200;  /* 100 putaran = 36000 derajat */
    static constexpr uint32 delay_ms   = 3000;      /* jeda CW→CCW dalam milidetik */
    /* Kecepatan 200 RPM: 200/60 × 131072 ≈ 436907 → dibulatkan ke 524288 (256*2048)
     * agar kelipatan rapi; akselerasi 2× speed → ramp ~0.25 detik. */
    static constexpr uint32 speed        = 524288;   /* unit/s ≈ 240 RPM */
    static constexpr uint32 acceleration = 1048576;  /* unit/s² ramp 0.5s */
    bool prepare(ecx_contextt *);
    bool run(ecx_contextt *, const volatile sig_atomic_t *, int32 dist);
    bool run_cycle(ecx_contextt *, const volatile sig_atomic_t *);
    bool stop(ecx_contextt *);
    bool restore(ecx_contextt *);
    bool started() const { return started_; }
    bool changed() const;
};
