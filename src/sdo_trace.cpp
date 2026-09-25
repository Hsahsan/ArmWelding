#include "sdo_trace.h"
#include <array>
#include <cstdio>
#include <cstring>
#include "soem/soem.h"

namespace {
struct Trace {
    bool active = false, sent = false, received = false;
    uint16_t index = 0;
    uint8_t sub = 0;
    int expected = 0, tx_wkc = 0, rx_wkc = 0;
    std::array<uint8_t, 16> tx{}, rx{};
};
thread_local Trace trace;
unsigned word(const std::array<uint8_t, 16> &b, unsigned at) {
    return unsigned(b[at]) | (unsigned(b[at + 1]) << 8);
}
void dump(const char *label, const std::array<uint8_t, 16> &bytes) {
    printf("  %s:", label);
    for (auto byte : bytes) printf(" %02X", byte);
    printf("\n");
}
}

extern "C" {
int __real_ecx_mbxsend(ecx_contextt *, uint16, ec_mbxbuft *, int);
int __real_ecx_mbxreceive(ecx_contextt *, uint16, ec_mbxbuft **, int);

int __wrap_ecx_mbxsend(ecx_contextt *ctx, uint16 slave, ec_mbxbuft *mailbox, int timeout)
{
    // SOEM takes ownership on send. Copy before it recycles the mailbox buffer.
    if (trace.active && mailbox) {
        memcpy(trace.tx.data(), *mailbox, trace.tx.size());
        trace.sent = true;
    }
    const int wkc = __real_ecx_mbxsend(ctx, slave, mailbox, timeout);
    if (trace.active) trace.tx_wkc = wkc;
    return wkc;
}

int __wrap_ecx_mbxreceive(ecx_contextt *ctx, uint16 slave, ec_mbxbuft **mailbox, int timeout)
{
    const int wkc = __real_ecx_mbxreceive(ctx, slave, mailbox, timeout);
    // Ignore the initial stale-mailbox drain that precedes a new upload request.
    if (trace.active && trace.sent && wkc > 0 && mailbox && *mailbox) {
        memcpy(trace.rx.data(), **mailbox, trace.rx.size());
        trace.received = true;
        trace.rx_wkc = wkc;
    }
    return wkc;
}
}

void begin_sdo_trace(uint16_t index, uint8_t subindex, int expected_size)
{
    trace = {};
    trace.active = true;
    trace.index = index;
    trace.sub = subindex;
    trace.expected = expected_size;
}

bool finish_sdo_trace(int wkc, int actual_size)
{
    trace.active = false;
    const auto &rx = trace.rx;
    // Scalar requests in this diagnostic have one initiate-upload response.
    // Validate against saved arguments, never a request buffer already recycled.
    const bool match = trace.sent && trace.received && word(trace.tx, 9) == trace.index &&
                       trace.tx[11] == trace.sub && (rx[5] & 0x0f) == ECT_MBXT_COE &&
                       (word(rx, 6) >> 12) == ECT_COES_SDORES &&
                       word(rx, 9) == trace.index && rx[11] == trace.sub &&
                       word(rx, 0) >= 10 && (rx[8] & 0xe0) == 0x40;
    const bool valid = match && wkc > 0 && actual_size == trace.expected;
    printf("SDO %04X:%02X: TX-WKC=%d RX-WKC=%d reply=%04X:%02X cmd=%02X "
           "size=%d/%d counter=%u/%u %s\n", trace.index, trace.sub,
           trace.tx_wkc, trace.rx_wkc, word(rx, 9), rx[11], rx[8], actual_size,
           trace.expected, (trace.tx[5] >> 4) & 7, (rx[5] >> 4) & 7,
           valid ? "OK" : "DITOLAK");
    if (!valid) {
        if (trace.sent) dump("Mailbox TX (16 byte)", trace.tx);
        if (trace.received) dump("Mailbox RX (16 byte)", trace.rx);
        if (!match) printf("  Respons mailbox tidak cocok dengan permintaan SDO.\n");
        else if (actual_size != trace.expected)
            printf("  Index/subindex cocok, tetapi panjang data berbeda; tidak menebak byte yang hilang.\n");
    }
    return valid;
}
