#pragma once
#include "soem/soem.h"

// Fixed PDO definitions from the supplied LC10E V1.04.xml, revision 0x204.
// tests/verify_lc10e_xml.py checks these constants against that file.
namespace lc10e_esi {
inline constexpr uint32 vendor = 0x766, product = 0x402, revision = 0x204;
inline constexpr uint32 rx[] = {
    0x60400010, 0x607a0020, 0x60b80010, 0x60710010, 0x607f0020, 0x60600008
};
inline constexpr uint32 tx[] = {
    0x603f0010, 0x60410010, 0x60640020, 0x60770010, 0x60f40020,
    0x60b90010, 0x60ba0020, 0x60bc0020, 0x60fd0020
};
inline constexpr uint16 rx_index = 0x1702, tx_index = 0x1b02;
inline constexpr uint16 output_bytes = 15, input_bytes = 28;
inline constexpr uint16 output_address = 0x1200, input_address = 0x1300;
inline constexpr uint32 output_flags = 0x00010064, input_flags = 0x00010020;
bool matches_slave(const ecx_contextt *ctx, uint16 slave);
bool matches(const ecx_contextt *ctx);
// Only used after checking the live assignment/counts. Scope ends after mapping.
bool begin_mapping(ecx_contextt *ctx);
bool end_mapping();
}
