#pragma once
#include "soem/soem.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>

// SOEM 2.0 appends mailbox status bytes after process inputs. They are not PDOs.
// Supports 1 or more slaves of identical PDO sizing in non-overlapped group zero.
inline bool validate_pdo_iomap(const ecx_contextt *ctx, const uint8 *buffer,
                               size_t capacity, int output_bytes, int input_bytes,
                               int mapped_bytes)
{
    const int n_slaves = ctx->slavecount;
    if (n_slaves < 1 || output_bytes <= 0 || input_bytes <= 0 ||
        output_bytes > 256 || input_bytes > 256)
        return false;

    const auto &g = ctx->grouplist[0];
    const int total_output_bytes = n_slaves * output_bytes;
    const int total_input_bytes = n_slaves * input_bytes;
    const int pdo_bytes = total_output_bytes + total_input_bytes;

    int total_mailbox_bytes = 0;
    for (int i = 1; i <= n_slaves; ++i) {
        if (ctx->slavelist[i].mbx_l) ++total_mailbox_bytes;
    }
    const int expected_total = pdo_bytes + total_mailbox_bytes;
    if (size_t(expected_total) > capacity) return false;

    const auto offset = [&](const uint8 *p) -> long long {
        const uintptr_t start = reinterpret_cast<uintptr_t>(buffer);
        const uintptr_t address = reinterpret_cast<uintptr_t>(p);
        if (!p || address < start || address - start > capacity) return -1;
        return static_cast<long long>(address - start);
    };

    printf("IOmap: total=%d expected=%d; PDO=%d mailbox-status=%d expected=%d\n",
           mapped_bytes, expected_total, pdo_bytes, g.mbxstatuslength, total_mailbox_bytes);

    for (int i = 1; i <= n_slaves; ++i) {
        const auto &s = ctx->slavelist[i];
        printf("  Slave %d: O/I bytes=%u/%u bits=%u/%u startbit=%u/%u offset=%lld/%lld\n",
               i, s.Obytes, s.Ibytes, s.Obits, s.Ibits, s.Ostartbit, s.Istartbit,
               offset(s.outputs), offset(s.inputs));
    }
    printf("  Group: O/I bytes=%u/%u offset=%lld/%lld; mailbox offset group=%lld\n",
           g.Obytes, g.Ibytes, offset(g.outputs), offset(g.inputs),
           offset(g.mbxstatus));

    bool valid = (!ctx->overlappedMode && g.logstartaddr == 0 &&
                  mapped_bytes == expected_total &&
                  g.Obytes == unsigned(total_output_bytes) &&
                  g.Ibytes == unsigned(total_input_bytes) &&
                  g.outputs == buffer &&
                  g.inputs == buffer + total_output_bytes &&
                  g.mbxstatuslength == total_mailbox_bytes);

    if (total_mailbox_bytes > 0) {
        valid = valid && (g.mbxstatus == buffer + pdo_bytes);
    }

    int cur_mbx = 0;
    for (int i = 1; i <= n_slaves; ++i) {
        const auto &s = ctx->slavelist[i];
        const bool s_valid = (s.Obytes == unsigned(output_bytes) &&
                              s.Ibytes == unsigned(input_bytes) &&
                              s.Obits == output_bytes * 8 &&
                              s.Ibits == input_bytes * 8 &&
                              s.Ostartbit == 0 && s.Istartbit == 0 &&
                              s.outputs == buffer + (i - 1) * output_bytes &&
                              s.inputs == buffer + total_output_bytes + (i - 1) * input_bytes);
        if (!s_valid) valid = false;

        if (s.mbx_l) {
            const bool mbx_valid = (s.mbxstatus == g.mbxstatus + cur_mbx &&
                                    g.mbxstatuslookup[cur_mbx] == i);
            if (!mbx_valid) valid = false;
            ++cur_mbx;
        }
    }

    if (!valid) printf("Ukuran/offset PDO atau status mailbox SOEM tidak sesuai; dibatalkan.\n");
    return valid;
}
