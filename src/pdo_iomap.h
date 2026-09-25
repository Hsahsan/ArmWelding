#pragma once
#include "soem/soem.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

// SOEM 2.0 appends mailbox status bytes after process inputs. They are not PDOs.
// This diagnostic accepts exactly one slave in non-overlapped group zero.
inline bool validate_pdo_iomap(const ecx_contextt *ctx, const uint8 *buffer,
                               size_t capacity, int output_bytes, int input_bytes,
                               int mapped_bytes)
{
    const auto &s = ctx->slavelist[1];
    const auto &g = ctx->grouplist[0];
    if (output_bytes <= 0 || input_bytes <= 0 || output_bytes > 256 || input_bytes > 256)
        return false;
    const int pdo_bytes = output_bytes + input_bytes;
    const int mailbox_bytes = s.mbx_l ? 1 : 0;
    const int expected_total = pdo_bytes + mailbox_bytes;
    if (size_t(expected_total) > capacity) return false;
    const auto offset = [&](const uint8 *p) -> long long {
        const uintptr_t start = reinterpret_cast<uintptr_t>(buffer);
        const uintptr_t address = reinterpret_cast<uintptr_t>(p);
        if (!p || address < start || address - start > capacity) return -1;
        return static_cast<long long>(address - start);
    };
    printf("IOmap: total=%d expected=%d; PDO=%d mailbox-status=%d expected=%d\n",
           mapped_bytes, expected_total, pdo_bytes, g.mbxstatuslength, mailbox_bytes);
    printf("  Slave: O/I bytes=%u/%u bits=%u/%u startbit=%u/%u offset=%lld/%lld\n",
           s.Obytes, s.Ibytes, s.Obits, s.Ibits, s.Ostartbit, s.Istartbit,
           offset(s.outputs), offset(s.inputs));
    printf("  Group: O/I bytes=%u/%u offset=%lld/%lld; mailbox offset group/slave=%lld/%lld\n",
           g.Obytes, g.Ibytes, offset(g.outputs), offset(g.inputs),
           offset(g.mbxstatus), offset(s.mbxstatus));
    const bool valid = ctx->slavecount == 1 && !ctx->overlappedMode && g.logstartaddr == 0 &&
        mapped_bytes == expected_total &&
        s.Obytes == unsigned(output_bytes) && s.Ibytes == unsigned(input_bytes) &&
        s.Obits == output_bytes * 8 && s.Ibits == input_bytes * 8 &&
        s.outputs == buffer && s.inputs == buffer + output_bytes &&
        s.Ostartbit == 0 && s.Istartbit == 0 &&
        g.Obytes == s.Obytes && g.Ibytes == s.Ibytes &&
        g.outputs == s.outputs && g.inputs == s.inputs &&
        g.mbxstatuslength == mailbox_bytes &&
        (!mailbox_bytes || (g.mbxstatus == buffer + pdo_bytes &&
                            s.mbxstatus == g.mbxstatus && g.mbxstatuslookup[0] == 1));
    if (!valid) printf("Ukuran/offset PDO atau status mailbox SOEM tidak sesuai; dibatalkan.\n");
    return valid;
}
