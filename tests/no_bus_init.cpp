// Link with --wrap=ecx_init to exercise real CLI confirmation without any NIC.
#include "soem/soem.h"
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
extern "C" int __wrap_ecx_init(ecx_contextt *, const char *)
{
    puts("TEST_BUS_INIT_REACHED_NO_SOCKET");
    if (std::getenv("LC_TEST_PAUSE_INIT")) usleep(1500000);
    return 0;
}
