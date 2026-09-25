#ifndef _ETHERCAT_H_
#define _ETHERCAT_H_

#include "soem/soem.h"

#ifdef __cplusplus
extern "C" {
#endif

// Deklarasi context global default SOEM
extern ecx_contextt ecx_context;

// Mapping fungsi dan makro standar SOEM agar langsung mengenali ec_init, dkk.
#define ec_slave            ecx_context.slavelist
#define ec_slavecount       ecx_context.slavecount
#define ec_childcount       ecx_context.slavecount
#define ec_grouplist        ecx_context.grouplist
#define ec_dc32             ecx_context.DCtime

#define ec_init(ifname)                     ecx_init(&ecx_context, ifname)
#define ec_close()                          ecx_close(&ecx_context)
#define ec_config_init(req)                 ecx_config_init(&ecx_context)
#define ec_config_map(pIOmap)               ecx_config_map_group(&ecx_context, pIOmap, 0)
#define ec_statecheck(slave, state, timeout) ecx_statecheck(&ecx_context, slave, state, timeout)
#define ec_send_processdata()               ecx_send_processdata(&ecx_context)
#define ec_receive_processdata(timeout)     ecx_receive_processdata(&ecx_context, timeout)

#ifdef __cplusplus
}
#endif

#endif