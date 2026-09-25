#pragma once
#include <cstdint>

// Trace a scalar upload through the real SOEM mailbox implementation.
void begin_sdo_trace(uint16_t index, uint8_t subindex, int expected_size);
bool finish_sdo_trace(int wkc, int actual_size);
