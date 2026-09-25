// Exercise the installed SOEM mapper, replacing only bus/state/EEPROM access.
#include "../src/lc10e_esi.h"
#include "../src/pdo_iomap.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <initializer_list>

static unsigned char registers[0x1000];
extern "C" {
uint16 __wrap_ecx_statecheck(ecx_contextt *ctx, uint16 slave, uint16 wanted, int)
{
    ctx->slavelist[slave].state = wanted;
    return wanted;
}
int __wrap_ecx_eeprom2pdi(ecx_contextt *, uint16) { return 1; }
int __wrap_ecx_FPWR(ecx_portt *, uint16, uint16 address, uint16 size, void *data, int)
{
    assert(address + size <= sizeof(registers));
    memcpy(registers + address, data, size);
    return 1;
}
int __wrap_ecx_FPRD(ecx_portt *, uint16, uint16 address, uint16 size, void *data, int)
{
    assert(address + size <= sizeof(registers));
    memcpy(data, registers + address, size);
    return 1;
}
int __wrap_ecx_SDOread(ecx_contextt *, uint16, uint16, uint8, boolean, int *, void *, int)
{
    assert(false && "ESI mapper must not rediscover PDO descriptors through SDO");
    return 0;
}
int __wrap_ecx_SDOwrite(ecx_contextt *, uint16, uint16, uint8, boolean, int, const void *, int)
{
    assert(false && "No SDO writes allowed");
    return 0;
}
}

int main()
{
    for (bool mailbox : {false, true}) for (bool ca : {false, true}) {
        auto ctx = std::make_unique<ecx_contextt>();
        ctx->slavecount = 1;
        ctx->manualstatechange = 1;
        auto &s = ctx->slavelist[1];
        s.eep_man = 0x766; s.eep_id = 0x402; s.eep_rev = 0x204;
        s.configadr = 0x1001; s.state = EC_STATE_PRE_OP;
        s.mbx_proto = ECT_MBXPROT_COE;
        s.mbx_l = s.mbx_rl = mailbox ? 128 : 0;
        s.mbx_wo = 0x1000; s.mbx_ro = 0x1100;
        s.CoEdetails = ca ? ECT_COEDET_SDOCA : 0;
        s.SM[0].StartAddr = htoes(0x1000); s.SM[0].SMlength = htoes(128);
        s.SM[1].StartAddr = htoes(0x1100); s.SM[1].SMlength = htoes(128);
        s.SMtype[0] = 1; s.SMtype[1] = 2;
        // Deliberately incorrect cached lengths must be replaced by the profile.
        s.SM[2].StartAddr = htoes(0x1200); s.SM[2].SMlength = htoes(26);
        s.SM[3].StartAddr = htoes(0x1300); s.SM[3].SMlength = htoes(24);
        unsigned char iomap[256]{};
        assert(lc10e_esi::begin_mapping(ctx.get()));
        const int size = ecx_config_map_group(ctx.get(), iomap, 0);
        assert(lc10e_esi::end_mapping());
        assert(size == (mailbox ? 44 : 43) && s.Obytes == 15 && s.Ibytes == 28);
        assert(validate_pdo_iomap(ctx.get(), iomap, sizeof(iomap), 15, 28, size));
        assert(ctx->grouplist[0].mbxstatuslength == (mailbox ? 1 : 0));
        if (mailbox) {
            assert(s.mbxstatus == iomap + 43);
            assert(s.FMMU[2].FMMUtype == 1 && s.FMMU[2].FMMUactive == 1);
            assert(etohs(s.FMMU[2].LogLength) == 1);
            assert(etohl(s.FMMU[2].LogStart) == 43);
            assert(etohs(s.FMMU[2].PhysStart) == ECT_REG_SM1STAT);
        }
        assert(s.Obits == 120 && s.Ibits == 224);
        assert(s.outputs == iomap && s.inputs == iomap + 15);
        assert(s.Ostartbit == 0 && s.Istartbit == 0);
        assert(ctx->grouplist[0].outputsWKC == 1 && ctx->grouplist[0].inputsWKC == 1);
        assert(registers[0x812] == 15 && registers[0x814] == 0x64);
        assert(registers[0x81a] == 28 && registers[0x81c] == 0x20);
        assert(registers[0x816] == 1 && registers[0x81e] == 1);
        assert(s.FMMU[0].FMMUtype == 2 && s.FMMU[1].FMMUtype == 1);
        assert(etohs(s.FMMU[0].LogLength) == 15 && etohs(s.FMMU[1].LogLength) == 28);
        assert(etohs(s.FMMU[0].PhysStart) == 0x1200 && etohs(s.FMMU[1].PhysStart) == 0x1300);
        assert(etohl(s.FMMU[1].LogStart) == 15);
    }
    puts("PASS: 4 installed SOEM mapper cases; with/without mailbox, standard/CA, production IOmap validator, 15/28-byte PDO, WKC=3.");
}
