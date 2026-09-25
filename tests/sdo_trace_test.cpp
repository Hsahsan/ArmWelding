// Execute the installed SOEM ecx_SDOread with simulated mailbox transport.
// No NIC initialization, sockets, SDO writes, or physical devices are used.
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include "soem/soem.h"
#include "../src/sdo_trace.h"

namespace {
enum Case { Byte1, Byte2, Byte4, ShortMapping, WrongSubindex, WrongIndex,
            SdoAbort, TruncatedHeader, NoReply, SendFailure };
Case test_case;
bool pending;
int sends, receives;
std::array<uint8_t, 16> request;
}

extern "C" int __wrap_ecx_FPWR(ecx_portt *, uint16, uint16 address, uint16 size, void *data, int)
{
    assert(address == 0x1000 && size == 128);
    ++sends;
    memcpy(request.data(), data, request.size());
    pending = test_case != SendFailure;
    return pending ? 1 : 0;
}

extern "C" int __wrap_ecx_FPRD(ecx_portt *, uint16, uint16 address, uint16 size, void *data, int)
{
    auto *bytes = static_cast<uint8_t *>(data);
    if (address == ECT_REG_SM1STAT) {
        assert(size == 1);
        bytes[0] = pending && test_case != NoReply ? 8 : 0;
        return 1;
    }
    if (address == ECT_REG_SM0STAT) {
        assert(size == 1); bytes[0] = 0; return 1;
    }
    assert(address == 0x1100 && size == 128 && pending);
    ++receives;
    pending = false;
    memset(bytes, 0, size);
    memcpy(bytes, request.data(), request.size());
    bytes[6] = 0; bytes[7] = 0x30; // CoE SDO response.
    bytes[8] = test_case == Byte1 ? 0x4f :
               (test_case == Byte2 || test_case == ShortMapping) ? 0x4b : 0x43;
    if (test_case == WrongSubindex) bytes[11] ^= 1;
    if (test_case == WrongIndex) bytes[10] ^= 1;
    if (test_case == SdoAbort) bytes[8] = 0x80;
    if (test_case == TruncatedHeader) bytes[0] = 6;
    bytes[12] = 0x10; bytes[13] = 0; bytes[14] = 0x40; bytes[15] = 0x60;
    return 1;
}

int main()
{
    for (int i = Byte1; i <= SendFailure; ++i) {
        test_case = static_cast<Case>(i);
        auto ctx = std::make_unique<ecx_contextt>();
        ecx_initmbxpool(ctx.get());
        assert(ctx->mbxpool.mbxmutex);
        ctx->slavecount = 1;
        ctx->slavelist[1].state = EC_STATE_PRE_OP;
        ctx->slavelist[1].configadr = 0x1001;
        ctx->slavelist[1].mbx_l = ctx->slavelist[1].mbx_rl = 128;
        ctx->slavelist[1].mbx_wo = 0x1000;
        ctx->slavelist[1].mbx_ro = 0x1100;
        pending = false; sends = receives = 0;
        const int expected = test_case == Byte1 ? 1 : test_case == Byte2 ? 2 : 4;
        int size = expected;
        uint32 value = 0;
        begin_sdo_trace(0x1702, 1, expected);
        const int wkc = ecx_SDOread(ctx.get(), 1, 0x1702, 1, FALSE, &size, &value, 1000);
        const bool valid = finish_sdo_trace(wkc, size);
        assert(valid == (i <= Byte4));
        assert(sends == (test_case == SendFailure ? 2 : 1));
        assert(receives == (test_case == SendFailure || test_case == NoReply ? 0 : 1));
        assert(ctx->mbxpool.listcount == EC_MBXPOOLSIZE);
        if (test_case == WrongSubindex) {
            // Reproduces the missing subindex check in the installed SOEM v2.0.0.
            assert(wkc > 0 && size == 4);
            assert(!valid);
        }
        if (test_case == ShortMapping) assert(size == 2 && !valid);
        osal_mutex_destroy(ctx->mbxpool.mbxmutex);
    }
    puts("PASS: 10 real-SOEM SDO parser scenarios; mismatched/short replies rejected.");
}
