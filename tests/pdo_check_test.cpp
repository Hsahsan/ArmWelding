// Simulated bus: no SOEM library, raw sockets, or physical devices are used.
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include <signal.h>
#include <time.h>
#include "soem/soem.h"

int check_pdo(const char *, const volatile sig_atomic_t *, bool);
extern "C" int __wrap_ecx_readPDOmap(ecx_contextt *, uint16, uint32 *, uint32 *);
// Protocol tracing is integration-tested against real SOEM in sdo_trace_test.cpp.
static int expected_sdo_size;
void begin_sdo_trace(uint16_t, uint8_t, int size) { expected_sdo_size = size; }
bool finish_sdo_trace(int wkc, int size) { return wkc > 0 && size == expected_sdo_size; }
namespace {
enum Scenario { Good, BadIdentity, BadMapping, BadSize, BadWkc, Enabled,
                EnableDuringOp, BadDc, NoOp, AssignmentReadFailure, DriveFault, Cancel,
                StateWriteNoAck, PreOpRefused, AlError, Group2, FourEntriesReordered,
                UnknownOutput, DuplicateControl, MissingControl, MissingStatus,
                MappingChanged, InvalidAssignment, TooManyEntries, ReadOnlyExtraInput,
                WrongControlWidth, BadSubindex, NonAligned, MissingTarget, ZeroEntries,
                DuplicateStatus, TruncatedSdo, PreserveReadFailure, EsiGood, EsiRevision, EsiAssignment,
                EsiCount, EsiChangedCount, EsiSize, EsiSmReadback, EsiNotUsed,
                EsiMailbox, EsiBadTotal, EsiBadMailboxLength, EsiBadMailboxOffset,
                EsiBadBits, EsiBadInputOffset, EsiBadMailboxLookup, EsiBadStartbit };
Scenario scenario;
uint16_t mappings[2];
uint8_t counts[2];
int frames, op_frames, writes;
int64_t fake_ns;
bool closed, dc_on, mapped;
ecx_contextt *mock_context;
std::vector<uint32> rx_entries, tx_entries;
std::vector<uint8> expected_output;
int input_status, input_error, input_position, input_bytes;
volatile sig_atomic_t running;
template<typename T> int reply(void *p, int *size, T x) {
    assert(*size >= int(sizeof(x))); memcpy(p, &x, sizeof(x)); *size = sizeof(x); return 1;
}
}

extern "C" {
int clock_gettime(clockid_t, timespec *t) noexcept {
    t->tv_sec = fake_ns / 1000000000; t->tv_nsec = fake_ns % 1000000000; return 0;
}
int clock_nanosleep(clockid_t, int, const timespec *t, timespec *) {
    fake_ns = int64_t(t->tv_sec) * 1000000000 + t->tv_nsec; return 0;
}
int ecx_init(ecx_contextt *ctx, const char *) { mock_context = ctx; return 1; }
void ecx_close(ecx_contextt *ctx) {
    assert(ctx->slavelist[1].state == EC_STATE_INIT);
    assert(!dc_on); closed = true;
}
int ecx_config_init(ecx_contextt *ctx) {
    ctx->slavecount = 1;
    auto &s = ctx->slavelist[1];
    s.eep_man = scenario == BadIdentity ? 123 : 0x766;
    s.eep_rev = scenario == EsiRevision ? 0x205 : 0x204;
    s.eep_id = 0x402; s.hasdc = TRUE; s.state = EC_STATE_INIT;
    s.configadr = 0x1001;
    s.mbx_l = scenario >= EsiMailbox ? 128 : 0;
    return 1;
}
int ecx_readstate(ecx_contextt *) { return 1; }
int ecx_writestate(ecx_contextt *ctx, uint16 slave) {
    const auto wanted = ctx->slavelist[slave].state;
    auto actual = wanted;
    if (scenario == NoOp && wanted == EC_STATE_OPERATIONAL) actual = EC_STATE_SAFE_OP;
    if (scenario == PreOpRefused && wanted == EC_STATE_PRE_OP) actual = EC_STATE_INIT;
    if (scenario == AlError && wanted == EC_STATE_PRE_OP) actual = EC_STATE_PRE_OP | EC_STATE_ERROR;
    ctx->slavelist[1].state = actual;
    return scenario == StateWriteNoAck ? 0 : 1;
}
uint16 ecx_statecheck(ecx_contextt *ctx, uint16, uint16, int) {
    // SOEM returns the low nibble while caching the complete AL status.
    return ctx->slavelist[1].state & 0x0f;
}
boolean ecx_iserror(ecx_contextt *) { return FALSE; }
char *ecx_elist2string(ecx_contextt *) { static char x[] = ""; return x; }
char *ec_ALstatuscode2string(uint16) { static char x[] = "mock"; return x; }

int ecx_SDOread(ecx_contextt *, uint16 slave, uint16 index, uint8 sub, boolean ca,
                int *size, void *p, int) {
    assert(slave == 1 && !ca);
    if (index == 0x6041) return reply(p, size, htoes(uint16(scenario == Enabled ? 4 : 0x40)));
    if (index == 0x603f) return reply(p, size, uint16(0));
    if (index == 0x6064) return reply(p, size, htoel(uint32(123456)));
    if (index == 0x6061) return reply(p, size, int8(8));
    if (index == 0x6060) {
        if (scenario == PreserveReadFailure) return 0;
        return reply(p, size, int8(8));
    }
    if (index == 0x607f) return reply(p, size, htoel(uint32(3000000)));
    if (index == 0x1c12 || index == 0x1c13) {
        int i = index - 0x1c12;
        if (scenario == AssignmentReadFailure) return 0;
        return sub ? reply(p, size, mappings[i]) : reply(p, size, counts[i]);
    }
    if (index == etohs(mappings[0]) || index == etohs(mappings[1])) {
        const auto &entries = index == etohs(mappings[0]) ? rx_entries : tx_entries;
        if (!sub) return reply(p, size, uint8(entries.size() + (scenario == EsiChangedCount && mapped ? 1 : 0)));
        assert(scenario < EsiGood && "ESI mode must not read broken descriptors");
        if (scenario == TruncatedSdo) return reply(p, size, uint16(0));
        uint32 entry = entries.at(sub - 1);
        if (scenario == MappingChanged && mapped) entry ^= 0x10000;
        return reply(p, size, htoel(entry));
    }
    // Inactive 1701h is never assumed to match the manual or selected.
    assert(false); return 0;
}
int ecx_SDOwrite(ecx_contextt *, uint16, uint16, uint8, boolean, int, const void *, int) {
    ++writes;
    assert(false && "Diagnostic must not write any SDO or PDO assignment");
    return 0;
}
int ecx_config_map_group(ecx_contextt *ctx, void *map, uint8) {
    assert(ctx->slavelist[1].PO2SOconfig == nullptr);
    assert(ctx->manualstatechange == 1);
    mapped = true;
    auto &s = ctx->slavelist[1];
    if (scenario >= EsiGood && scenario != EsiNotUsed) {
        uint32 outputs = 0, inputs = 0;
        assert(__wrap_ecx_readPDOmap(ctx, 1, &outputs, &inputs) == 1);
        assert(outputs == 120 && inputs == 224);
    }
    s.outputs = static_cast<uint8 *>(map); s.inputs = s.outputs + expected_output.size();
    s.Obytes = (scenario == BadSize || scenario == EsiSize) ? expected_output.size() + 2 : expected_output.size();
    s.Ibytes = input_bytes;
    s.Obits = s.Obytes * 8; s.Ibits = s.Ibytes * 8;
    ctx->grouplist[0].outputsWKC = 1; ctx->grouplist[0].inputsWKC = 1;
    auto &g = ctx->grouplist[0];
    g.Obytes = expected_output.size(); g.Ibytes = input_bytes;
    g.outputs = s.outputs; g.inputs = s.inputs;
    g.mbxstatuslength = s.mbx_l ? 1 : 0;
    g.mbxstatus = s.outputs + expected_output.size() + input_bytes;
    if (s.mbx_l) { s.mbxstatus = g.mbxstatus; g.mbxstatuslookup[0] = 1; }
    if (scenario == EsiBadMailboxLength) g.mbxstatuslength = 2;
    if (scenario == EsiBadMailboxOffset) --s.mbxstatus;
    if (scenario == EsiBadBits) --s.Ibits;
    if (scenario == EsiBadInputOffset) ++s.inputs;
    if (scenario == EsiBadMailboxLookup) g.mbxstatuslookup[0] = 2;
    if (scenario == EsiBadStartbit) s.Ostartbit = 1;
    return expected_output.size() + input_bytes + (s.mbx_l ? 1 : 0) + (scenario == EsiBadTotal ? 1 : 0);
}
boolean ecx_configdc(ecx_contextt *) { return scenario != BadDc; }
void ecx_dcsync0(ecx_contextt *, uint16, boolean act, uint32 cycle, int32) {
    assert(!act || cycle == 1000000); dc_on = act;
}
int ecx_FPRD(ecx_portt *, uint16, uint16 ado, uint16 size, void *p, int) {
    if (ado == 0x0810 || ado == 0x0818) {
        assert(size == sizeof(ec_smt));
        auto sm = mock_context->slavelist[1].SM[ado == 0x0810 ? 2 : 3];
        if (scenario == EsiSmReadback) sm.SMlength = htoes(26);
        memcpy(p, &sm, sizeof(sm));
    }
    else if (ado == ECT_REG_STADR || ado == ECT_REG_ALCTL || ado == ECT_REG_ALSTAT || ado == ECT_REG_ALSTATCODE) {
        assert(size == 2);
        const uint16 value = htoes(ado == ECT_REG_STADR ? 0x1001 :
                                  ado == ECT_REG_ALSTAT ? (scenario == AlError ? 0x12 : 1) : 0);
        memcpy(p, &value, 2);
    }
    else if (ado == ECT_REG_DCSYNCACT) { assert(size == 1); *static_cast<uint8 *>(p) = 3; }
    else { assert(ado == ECT_REG_DCCYCLE0 && size == 4); uint32 x = htoel(1000000); memcpy(p, &x, 4); }
    return 1;
}
int __real_ecx_readPDOmap(ecx_contextt *, uint16, uint32 *, uint32 *) { assert(false); return 0; }
int __real_ecx_readPDOmapCA(ecx_contextt *, uint16, int, uint32 *, uint32 *) { assert(false); return 0; }
int ecx_send_processdata(ecx_contextt *ctx) {
    const auto *out = ctx->slavelist[1].outputs;
    // Explicit expected bytes are independent of the production offset parser.
    assert(memcmp(out, expected_output.data(), expected_output.size()) == 0);
    ++frames;
    if (ctx->slavelist[1].state == EC_STATE_OPERATIONAL) ++op_frames;
    return 1;
}
int ecx_receive_processdata(ecx_contextt *ctx, int) {
    auto *in = ctx->slavelist[1].inputs;
    const uint32 pos = htoel(123456); memcpy(in + input_position, &pos, 4);
    uint16 sw = 0x40;
    if (scenario == EnableDuringOp && op_frames) sw = 4;
    if (scenario == DriveFault && op_frames) sw = 8;
    sw = htoes(sw); memcpy(in + input_status, &sw, 2);
    const uint16 error = 0; memcpy(in + input_error, &error, 2);
    ctx->DCtime += 1000000;
    if (scenario == Cancel && op_frames) running = 0;
    return scenario == BadWkc && op_frames ? 1 : 3;
}
}

int main() {
    for (int c = Good; c <= EsiBadStartbit; ++c) {
        scenario = static_cast<Scenario>(c);
        mappings[0] = htoes(0x1701); mappings[1] = htoes(0x1b01);
        counts[0] = counts[1] = 1;
        // Manual group 1 fixture; little-endian position 123456 = 0x0001e240.
        rx_entries = {0x60400010, 0x607a0020, 0x60b80010};
        tx_entries = {0x603f0010, 0x60410010, 0x60640020, 0x60770010,
                      0x60f40020, 0x60b90010, 0x60ba0020, 0x60fd0020};
        expected_output = {0, 0, 0x40, 0xe2, 1, 0, 0, 0};
        input_error = 0; input_status = 2; input_position = 4; input_bytes = 24;
        if (scenario == Group2 || scenario == PreserveReadFailure) {
            // Active assignment reported by the user; entries here are the MANUAL
            // fixture, not a claim about firmware entries not yet read on hardware.
            mappings[0] = htoes(0x1702); mappings[1] = htoes(0x1b02);
            rx_entries = {0x60400010, 0x607a0020, 0x60ff0020, 0x60710010,
                          0x60600008, 0x60b80010, 0x607f0020};
            tx_entries = {0x603f0010, 0x60410010, 0x60640020, 0x60770010,
                          0x60610008, 0x60b90010, 0x60ba0020, 0x60bc0020, 0x60fd0020};
            expected_output = {0, 0, 0x40, 0xe2, 1, 0, 0, 0, 0, 0, 0, 0, 8,
                               0, 0, 0xc0, 0xc6, 0x2d, 0};
            input_bytes = 25;
        }
        if (scenario >= EsiGood) {
            mappings[0] = htoes(0x1702); mappings[1] = htoes(0x1b02);
            rx_entries = {0x60400010, 0x607a0020, 0x60b80010, 0x60710010, 0x607f0020, 0x60600008};
            tx_entries = {0x603f0010, 0x60410010, 0x60640020, 0x60770010, 0x60f40020,
                          0x60b90010, 0x60ba0020, 0x60bc0020, 0x60fd0020};
            expected_output = {0, 0, 0x40, 0xe2, 1, 0, 0, 0, 0, 0, 0xc0, 0xc6, 0x2d, 0, 8};
            input_bytes = 28;
            if (scenario == EsiAssignment) mappings[0] = htoes(0x1701);
            if (scenario == EsiCount) rx_entries.pop_back();
        }
        if (scenario == FourEntriesReordered) {
            // Synthetic four-entry firmware variant: mode and offsets differ.
            rx_entries = {0x60600008, 0x60b80010, 0x607a0020, 0x60400010};
            tx_entries = {0x60640020, 0x60610008, 0x60410010, 0x603f0010};
            expected_output = {8, 0, 0, 0x40, 0xe2, 1, 0, 0, 0};
            input_position = 0; input_status = 5; input_error = 7; input_bytes = 9;
        }
        if (scenario == BadMapping) tx_entries[0] = 0x603f0011;
        if (scenario == UnknownOutput) rx_entries.push_back(0x20000010);
        if (scenario == DuplicateControl) rx_entries.push_back(0x60400010);
        if (scenario == MissingControl) rx_entries.erase(rx_entries.begin());
        if (scenario == MissingTarget) rx_entries.erase(rx_entries.begin() + 1);
        if (scenario == MissingStatus) tx_entries.erase(tx_entries.begin() + 1);
        if (scenario == DuplicateStatus) tx_entries.push_back(0x60410010);
        if (scenario == InvalidAssignment) counts[0] = 2;
        if (scenario == TooManyEntries) rx_entries.resize(33, 0x00000008);
        if (scenario == ZeroEntries) rx_entries.clear();
        if (scenario == WrongControlWidth) rx_entries[0] = 0x60400020;
        if (scenario == BadSubindex) rx_entries[0] = 0x60400110;
        if (scenario == NonAligned) rx_entries.insert(rx_entries.begin(), 0x00000001);
        if (scenario == ReadOnlyExtraInput) { tx_entries.push_back(0x20050010); input_bytes += 2; }
        frames = op_frames = writes = 0; fake_ns = 1000000000;
        closed = dc_on = mapped = false; running = 1;
        int result = check_pdo("mock", &running, scenario >= EsiGood);
        const bool success = scenario == Good || scenario == StateWriteNoAck ||
                             scenario == Group2 || scenario == FourEntriesReordered ||
                             scenario == ReadOnlyExtraInput || scenario == EsiGood || scenario == EsiMailbox;
        assert((result == 0) == success);
        assert(closed && writes == 0);
        assert(etohs(mappings[0]) == (scenario == Group2 || scenario == PreserveReadFailure || (scenario >= EsiGood && scenario != EsiAssignment) ? 0x1702 : 0x1701));
        assert(etohs(mappings[1]) == (scenario == Group2 || scenario == PreserveReadFailure || scenario >= EsiGood ? 0x1b02 : 0x1b01));
        assert(counts[0] == (scenario == InvalidAssignment ? 2 : 1) && counts[1] == 1);
        if (success) assert(op_frames == 5001);
        else if (scenario != BadWkc && scenario != EnableDuringOp && scenario != NoOp &&
                 scenario != DriveFault && scenario != Cancel) assert(frames == 0);
    }
    printf("PASS: %d simulated scenarios; active mappings, all output bytes, and no SDO writes verified.\n",
           int(EsiBadStartbit) + 1);
}
