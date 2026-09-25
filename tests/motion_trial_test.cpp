// Whole test_rotate path on a fake bus/clock. No SOEM library or raw socket.
#include "soem/soem.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <climits>
#include <map>
#include <utility>
#include <signal.h>
#include <time.h>

int test_rotate(const char *, const volatile sig_atomic_t *);
extern "C" int __wrap_ecx_readPDOmap(ecx_contextt *, uint16, uint32 *, uint32 *);
static int expected_size;
void begin_sdo_trace(uint16_t, uint8_t, int size) { expected_size = size; }
bool finish_sdo_trace(int wkc, int size) { return wkc > 0 && size == expected_size; }
namespace {
enum Case { Good, Cancel, Fault, WkcLoss, StopWkcLoss, EnableLoss, StateLoss,
            Stall, NoAck, AckStuck, Overshoot, WrongDirection, Late,
            WrongMode, WriteFailure, ReadFailure, RestoreFailure, Bounds,
            Overflow, BadGear, BadOffset, BadPolarity, ZeroTorque, LowSpeed,
            ExistingFault, NoReady, PartialWriteFailure, Last = PartialWriteFailure };
Case scenario;
using Key = std::pair<uint16, uint8>;
std::map<Key, uint32> parameters, original;
ecx_contextt *context;
volatile sig_atomic_t running;
int64_t fake_time;
uint16 command, previous_command, status;
int32 pos, initial_pos, dest;
int motion_frames, triggers, enable_frames, stop_frames, sdo_writes;
bool triggered, stopping, closed, dc_on, injected, mapped;
template<typename T> int reply(void *p, int *size, T value) {
    assert(*size == int(sizeof(value))); memcpy(p, &value, sizeof(value)); return 1;
}
}
extern "C" {
int clock_gettime(clockid_t, timespec *t) noexcept {
    t->tv_sec = fake_time / 1000000000; t->tv_nsec = fake_time % 1000000000; return 0;
}
int clock_nanosleep(clockid_t, int, const timespec *t, timespec *) {
    fake_time = int64_t(t->tv_sec) * 1000000000 + t->tv_nsec;
    if (scenario == Late && triggered && !injected) { fake_time += 21000000; injected = true; }
    return 0;
}
int ecx_init(ecx_contextt *c, const char *) { context = c; return 1; }
void ecx_close(ecx_contextt *c) {
    assert(c->slavelist[1].state == EC_STATE_INIT); assert(!dc_on); closed = true;
}
int ecx_config_init(ecx_contextt *c) {
    c->slavecount = 1; auto &s = c->slavelist[1];
    s.eep_man = 0x766; s.eep_id = 0x402; s.eep_rev = 0x204;
    s.configadr = 0x1001; s.hasdc = TRUE; s.mbx_l = s.mbx_rl = 128;
    s.state = EC_STATE_INIT; return 1;
}
int ecx_readstate(ecx_contextt *) { return 1; }
int ecx_writestate(ecx_contextt *, uint16) { return 1; }
uint16 ecx_statecheck(ecx_contextt *c, uint16 s, uint16, int) { return c->slavelist[s].state & 15; }
boolean ecx_iserror(ecx_contextt *) { return FALSE; }
char *ecx_elist2string(ecx_contextt *) { static char v[] = ""; return v; }
char *ec_ALstatuscode2string(uint16) { static char v[] = "mock"; return v; }
int ecx_SDOread(ecx_contextt *, uint16 slave, uint16 index, uint8 sub, boolean ca,
                int *size, void *p, int) {
    assert(slave == 1 && !ca);
    if (scenario == ReadFailure && index == 0x6083 && !sdo_writes) return 0;
    if (index == 0x6041) return reply(p, size, htoes(uint16(scenario == ExistingFault ? 8 : 0x250)));
    if (index == 0x603f) return reply(p, size, uint16(0));
    // Deliberately stale SDO position: first enabled PDO must use fresh feedback.
    if (index == 0x6064) return reply(p, size, htoel(uint32(26130)));
    if (index == 0x6061) return reply(p, size, uint8(scenario == WrongMode ? 0 : parameters[{0x6060, 0}]));
    if (index == 0x1c12 || index == 0x1c13) {
        return sub ? reply(p, size, htoes(uint16(index == 0x1c12 ? 0x1702 : 0x1b02))) : reply(p, size, uint8(1));
    }
    if (index == 0x1702 || index == 0x1b02) {
        assert(sub == 0); return reply(p, size, uint8(index == 0x1702 ? 6 : 9));
    }
    assert(parameters.count({index, sub}));
    const uint32 value = parameters.at({index, sub});
    for (int i = 0; i < *size; ++i) static_cast<uint8 *>(p)[i] = uint8(value >> (i * 8));
    return 1;
}
int ecx_SDOwrite(ecx_contextt *c, uint16 slave, uint16 index, uint8 sub,
                 boolean ca, int size, const void *p, int) {
    assert(slave == 1 && !ca && sub == 0 && c->slavelist[1].state == EC_STATE_PRE_OP);
    assert(index == 0x6060 || index == 0x607f || index == 0x6081 || index == 0x6083 ||
           index == 0x6084 || index == 0x6085 || index == 0x605a || index == 0x6072);
    uint32 value = 0;
    for (int i = 0; i < size; ++i) value |= uint32(static_cast<const uint8 *>(p)[i]) << (i * 8);
    ++sdo_writes;
    const bool restore = value == original.at({index, sub});
    if (scenario == RestoreFailure && index == 0x6081 && restore) return 0;
    if (scenario == WriteFailure && index == 0x6083 && !restore) return 0;
    parameters[{index, sub}] = value;
    if (scenario == PartialWriteFailure && index == 0x6083 && !restore) return 0;
    return 1;
}
int __real_ecx_readPDOmap(ecx_contextt *, uint16, uint32 *, uint32 *) { assert(false); return 0; }
int __real_ecx_readPDOmapCA(ecx_contextt *, uint16, int, uint32 *, uint32 *) { assert(false); return 0; }
int ecx_config_map_group(ecx_contextt *c, void *map, uint8) {
    assert(c->manualstatechange && c->slavelist[1].PO2SOconfig == nullptr);
    uint32 o, i; assert(__wrap_ecx_readPDOmap(c, 1, &o, &i) == 1 && o == 120 && i == 224);
    auto &s = c->slavelist[1]; auto &g = c->grouplist[0]; mapped = true;
    s.Obits = o; s.Ibits = i; s.Obytes = g.Obytes = 15; s.Ibytes = g.Ibytes = 28;
    s.outputs = g.outputs = static_cast<uint8 *>(map); s.inputs = g.inputs = s.outputs + 15;
    s.mbxstatus = g.mbxstatus = s.inputs + 28; g.mbxstatuslength = 1; g.mbxstatuslookup[0] = 1;
    g.outputsWKC = g.inputsWKC = 1; return 44;
}
boolean ecx_configdc(ecx_contextt *) { return TRUE; }
void ecx_dcsync0(ecx_contextt *, uint16, boolean on, uint32, int32) { dc_on = on; }
int ecx_FPRD(ecx_portt *, uint16, uint16 ado, uint16 size, void *p, int) {
    if (ado == 0x0810 || ado == 0x0818) {
        assert(size == sizeof(ec_smt)); memcpy(p, &context->slavelist[1].SM[ado == 0x0810 ? 2 : 3], size);
    } else if (ado == ECT_REG_DCSYNCACT) { assert(size == 1); *static_cast<uint8 *>(p) = 3; }
    else { assert(ado == ECT_REG_DCCYCLE0 && size == 4); const uint32 cycle = htoel(1000000); memcpy(p, &cycle, 4); }
    return 1;
}
int ecx_send_processdata(ecx_contextt *c) {
    const auto *out = c->slavelist[1].outputs;
    memcpy(&command, out, 2); command = etohs(command);
    uint32 t; memcpy(&t, out + 2, 4); dest = int32(etohl(t));
    assert(command == 0 || command == 2 || command == 0x106 || command == 0x107 ||
           command == 0x10f || command == 0x3f || command == 0x2f);
    assert(out[6] == 0 && out[7] == 0 && out[8] == 0 && out[9] == 0);
    assert(out[10] == 0 && out[11] == 0x20 && out[12] == 0 && out[13] == 0 && out[14] == 1);
    if (command & 0x100) { ++enable_frames; assert(dest == initial_pos); }
    if ((command & 0x10) && !(previous_command & 0x10)) {
        assert(!triggered); triggered = true; ++triggers;
        assert(dest == int64_t(initial_pos) + 32768);
    }
    if (command == 2) { stopping = true; ++stop_frames; }
    if (stopping) assert(command == 2 || command == 0);
    previous_command = command;
    return 1;
}
int ecx_receive_processdata(ecx_contextt *c, int) {
    status = 0x250;
    if (command == 0x106) status = scenario == NoReady ? 0x250 : 0x221;
    if (command == 0x107) status = 0x223;
    if (command == 0x10f) status = 0x627;
    if (triggered && !stopping) {
        ++motion_frames; status = 0x227;
        if (scenario != Stall) pos += std::min<int32>(8, dest - pos);
        if (scenario == WrongDirection) pos = initial_pos - 513;
        if (scenario == Overshoot) pos = dest + 513;
        if (scenario != NoAck && (command == 0x3f || scenario == AckStuck)) status |= 0x1000;
        if (pos == dest) status |= 0x400;
        if (scenario == Fault) status |= 8;
        if (scenario == EnableLoss) status &= ~uint16(4);
        if (scenario == Cancel && motion_frames == 20) running = 0;
        if (scenario == StateLoss) c->slavelist[1].state = EC_STATE_SAFE_OP;
    }
    auto *in = c->slavelist[1].inputs;
    uint16 wire_status = htoes(status); uint32 wire_pos = htoel(uint32(pos));
    memcpy(in + 2, &wire_status, 2); memcpy(in + 4, &wire_pos, 4);
    c->DCtime += 1000000;
    if (scenario == WkcLoss && triggered && !stopping) return 0;
    if (scenario == StopWkcLoss && stopping) return 0;
    return 3;
}
}

int main() {
    for (int n = 0; n <= Last; ++n) {
        scenario = Case(n);
        parameters = {{{0x6091, 1}, 1}, {{0x6091, 2}, 1}, {{0x60b0, 0}, 0}, {{0x607e, 0}, 0},
            {{0x607d, 1}, uint32(INT32_MIN)}, {{0x607d, 2}, uint32(INT32_MAX)},
            {{0x6060, 0}, 0}, {{0x607f, 0}, 3000000}, {{0x6081, 0}, 12345},
            {{0x6083, 0}, 100000}, {{0x6084, 0}, 100000}, {{0x6085, 0}, 100000},
            {{0x605a, 0}, 6}, {{0x6072, 0}, 1000}};
        if (scenario == BadGear) parameters[{0x6091, 1}] = 2;
        if (scenario == BadOffset) parameters[{0x60b0, 0}] = 1;
        if (scenario == BadPolarity) parameters[{0x607e, 0}] = 0x80;
        if (scenario == ZeroTorque) parameters[{0x6072, 0}] = 0;
        if (scenario == LowSpeed) parameters[{0x607f, 0}] = 1;
        if (scenario == Bounds) parameters[{0x607d, 2}] = 26001;
        original = parameters;
        pos = initial_pos = scenario == Overflow ? INT32_MAX - 10 : 26000;
        fake_time = 1000000000; running = 1; command = previous_command = 0;
        motion_frames = triggers = enable_frames = stop_frames = sdo_writes = 0;
        triggered = stopping = closed = dc_on = injected = mapped = false;
        const int result = test_rotate("mock", &running);
        assert((result == 0) == (scenario == Good));
        assert(closed && triggers <= 1);
        if (scenario == Good) assert(triggers == 1 && pos == initial_pos + 32768);
        if (scenario == WrongMode || scenario == Bounds || scenario == Overflow || scenario >= BadGear ||
            scenario == WriteFailure || scenario == ReadFailure) {
            if (scenario != NoReady) assert(triggers == 0);
        }
        if (scenario != StopWkcLoss && scenario != RestoreFailure) assert(parameters == original);
        if (enable_frames || triggered) { assert(stopping && stop_frames > 0); assert(command == 0); }
        printf("CASE %d PASS\n", n);
    }
    printf("PASS: %d full motion/bus scenarios; bounded target, no reset/re-enable, stop and restoration verified.\n", int(Last) + 1);
}
