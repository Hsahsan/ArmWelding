/**
 * EtherCAT Master - LICHUAN LC-E Series Servo Drive
 * ===================================================
 * Library : SOEM (Simple Open EtherCAT Master)
 * Mode    : CSP - Cyclic Synchronous Position (6060h = 8)
 * PDO     : Actual PDO Mapping from Hardware (hardcoded by SOEM)
 *             RxPDO 1702h (master->servo) : 6040h + 607Ah + 60B8h + padding (26 byte)
 *             TxPDO 1B02h (servo->master) : 603Fh + 6041h + 6064h + 6077h + 60F4h +
 *                                           60B9h + 60BAh + 60BCh                (24 byte)
 *
 * State machine CiA402 (manual Chapter 7.1):
 *   Power on -> Init -> Pre-Op -> Safe-Op -> Operational
 *   6040h: 0x06 (Shutdown) -> 0x07 (Switch On) -> 0x0F (Enable Operation)
 *
 * Penggunaan: sudo ./ethercat_servo <interface>
 *   Contoh  : sudo ./ethercat_servo enp2s0
 *
 * Referensi: Lihat QUICK_START.md untuk mapping aktual dari hardware
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "soem/ethercat.h"
#include "soem/soem.h"
#include "soem/ec_main.h"
#include "soem/ec_config.h"
#include "soem/ec_coe.h"
#include "soem/ec_dc.h"
#include "osal/osal.h"
#include "brake_confirm.h"

/* ───────────────────────────────────────────────────────────────
 * Konstanta CiA402 - Control Word (6040h) - manual Chapter 7.1.1
 * ─────────────────────────────────────────────────────────────── */
#define CW_SHUTDOWN          0x0006u  /* bit1=1, bit2=1: Servo fault free -> Servo ready    */
#define CW_SWITCH_ON         0x0007u  /* bit0=1: Servo ready -> Waiting enable               */
#define CW_ENABLE_OPERATION  0x000Fu  /* bit3=1: Waiting enable -> Servo running             */
#define CW_DISABLE_VOLTAGE   0x0000u  /* bit1=0: kembali ke Servo fault free                 */
#define CW_QUICK_STOP        0x0002u  /* bit2=0: Quick stop                                  */
#define CW_FAULT_RESET       0x0080u  /* bit7 rising edge: reset fault                       */

/* Status Word (6041h) bitmask - manual Chapter 7.1.2 */
#define SW_READY_TO_SWITCH_ON  (1u << 0)
#define SW_SWITCHED_ON         (1u << 1)
#define SW_OPERATION_ENABLED   (1u << 2)
#define SW_FAULT               (1u << 3)
#define SW_VOLTAGE_ENABLED     (1u << 4)
#define SW_QUICK_STOP_ACTIVE   (1u << 5)  /* 0 = aktif, 1 = tidak aktif */
#define SW_SWITCH_ON_DISABLED  (1u << 6)
#define SW_WARNING             (1u << 7)

/* ───────────────────────────────────────────────────────────────
 * Struktur PDO - Actual mapping from hardware (0x1702/0x1B02)
 * Berdasarkan hasil --check-pdo, servo menggunakan hardcoded layout:
 *
 * RxPDO 1702h - Output (master -> servo), 8 objects, total 26 byte:
 *   6040h UINT16 (2) + 607Ah INT32 (4) + 60B8h UINT16 (2) + 
 *   padding 0000h INT32 (4) + padding 0000h INT32 (4) +
 *   padding 0000h INT32 (4) + padding 0000h INT32 (4) +
 *   padding 0000h UINT16 (2) = 26 byte
 *
 * TxPDO 1B02h - Input (servo -> master), 8 objects, total 24 byte:
 *   603Fh UINT16 (2) + 6041h UINT16 (2) + 6064h INT32  (4) +
 *   6077h INT16  (2) + 60F4h INT32  (4) + 60B9h UINT16 (2) +
 *   60BAh INT32  (4) + 60BCh INT32  (4) = 24 byte
 *
 * Lihat QUICK_START.md untuk penjelasan lengkap mapping aktual
 * ─────────────────────────────────────────────────────────────── */
#pragma pack(push, 1)

typedef struct {
    uint16_t control_word;          /* 6040h - Control Word (state machine)      */
    int32_t  target_position;       /* 607Ah - Target Position (pulse/encoder)   */
    uint16_t touch_probe_function;  /* 60B8h - Touch Probe Function              */
    int32_t  padding1;              /* 0000h - Padding                           */
    int32_t  padding2;              /* 0000h - Padding                           */
    int32_t  padding3;              /* 0000h - Padding                           */
    int32_t  padding4;              /* 0000h - Padding                           */
    uint16_t padding5;              /* 0000h - Padding                           */
} OutputPDO;                        /* Total: 26 bytes                           */

typedef struct {
    uint16_t error_code;              /* 603Fh - Error Code                      */
    uint16_t status_word;             /* 6041h - Status Word                     */
    int32_t  position_actual_value;   /* 6064h - Position Actual Value           */
    int16_t  torque_actual_value;     /* 6077h - Torque Actual Value (0.1%)      */
    int32_t  position_deviation;      /* 60F4h - Following Error (Position Dev)  */
    uint16_t touch_probe_status;      /* 60B9h - Touch Probe Status              */
    int32_t  touch_probe_pos1_value;  /* 60BAh - Touch Probe Pos1 Value          */
    int32_t  touch_probe_pos2_value;  /* 60BCh - Touch Probe Pos2 Value          */
} InputPDO;                           /* Total: 24 bytes                         */

#pragma pack(pop)

/* ───────────────────────────────────────────────────────────────
 * Definisi ecx_context global
 *
 * ethercat.h mendeklarasikan "extern ecx_contextt ecx_context;"
 * sehingga kita harus mendefinisikannya di sini.
 * Semua field (slavelist, grouplist, esibuf, dll.) sudah embedded
 * langsung di dalam struct — tidak perlu buffer terpisah.
 * ─────────────────────────────────────────────────────────────── */
ecx_contextt ecx_context;

static void init_context(void)
{
    memset(&ecx_context, 0, sizeof(ecx_context));
}

/* ───────────────────────────────────────────────────────────────
 * Variabel global lain
 * ─────────────────────────────────────────────────────────────── */
static char      IOmap[4096];
static volatile sig_atomic_t g_running = 1;

int check_pdo(const char *ifname, const volatile sig_atomic_t *running, bool use_esi);
int test_rotate(const char *ifname, const volatile sig_atomic_t *running);

/* ───────────────────────────────────────────────────────────────
 * Signal handler - Ctrl+C untuk graceful shutdown
 * ─────────────────────────────────────────────────────────────── */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* ───────────────────────────────────────────────────────────────
 * PO2SOconfig callback
 * Dipanggil SOEM saat slave di Pre-Op sebelum pindah ke Safe-Op.
 * Tulis SDO konfigurasi yang diperlukan sebelum IOmap di-map.
 * ─────────────────────────────────────────────────────────────── */
static int lichuan_po2so_config(ecx_contextt *context, uint16 slave)
{
    int      sz;
    int      ret;
    uint8_t  u8val;
    uint16_t u16val;
    uint32_t u32val;

    printf("  [Slave %d] PO2SOconfig: Menulis SDO konfigurasi...\n", slave);

    /* 1. Mode operasi: CSP = 8 (manual Chapter 7.2.1, object 6060h) */
    u8val = 8;
    sz    = sizeof(u8val);
    ret   = ecx_SDOwrite(context, slave, 0x6060, 0x00, FALSE, sz, &u8val, EC_TIMEOUTRXM);
    if (ret <= 0)
        printf("  [Slave %d] WARN: Gagal set mode CSP (6060h)\n", slave);
    else
        printf("  [Slave %d] Mode: CSP (6060h = 8)\n", slave);

    /* 2. Gear ratio 1:1 - sesuaikan dengan mekanik sistem
     *    6091h-01h = motor resolution (numerator)
     *    6091h-02h = axis  resolution (denominator)
     *    Rasio = 6091-01h / 6091-02h  */
    u32val = 1;
    sz     = sizeof(u32val);
    ecx_SDOwrite(context, slave, 0x6091, 0x01, FALSE, sz, &u32val, EC_TIMEOUTRXM);
    ecx_SDOwrite(context, slave, 0x6091, 0x02, FALSE, sz, &u32val, EC_TIMEOUTRXM);
    printf("  [Slave %d] Gear ratio: 1/1\n", slave);

    /* 3. Batas kecepatan maksimum (607Fh) - command unit/s
     *    Sesuaikan dengan rating motor. Default 10485760.        */
    u32val = 3000000;
    sz     = sizeof(u32val);
    ecx_SDOwrite(context, slave, 0x607F, 0x00, FALSE, sz, &u32val, EC_TIMEOUTRXM);

    /* 4. Torque limit maksimum (6072h) - satuan 0.1%, 1000 = 100% */
    u16val = 1000;
    sz     = sizeof(u16val);
    ecx_SDOwrite(context, slave, 0x6072, 0x00, FALSE, sz, &u16val, EC_TIMEOUTRXM);

    /* 5. Threshold deviasi posisi berlebihan (6065h) - command unit
     *    Jika deviasi melebihi nilai ini servo akan fault Er.B00   */
    u32val = 100000;
    sz     = sizeof(u32val);
    ecx_SDOwrite(context, slave, 0x6065, 0x00, FALSE, sz, &u32val, EC_TIMEOUTRXM);

    printf("  [Slave %d] Konfigurasi SDO selesai.\n", slave);
    return 0;
}

/* ───────────────────────────────────────────────────────────────
 * Helper: decode dan cetak status word
 * ─────────────────────────────────────────────────────────────── */
static void print_status(uint16_t sw, uint16_t err)
{
    printf("  SW=0x%04X ERR=0x%04X |", sw, err);
    if (sw & SW_FAULT)              printf(" FAULT");
    if (sw & SW_WARNING)            printf(" WARN");
    if (sw & SW_OPERATION_ENABLED)  printf(" OP_ENABLED");
    else if (sw & SW_SWITCHED_ON)   printf(" SWITCHED_ON");
    else if (sw & SW_READY_TO_SWITCH_ON) printf(" READY");
    else                            printf(" DISABLED");
    printf("\n");
}

/* ───────────────────────────────────────────────────────────────
 * Fungsi utama EtherCAT
 * ─────────────────────────────────────────────────────────────── */
static void run_ethercat(const char *ifname)
{
    int        wkc;
    int        slave_idx     = 1;   /* Asumsi 1 servo pada posisi slave ke-1 */
    OutputPDO *output        = NULL;
    InputPDO  *input         = NULL;
    int        cia_step      = 0;
    int        enabled       = 0;
    int32_t    home_position = 0;
    int        cycle         = 0;
    int        map_size      = 0;

    /* ── Langkah 1: Inisialisasi context & buka raw socket ke interface ── */
    init_context();

    if (!ec_init(ifname)) {
        printf("ec_init gagal pada '%s'.\n"
               "Pastikan interface benar dan program dijalankan sebagai root.\n", ifname);
        return;
    }
    printf("ec_init pada '%s' berhasil.\n", ifname);

    /* ── Langkah 2: Scan bus EtherCAT, baca EEPROM semua slave ── */
    if (ec_config_init(FALSE) <= 0) {
        printf("Tidak ada slave EtherCAT ditemukan!\n");
        goto cleanup;
    }
    printf("%d slave ditemukan:\n", ec_slavecount);
    for (int i = 1; i <= ec_slavecount; i++) {
        printf("  Slave %d: %-20s  VendorID=0x%08X  ProductCode=0x%08X\n",
               i, ec_slave[i].name, ec_slave[i].eep_man, ec_slave[i].eep_id);
    }

    /* ── Langkah 3: Daftarkan callback PO2SOconfig ── */
    for (int i = 1; i <= ec_slavecount; i++) {
        ec_slave[i].PO2SOconfig = lichuan_po2so_config;
    }

    /* ── Langkah 4: Map IOmap
     *    ec_config_map memanggil PO2SOconfig tiap slave,
     *    mengonfigurasi FMMU/SyncManager, dan mengisi IOmap.     ── */
    map_size = ec_config_map(IOmap);
    printf("IOmap size: %d byte\n", map_size);
    for (int i = 1; i <= ec_slavecount; i++) {
        printf("  Slave %d: Output=%d byte  Input=%d byte\n",
               i, ec_slave[i].Obytes, ec_slave[i].Ibytes);
    }

    /* ── Langkah 5: Tunggu semua slave masuk Safe-Op ── */
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 3);
    if (ec_slave[0].state != EC_STATE_SAFE_OP) {
        printf("Slave tidak mencapai Safe-Op (state=0x%02X).\n", ec_slave[0].state);
        for (int i = 1; i <= ec_slavecount; i++) {
            printf("  Slave %d: state=0x%02X  ALstatus=0x%04X\n",
                   i, ec_slave[i].state, ec_slave[i].ALstatuscode);
        }
        goto cleanup;
    }
    printf("Semua slave di Safe-Op.\n");

    /* ── Langkah 6: Kirim satu frame sebelum request Operational ── */
    ec_send_processdata();
    wkc = ec_receive_processdata(EC_TIMEOUTRET);

    /* ── Langkah 7: Request semua slave masuk Operational ── */
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ecx_writestate(&ecx_context, 0);
    ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);

    if (ec_slave[0].state != EC_STATE_OPERATIONAL) {
        printf("Gagal masuk Operational (state=0x%02X).\n", ec_slave[0].state);
        for (int i = 1; i <= ec_slavecount; i++) {
            if (ec_slave[i].state != EC_STATE_OPERATIONAL) {
                printf("  Slave %d: state=0x%02X  ALstatus=0x%04X\n",
                       i, ec_slave[i].state, ec_slave[i].ALstatuscode);
            }
        }
        goto cleanup;
    }
    printf("Semua slave di Operational!\n\n");

    /* ── Langkah 8: Dapatkan pointer ke PDO slave ── */
    output = (OutputPDO *)(ec_slave[slave_idx].outputs);
    input  = (InputPDO  *)(ec_slave[slave_idx].inputs);

    if (!output || !input) {
        printf("Pointer PDO NULL. Periksa ukuran IOmap dan PDO mapping slave.\n");
        goto cleanup;
    }

    /* Inisialisasi output PDO */
    memset(output, 0, sizeof(*output));
    output->control_word          = CW_DISABLE_VOLTAGE;
    output->target_position       = 0;
    output->touch_probe_function  = 0;
    // padding fields will be zero from memset

    /* ── Langkah 9: Cyclic loop ── */
    printf("Cyclic loop berjalan (Ctrl+C untuk berhenti)...\n");

    /*
     * Sub-step state machine CiA402 (manual Chapter 7.1, tabel state switching):
     *   Step 0: Kirim Shutdown (0x06)  -> Servo fault free → Servo ready
     *   Step 1: Tunggu READY, kirim Switch On (0x07) -> Servo ready → Waiting enable
     *   Step 2: Tunggu SWITCHED_ON, kirim Enable (0x0F) -> Servo running
     *   Step 3: Tunggu OPERATION_ENABLED -> mulai kontrol posisi
     */

    while (g_running) {

        /* ── Kirim & terima frame process data ── */
        ec_send_processdata();
        wkc = ec_receive_processdata(EC_TIMEOUTRET);

        if (wkc < ec_slavecount) {
            printf("[%d] WKC rendah: %d (expected %d)\n", cycle, wkc, ec_slavecount);
        }

        uint16_t sw = input->status_word;

        /* ── State machine CiA402 ── */
        if (!enabled) {

            if (sw & SW_FAULT) {
                /* Ada fault - lakukan fault reset (bit7 rising edge pada 6040h)
                 * Manual Chapter 7.1, state 15: kontrol word 0x80              */
                printf("[%d] FAULT! error_code=0x%04X. Mencoba fault reset...\n",
                       cycle, input->error_code);
                output->control_word = CW_FAULT_RESET;
                cia_step = 0;
            }
            else {
                switch (cia_step) {
                case 0:
                    /* Shutdown command: 0x06
                     * Manual state 2: Servo fault free → Servo ready */
                    output->control_word = CW_SHUTDOWN;
                    printf("[%d] CiA402: Shutdown (0x06)\n", cycle);
                    cia_step = 1;
                    break;

                case 1:
                    /* Tunggu status READY_TO_SWITCH_ON */
                    if ((sw & SW_READY_TO_SWITCH_ON) && !(sw & SW_SWITCHED_ON)) {
                        /* Switch On command: 0x07
                         * Manual state 3: Servo ready → Waiting to turn on enable */
                        output->control_word = CW_SWITCH_ON;
                        printf("[%d] CiA402: Switch On (0x07)\n", cycle);
                        cia_step = 2;
                    }
                    break;

                case 2:
                    /* Tunggu status SWITCHED_ON */
                    if ((sw & SW_SWITCHED_ON) && !(sw & SW_OPERATION_ENABLED)) {
                        /* Enable Operation command: 0x0F
                         * Manual state 4: Waiting enable → Servo running */
                        output->control_word = CW_ENABLE_OPERATION;
                        printf("[%d] CiA402: Enable Operation (0x0F)\n", cycle);
                        cia_step = 3;
                    }
                    break;

                case 3:
                    /* Tunggu OPERATION_ENABLED */
                    if (sw & SW_OPERATION_ENABLED) {
                        printf("[%d] Servo ENABLED!\n", cycle);
                        print_status(sw, input->error_code);

                        /* Kunci target ke posisi saat ini agar tidak ada lompatan */
                        home_position            = input->position_actual_value;
                        output->target_position  = home_position;
                        printf("  Home position: %d command unit\n", home_position);
                        printf("  Torque: %.1f%%\n", input->torque_actual_value * 0.1f);

                        enabled  = 1;
                        cia_step = 0;
                    }
                    break;
                }
            }
        }
        else {
            /* ── Mode operasi: CSP (Cyclic Synchronous Position) ──
             *
             * Jika servo kehilangan enable (fault / warning) → kembali ke
             * state machine untuk re-enable.
             */
            if (!(sw & SW_OPERATION_ENABLED)) {
                printf("[%d] Servo kehilangan enable! sw=0x%04X\n", cycle, sw);
                print_status(sw, input->error_code);
                enabled  = 0;
                cia_step = 0;
            }
            else {
                /* ──────────────────────────────────────────────────────
                 * LOGIKA KONTROL POSISI
                 * Contoh: Gerakan pulang-pergi setiap 3 detik
                 *   - Fase 0 (0-2999 cycle): gerak ke home + 100000 unit
                 *   - Fase 1 (3000-5999 cycle): kembali ke home
                 *
                 * GANTI bagian ini dengan logika kontrol Anda sendiri,
                 * misalnya membaca target dari ROS topic, modbus, atau
                 * antarmuka lain.
                 * ────────────────────────────────────────────────────── */
                const int32_t MOVE_DISTANCE = 100000; /* dalam command unit  */
                const int     HALF_PERIOD   = 3000;   /* cycle per 1/2 gerak */

                int phase = (cycle / HALF_PERIOD) % 2;
                output->target_position = home_position +
                                         (phase == 0 ? MOVE_DISTANCE : 0);

                /* Cetak monitor setiap 1000 cycle (~1 detik pada 1ms/cycle) */
                if ((cycle % 1000) == 0) {
                    printf("[%d] pos=%-8d target=%-8d dev=%-6d torque=%.1f%% sw=0x%04X\n",
                           cycle,
                           (int)input->position_actual_value,
                           (int)output->target_position,
                           (int)input->position_deviation,
                           input->torque_actual_value * 0.1f,
                           sw);
                }
            }
        }

        cycle++;

        /* Tunda 1ms per cycle (1kHz) - sesuai tabel Chapter 6.3 manual */
        osal_usleep(1000);
    }

    /* ── Langkah 10: Graceful shutdown ── */
    printf("\nMenghentikan servo...\n");

    /* Quick Stop dulu agar motor melambat dengan ramp */
    output->control_word = CW_QUICK_STOP;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    osal_usleep(200000);  /* 200ms tunggu motor berhenti */

    /* Disable Voltage - lepas enable */
    output->control_word = CW_DISABLE_VOLTAGE;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    osal_usleep(50000);

    /* Kembalikan slave ke state Init */
    ec_slave[0].state = EC_STATE_INIT;
    ecx_writestate(&ecx_context, 0);
    printf("Servo dinonaktifkan.\n");

cleanup:
    ec_close();
    printf("Selesai.\n");
}

/* Deteksi slave saja: ec_config_init menyiapkan bus ke INIT/PRE-OP.
 * Tidak memasang callback konfigurasi servo, memetakan PDO, meminta OP,
 * atau mengirim control word/target gerakan. */
static int scan_ethercat(const char *ifname)
{
    init_context();
    if (!ec_init(ifname)) {
        printf("ec_init gagal pada '%s'. Periksa interface dan hak raw socket.\n", ifname);
        return 1;
    }
    printf("ec_init pada '%s' berhasil.\n", ifname);

    const int count = ec_config_init(FALSE);
    if (count <= 0) {
        printf("Tidak ada slave EtherCAT ditemukan!\n");
        ec_close();
        return 2;
    }

    ecx_readstate(&ecx_context);
    printf("%d slave ditemukan:\n", count);
    for (int i = 1; i <= count; ++i) {
        printf("  Slave %d: %s VendorID=0x%08X ProductCode=0x%08X "
               "state=0x%02X ALstatus=0x%04X\n",
               i, ec_slave[i].name, ec_slave[i].eep_man, ec_slave[i].eep_id,
               ec_slave[i].state, ec_slave[i].ALstatuscode);
    }
    ec_close();
    printf("Scan selesai; tidak mengirim perintah enable atau target gerak.\n");
    return 0;
}

/* ───────────────────────────────────────────────────────────────
 * Entry point
 * ─────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    // Show each diagnostic line immediately, including through `2>&1 | tee`.
    setvbuf(stdout, nullptr, _IOLBF, 0);
    // The optional fourth argument adds confirmation to a no-enable test.
    // Motion mode (no mode argument) always requires confirmation.
    bool ask_brake = false;
    if (argc >= 3 && strcmp(argv[argc - 1], "--confirm-brake") == 0) {
        ask_brake = true;
        --argc;
    }
    if (argc < 2 || argc > 3 || argv[1][0] == '-' ||
        (argc == 3 && strcmp(argv[2], "--scan-only") != 0 &&
         strcmp(argv[2], "--check-pdo") != 0 &&
         strcmp(argv[2], "--check-pdo-esi") != 0 &&
         strcmp(argv[2], "--test-rotate") != 0)) {
        printf("Penggunaan: sudo %s <interface_jaringan> [--scan-only|--check-pdo|--check-pdo-esi|--test-rotate] [--confirm-brake]\n\n", argv[0]);
        printf("  Uji PDO dengan konfirmasi rem: sudo %s enp2s0 --check-pdo-esi --confirm-brake\n", argv[0]);
        printf("  Tanpa opsi mode: program gerak lama, konfirmasi rem wajib.\n\n");
        printf("Gunakan 'ip link' atau 'ifconfig' untuk melihat nama interface.\n");
        return 1;
    }

    /* Pasang signal handler agar Ctrl+C mematikan servo dengan benar */
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    printf("=== EtherCAT Master - LICHUAN LC-E Series ===\n");
    printf("Interface : %s\n\n", argv[1]);

    const bool rotate = argc == 3 && strcmp(argv[2], "--test-rotate") == 0;
    if (rotate) {
        printf("Uji POROS BEBAS: satu gerakan +13107200 unit (100 × 360 derajat), speed 1310720 unit/s (~500 RPM), lalu stop dan disable.\n");
        printf("Untuk motor R17 gear 1:1 kira-kira +36000 derajat (100 putaran penuh); tidak kembali otomatis.\n");
        printf("Rem harus dilepas dan poros bebas dari mekanisme/beban. Ctrl+C meminta stop.\n");
    } else if (argc == 2) {
        printf("Mode gerak lama: dapat enable dan menggerakkan motor; mapping belum divalidasi dengan XML.\n");
    } else if (ask_brake) {
        printf("Mode diagnostik: tidak mengaktifkan servo, meskipun rem sudah dilepas.\n");
    }
    if ((argc == 2 || ask_brake || rotate) && !confirm_brake_released(&g_running)) return 2;
    if (!g_running) return 2;

    if (rotate) return test_rotate(argv[1], &g_running);

    if (argc == 3 && (strcmp(argv[2], "--check-pdo") == 0 ||
                      strcmp(argv[2], "--check-pdo-esi") == 0)) {
        return check_pdo(argv[1], &g_running, strcmp(argv[2], "--check-pdo-esi") == 0);
    }
    if (argc == 3) {
        printf("Mode: deteksi slave tanpa enable motor (INIT/PRE-OP).\n");
        return scan_ethercat(argv[1]);
    }

    run_ethercat(argv[1]);

    return 0;
}
