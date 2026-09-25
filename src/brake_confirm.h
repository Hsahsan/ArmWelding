#pragma once
#include <signal.h>

// Operator acknowledgement only: there is no brake feedback or PSU control.
// Call before opening the EtherCAT interface, never inside a cyclic loop.
bool confirm_brake_released(const volatile sig_atomic_t *running);
