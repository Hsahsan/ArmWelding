#include "lc10e_esi.h"
#include <atomic>

namespace {
ecx_contextt *active_context = nullptr;
std::atomic<int> used_count{0};
bool supply_mapping(ecx_contextt *ctx, uint16 slave, uint32 *outputs, uint32 *inputs)
{
    if (ctx != active_context || !lc10e_esi::matches_slave(ctx, slave)) return false;
    auto &s = ctx->slavelist[slave];
    // Keep mailbox SMs; configure process SMs from ESI, with activation bit 16.
    for (int i = 2; i < EC_MAXSM; ++i) { s.SM[i] = {}; s.SMtype[i] = 0; }
    s.SM[2].StartAddr = htoes(lc10e_esi::output_address);
    s.SM[2].SMlength = htoes(lc10e_esi::output_bytes);
    s.SM[2].SMflags = htoel(lc10e_esi::output_flags);
    s.SMtype[2] = 3;
    s.SM[3].StartAddr = htoes(lc10e_esi::input_address);
    s.SM[3].SMlength = htoes(lc10e_esi::input_bytes);
    s.SM[3].SMflags = htoel(lc10e_esi::input_flags);
    s.SMtype[3] = 4;
    *outputs = lc10e_esi::output_bytes * 8;
    *inputs = lc10e_esi::input_bytes * 8;
    ++used_count;
    return true;
}
}

namespace lc10e_esi {
bool matches_slave(const ecx_contextt *ctx, uint16 slave)
{
    if (!ctx || slave < 1 || slave > ctx->slavecount) return false;
    const auto &s = ctx->slavelist[slave];
    return s.eep_man == vendor && s.eep_id == product && s.eep_rev == revision;
}

bool matches(const ecx_contextt *ctx)
{
    if (!ctx || ctx->slavecount < 1) return false;
    for (int i = 1; i <= ctx->slavecount; ++i) {
        if (!matches_slave(ctx, i)) return false;
    }
    return true;
}

bool begin_mapping(ecx_contextt *ctx)
{
    if (active_context || !matches(ctx)) return false;
    used_count = 0;
    active_context = ctx;
    return true;
}

bool end_mapping()
{
    const int count = used_count.load();
    const bool ok = active_context && count == active_context->slavecount;
    active_context = nullptr;
    return ok;
}
}

// Replace only SOEM's descriptor discovery during this explicit ESI mapping.
// SOEM still constructs/writes its own FMMUs and SMs. Other callers use SOEM.
extern "C" {
int __real_ecx_readPDOmap(ecx_contextt *, uint16, uint32 *, uint32 *);
int __real_ecx_readPDOmapCA(ecx_contextt *, uint16, int, uint32 *, uint32 *);
int __wrap_ecx_readPDOmap(ecx_contextt *ctx, uint16 slave, uint32 *o, uint32 *i)
{
    if (supply_mapping(ctx, slave, o, i)) return 1;
    return __real_ecx_readPDOmap(ctx, slave, o, i);
}
int __wrap_ecx_readPDOmapCA(ecx_contextt *ctx, uint16 slave, int thread, uint32 *o, uint32 *i)
{
    if (supply_mapping(ctx, slave, o, i)) return 1;
    return __real_ecx_readPDOmapCA(ctx, slave, thread, o, i);
}
}
