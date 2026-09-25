/**
 * EtherCAT Diagnostic Tool for LICHUAN LC10E Servo Drive
 * ========================================================
 * 
 * Program diagnostic lengkap untuk:
 * - Monitoring real-time PDO data
 * - Testing berbagai mode operasi (CSP, CSV, PP, PV)
 * - Analisis state machine CiA402
 * - Error detection dan logging
 * - SDO read/write untuk konfigurasi
 * 
 * PDO Mapping: 0x1702/0x1B02 (26/24 bytes) - Hardcoded layout
 * 
 * Penggunaan: sudo ./diagnostic <interface> [options]
 *   --monitor     : Monitor mode (default) - tampilkan data real-time
 *   --test-csp    : Test Cyclic Synchronous Position mode
 *   --test-csv    : Test Cyclic Synchronous Velocity mode
 *   --read-sdo    : Baca parameter SDO penting
 *   --info        : Tampilkan informasi device
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <stdarg.h>
#include "soem/ethercat.h"
#include "soem/soem.h"
#include "osal/osal.h"
#include "brake_confirm.h"

/* ───────────────────────────────────────────────────────────────
 * Konstanta CiA402
 * ─────────────────────────────────────────────────────────────── */
#define CW_SHUTDOWN          0x0006u
#define CW_SWITCH_ON         0x0007u
#define CW_ENABLE_OPERATION  0x000Fu
#define CW_DISABLE_VOLTAGE   0x0000u
#define CW_QUICK_STOP        0x0002u
#define CW_FAULT_RESET       0x0080u

#define SW_READY_TO_SWITCH_ON  (1u << 0)
#define SW_SWITCHED_ON         (1u << 1)
#define SW_OPERATION_ENABLED   (1u << 2)
#define SW_FAULT               (1u << 3)
#define SW_VOLTAGE_ENABLED     (1u << 4)
#define SW_QUICK_STOP_ACTIVE   (1u << 5)
#define SW_SWITCH_ON_DISABLED  (1u << 6)
#define SW_WARNING             (1u << 7)
#define SW_TARGET_REACHED      (1u << 10)

/* Mode operasi */
#define MODE_PP   1   // Profile Position
#define MODE_PV   3   // Profile Velocity
#define MODE_HM   6   // Homing
#define MODE_CSP  8   // Cyclic Synchronous Position
#define MODE_CSV  9   // Cyclic Synchronous Velocity
#define MODE_CST  10  // Cyclic Synchronous Torque

/* ───────────────────────────────────────────────────────────────
 * Struktur PDO - Actual mapping from hardware (0x1702/0x1B02)
 * ─────────────────────────────────────────────────────────────── */
#pragma pack(push, 1)

typedef struct {
    uint16_t control_word;
    int32_t  target_position;
    uint16_t touch_probe_function;
    int32_t  padding1;
    int32_t  padding2;
    int32_t  padding3;
    int32_t  padding4;
    uint16_t padding5;
} OutputPDO;  // 26 bytes

typedef struct {
    uint16_t error_code;
    uint16_t status_word;
    int32_t  position_actual_value;
    int16_t  torque_actual_value;
    int32_t  position_deviation;
    uint16_t touch_probe_status;
    int32_t  touch_probe_pos1_value;
    int32_t  touch_probe_pos2_value;
} InputPDO;  // 24 bytes

#pragma pack(pop)

/* ───────────────────────────────────────────────────────────────
 * Global variables
 * ─────────────────────────────────────────────────────────────── */
ecx_contextt ecx_context;
static char IOmap[4096];
static volatile sig_atomic_t g_running = 1;
static FILE *log_file = NULL;

/* ───────────────────────────────────────────────────────────────
 * Signal handler
 * ─────────────────────────────────────────────────────────────── */
static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
}

/* ───────────────────────────────────────────────────────────────
 * Helper functions
 * ─────────────────────────────────────────────────────────────── */
static void init_context(void)
{
    memset(&ecx_context, 0, sizeof(ecx_context));
}

static const char* get_state_name(uint16_t sw)
{
    if (sw & SW_FAULT) return "FAULT";
    if ((sw & 0x006F) == 0x0000) return "NOT_READY_TO_SWITCH_ON";
    if ((sw & 0x006F) == 0x0040) return "SWITCH_ON_DISABLED";
    if ((sw & 0x006F) == 0x0021) return "READY_TO_SWITCH_ON";
    if ((sw & 0x006F) == 0x0023) return "SWITCHED_ON";
    if ((sw & 0x006F) == 0x0027) return "OPERATION_ENABLED";
    if ((sw & 0x006F) == 0x0007) return "QUICK_STOP_ACTIVE";
    if ((sw & 0x004F) == 0x000F) return "FAULT_REACTION_ACTIVE";
    return "UNKNOWN";
}

static const char* get_mode_name(int8_t mode)
{
    switch(mode) {
        case MODE_PP:  return "Profile Position (PP)";
        case MODE_PV:  return "Profile Velocity (PV)";
        case MODE_HM:  return "Homing (HM)";
        case MODE_CSP: return "Cyclic Sync Position (CSP)";
        case MODE_CSV: return "Cyclic Sync Velocity (CSV)";
        case MODE_CST: return "Cyclic Sync Torque (CST)";
        default:       return "Unknown";
    }
}

static void log_message(const char *format, ...)
{
    char timestamp[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    va_list args;
    va_start(args, format);
    
    printf("[%s] ", timestamp);
    vprintf(format, args);
    
    if (log_file) {
        fprintf(log_file, "[%s] ", timestamp);
        vfprintf(log_file, format, args);
        fflush(log_file);
    }
    
    va_end(args);
}

static void print_status_detailed(uint16_t sw, uint16_t err)
{
    printf("\n┌─── Status Word: 0x%04X ───────────────────────────────────────┐\n", sw);
    printf("│ State: %-25s                              │\n", get_state_name(sw));
    printf("│ Bits:                                                          │\n");
    printf("│   [0] Ready to Switch On    : %s                              │\n", 
           (sw & SW_READY_TO_SWITCH_ON) ? "YES" : "NO ");
    printf("│   [1] Switched On           : %s                              │\n", 
           (sw & SW_SWITCHED_ON) ? "YES" : "NO ");
    printf("│   [2] Operation Enabled     : %s                              │\n", 
           (sw & SW_OPERATION_ENABLED) ? "YES" : "NO ");
    printf("│   [3] Fault                 : %s                              │\n", 
           (sw & SW_FAULT) ? "YES" : "NO ");
    printf("│   [4] Voltage Enabled       : %s                              │\n", 
           (sw & SW_VOLTAGE_ENABLED) ? "YES" : "NO ");
    printf("│   [5] Quick Stop            : %s                              │\n", 
           !(sw & SW_QUICK_STOP_ACTIVE) ? "ACTIVE  " : "INACTIVE");
    printf("│   [6] Switch On Disabled    : %s                              │\n", 
           (sw & SW_SWITCH_ON_DISABLED) ? "YES" : "NO ");
    printf("│   [7] Warning               : %s                              │\n", 
           (sw & SW_WARNING) ? "YES" : "NO ");
    printf("│   [10] Target Reached       : %s                              │\n", 
           (sw & SW_TARGET_REACHED) ? "YES" : "NO ");
    printf("└────────────────────────────────────────────────────────────────┘\n");
    
    if (err != 0) {
        printf("│ Error Code: 0x%04X                                             │\n", err);
        printf("└────────────────────────────────────────────────────────────────┘\n");
    }
}

/* ───────────────────────────────────────────────────────────────
 * SDO read/write functions
 * ─────────────────────────────────────────────────────────────── */
static int read_sdo_uint8(uint16 slave, uint16 index, uint8 subindex, uint8_t *value)
{
    int size = sizeof(*value);
    int ret = ecx_SDOread(&ecx_context, slave, index, subindex, FALSE, &size, value, EC_TIMEOUTRXM);
    return (ret > 0) ? 0 : -1;
}

static int read_sdo_uint16(uint16 slave, uint16 index, uint8 subindex, uint16_t *value)
{
    int size = sizeof(*value);
    int ret = ecx_SDOread(&ecx_context, slave, index, subindex, FALSE, &size, value, EC_TIMEOUTRXM);
    return (ret > 0) ? 0 : -1;
}

static int read_sdo_uint32(uint16 slave, uint16 index, uint8 subindex, uint32_t *value)
{
    int size = sizeof(*value);
    int ret = ecx_SDOread(&ecx_context, slave, index, subindex, FALSE, &size, value, EC_TIMEOUTRXM);
    return (ret > 0) ? 0 : -1;
}

static int read_sdo_int32(uint16 slave, uint16 index, uint8 subindex, int32_t *value)
{
    int size = sizeof(*value);
    int ret = ecx_SDOread(&ecx_context, slave, index, subindex, FALSE, &size, value, EC_TIMEOUTRXM);
    return (ret > 0) ? 0 : -1;
}

static int write_sdo_uint8(uint16 slave, uint16 index, uint8 subindex, uint8_t value)
{
    int size = sizeof(value);
    int ret = ecx_SDOwrite(&ecx_context, slave, index, subindex, FALSE, size, &value, EC_TIMEOUTRXM);
    return (ret > 0) ? 0 : -1;
}

static int write_sdo_uint32(uint16 slave, uint16 index, uint8 subindex, uint32_t value)
{
    int size = sizeof(value);
    int ret = ecx_SDOwrite(&ecx_context, slave, index, subindex, FALSE, size, &value, EC_TIMEOUTRXM);
    return (ret > 0) ? 0 : -1;
}

/* ───────────────────────────────────────────────────────────────
 * Configuration callback
 * ─────────────────────────────────────────────────────────────── */
static int diagnostic_config(ecx_contextt *context, uint16 slave)
{
    printf("  [Slave %d] Konfigurasi diagnostic...\n", slave);
    
    // Set mode CSP sebagai default
    uint8_t mode = MODE_CSP;
    ecx_SDOwrite(context, slave, 0x6060, 0x00, FALSE, sizeof(mode), &mode, EC_TIMEOUTRXM);
    
    // Set gear ratio 1:1
    uint32_t ratio = 1;
    ecx_SDOwrite(context, slave, 0x6091, 0x01, FALSE, sizeof(ratio), &ratio, EC_TIMEOUTRXM);
    ecx_SDOwrite(context, slave, 0x6091, 0x02, FALSE, sizeof(ratio), &ratio, EC_TIMEOUTRXM);
    
    printf("  [Slave %d] Konfigurasi selesai.\n", slave);
    return 0;
}

/* ───────────────────────────────────────────────────────────────
 * State machine control
 * ─────────────────────────────────────────────────────────────── */
static int enable_servo(OutputPDO *output, InputPDO *input, int timeout_ms)
{
    int elapsed = 0;
    int step = 0;
    
    log_message("Memulai sequence enable servo...\n");
    
    while (elapsed < timeout_ms && g_running) {
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        
        uint16_t sw = input->status_word;
        
        if (sw & SW_FAULT) {
            log_message("  FAULT detected! Melakukan reset...\n");
            output->control_word = CW_FAULT_RESET;
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);
            osal_usleep(100000);
            output->control_word = CW_DISABLE_VOLTAGE;
            step = 0;
            elapsed = 0;
            continue;
        }
        
        switch(step) {
            case 0:  // Shutdown
                output->control_word = CW_SHUTDOWN;
                step = 1;
                log_message("  Step 0: Shutdown (0x06)\n");
                break;
                
            case 1:  // Wait for Ready to Switch On
                if ((sw & SW_READY_TO_SWITCH_ON) && !(sw & SW_SWITCHED_ON)) {
                    output->control_word = CW_SWITCH_ON;
                    step = 2;
                    log_message("  Step 1: Switch On (0x07)\n");
                }
                break;
                
            case 2:  // Wait for Switched On
                if ((sw & SW_SWITCHED_ON) && !(sw & SW_OPERATION_ENABLED)) {
                    output->control_word = CW_ENABLE_OPERATION;
                    step = 3;
                    log_message("  Step 2: Enable Operation (0x0F)\n");
                }
                break;
                
            case 3:  // Wait for Operation Enabled
                if (sw & SW_OPERATION_ENABLED) {
                    log_message("  Step 3: Servo ENABLED! State: %s\n", get_state_name(sw));
                    return 0;  // Success
                }
                break;
        }
        
        osal_usleep(10000);  // 10ms
        elapsed += 10;
    }
    
    log_message("GAGAL: Timeout saat enable servo!\n");
    return -1;
}

static void disable_servo(OutputPDO *output)
{
    log_message("Menonaktifkan servo...\n");
    output->control_word = CW_QUICK_STOP;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    osal_usleep(200000);
    
    output->control_word = CW_DISABLE_VOLTAGE;
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    osal_usleep(50000);
    
    log_message("Servo dinonaktifkan.\n");
}

/* ───────────────────────────────────────────────────────────────
 * Diagnostic modes
 * ─────────────────────────────────────────────────────────────── */

// Mode 1: Monitor real-time data
static void mode_monitor(OutputPDO *output, InputPDO *input)
{
    int cycle = 0;
    int32_t last_pos = 0;
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║              MONITOR MODE - Real-time Data                     ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    printf("Tekan Ctrl+C untuk berhenti\n\n");
    
    if (enable_servo(output, input, 5000) != 0) {
        return;
    }
    
    // Set target ke posisi saat ini
    output->target_position = input->position_actual_value;
    last_pos = input->position_actual_value;
    
    printf("\n┌────────┬───────────┬───────────┬──────────┬──────────┬────────┐\n");
    printf("│ Cycle  │ Position  │  Target   │ Deviation│  Torque  │ Error  │\n");
    printf("├────────┼───────────┼───────────┼──────────┼──────────┼────────┤\n");
    
    while (g_running) {
        ec_send_processdata();
        ec_receive_processdata(EC_TIMEOUTRET);
        
        if ((cycle % 100) == 0) {  // Update setiap 100ms
            printf("│ %6d │ %9d │ %9d │ %8d │ %7.1f%% │ 0x%04X │\n",
                   cycle,
                   input->position_actual_value,
                   output->target_position,
                   input->position_deviation,
                   input->torque_actual_value * 0.1f,
                   input->error_code);
            
            if (input->status_word & SW_FAULT) {
                printf("└────────┴───────────┴───────────┴──────────┴──────────┴────────┘\n");
                log_message("FAULT terdeteksi! Error code: 0x%04X\n", input->error_code);
                print_status_detailed(input->status_word, input->error_code);
                break;
            }
        }
        
        cycle++;
        osal_usleep(1000);  // 1ms cycle
    }
    
    printf("└────────┴───────────┴───────────┴──────────┴──────────┴────────┘\n");
}

// Mode 2: Test CSP (Cyclic Synchronous Position)
static void mode_test_csp(OutputPDO *output, InputPDO *input)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           TEST MODE - Cyclic Synchronous Position              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    if (enable_servo(output, input, 5000) != 0) {
        return;
    }
    
    int32_t home_pos = input->position_actual_value;
    output->target_position = home_pos;
    
    log_message("Home position: %d\n", home_pos);
    log_message("Test akan melakukan gerakan:\n");
    log_message("  1. Forward  +50000 pulses\n");
    log_message("  2. Backward -50000 pulses (kembali home)\n");
    log_message("  3. Forward  +100000 pulses\n");
    log_message("  4. Backward -100000 pulses (kembali home)\n\n");
    
    struct {
        int32_t target;
        int duration_ms;
        const char *name;
    } moves[] = {
        {home_pos + 50000,  3000, "Forward +50k"},
        {home_pos,          3000, "Return home"},
        {home_pos + 100000, 4000, "Forward +100k"},
        {home_pos,          4000, "Return home"},
    };
    
    for (size_t i = 0; i < sizeof(moves)/sizeof(moves[0]) && g_running; i++) {
        log_message("Move %zu: %s -> target=%d\n", i+1, moves[i].name, moves[i].target);
        output->target_position = moves[i].target;
        
        int elapsed = 0;
        int last_print = 0;
        
        while (elapsed < moves[i].duration_ms && g_running) {
            ec_send_processdata();
            ec_receive_processdata(EC_TIMEOUTRET);
            
            if (elapsed - last_print >= 500) {  // Print setiap 500ms
                int32_t error = output->target_position - input->position_actual_value;
                printf("  [%4d ms] pos=%9d target=%9d error=%6d torque=%.1f%%\n",
                       elapsed, input->position_actual_value, 
                       output->target_position, error,
                       input->torque_actual_value * 0.1f);
                last_print = elapsed;
            }
            
            if (input->status_word & SW_FAULT) {
                log_message("FAULT! Menghentikan test.\n");
                print_status_detailed(input->status_word, input->error_code);
                return;
            }
            
            osal_usleep(1000);
            elapsed++;
        }
        
        log_message("Move %zu selesai. Position: %d\n\n", i+1, input->position_actual_value);
    }
    
    log_message("Test CSP selesai!\n");
}

// Mode 3: Test CSV - TIDAK TERSEDIA di mapping 0x1702/0x1B02
// Mapping aktual tidak memiliki target_velocity atau velocity_actual_value
static void mode_test_csv(OutputPDO *output, InputPDO *input)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║           TEST CSV - Not Available in Current Mapping          ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    log_message("CSV mode tidak tersedia dengan PDO mapping 0x1702/0x1B02\n");
    log_message("Mapping aktual tidak memiliki field target_velocity atau velocity_actual.\n");
    log_message("Gunakan mode CSP (Cyclic Synchronous Position) sebagai gantinya.\n");
}

// Mode 4: Read important SDO parameters
static void mode_read_sdo(void)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║              SDO PARAMETERS - Device Information               ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    uint16 slave = 1;
    
    // Device identification
    char device_name[35] = {0};
    int size = sizeof(device_name) - 1;
    if (ecx_SDOread(&ecx_context, slave, 0x1008, 0x00, FALSE, &size, device_name, EC_TIMEOUTRXM) > 0) {
        printf("Device Name (1008h)         : %s\n", device_name);
    }
    
    char hw_version[5] = {0};
    size = sizeof(hw_version) - 1;
    if (ecx_SDOread(&ecx_context, slave, 0x1009, 0x00, FALSE, &size, hw_version, EC_TIMEOUTRXM) > 0) {
        printf("Hardware Version (1009h)    : %s\n", hw_version);
    }
    
    char sw_version[5] = {0};
    size = sizeof(sw_version) - 1;
    if (ecx_SDOread(&ecx_context, slave, 0x100A, 0x00, FALSE, &size, sw_version, EC_TIMEOUTRXM) > 0) {
        printf("Software Version (100Ah)    : %s\n", sw_version);
    }
    
    // Identity object
    uint32_t vendor_id = 0;
    if (read_sdo_uint32(slave, 0x1018, 0x01, &vendor_id) == 0) {
        printf("Vendor ID (1018h:01)        : 0x%08X\n", vendor_id);
    }
    
    uint32_t product_code = 0;
    if (read_sdo_uint32(slave, 0x1018, 0x02, &product_code) == 0) {
        printf("Product Code (1018h:02)     : 0x%08X\n", product_code);
    }
    
    uint32_t revision = 0;
    if (read_sdo_uint32(slave, 0x1018, 0x03, &revision) == 0) {
        printf("Revision (1018h:03)         : 0x%08X\n", revision);
    }
    
    uint32_t serial = 0;
    if (read_sdo_uint32(slave, 0x1018, 0x04, &serial) == 0) {
        printf("Serial Number (1018h:04)    : %u\n", serial);
    }
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                   CiA402 Drive Parameters                      ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    // CiA402 parameters
    uint8_t modes_supported = 0;
    if (read_sdo_uint8(slave, 0x6502, 0x00, &modes_supported) == 0) {
        printf("Supported Drive Modes (6502h):\n");
        if (modes_supported & (1<<0)) printf("  - Profile Position (PP)\n");
        if (modes_supported & (1<<1)) printf("  - Profile Velocity (PV)\n");
        if (modes_supported & (1<<2)) printf("  - Homing (HM)\n");
        if (modes_supported & (1<<3)) printf("  - Cyclic Sync Position (CSP)\n");
        if (modes_supported & (1<<4)) printf("  - Cyclic Sync Velocity (CSV)\n");
        if (modes_supported & (1<<5)) printf("  - Cyclic Sync Torque (CST)\n");
    }
    
    uint32_t max_velocity = 0;
    if (read_sdo_uint32(slave, 0x607F, 0x00, &max_velocity) == 0) {
        printf("Max Velocity (607Fh)        : %u\n", max_velocity);
    }
    
    uint32_t max_accel = 0;
    if (read_sdo_uint32(slave, 0x60C5, 0x00, &max_accel) == 0) {
        printf("Max Acceleration (60C5h)    : %u\n", max_accel);
    }
    
    uint32_t max_decel = 0;
    if (read_sdo_uint32(slave, 0x60C6, 0x00, &max_decel) == 0) {
        printf("Max Deceleration (60C6h)    : %u\n", max_decel);
    }
    
    uint32_t pos_range_limit = 0;
    if (read_sdo_uint32(slave, 0x607B, 0x01, &pos_range_limit) == 0) {
        printf("Position Range Min (607Bh:01): %d\n", (int32_t)pos_range_limit);
    }
    if (read_sdo_uint32(slave, 0x607B, 0x02, &pos_range_limit) == 0) {
        printf("Position Range Max (607Bh:02): %d\n", (int32_t)pos_range_limit);
    }
    
    printf("\n");
}

// Mode 5: Device info
static void mode_info(void)
{
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                    EtherCAT Device Info                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    for (int i = 1; i <= ec_slavecount; i++) {
        printf("Slave %d:\n", i);
        printf("  Name            : %s\n", ec_slave[i].name);
        printf("  Vendor ID       : 0x%08X\n", ec_slave[i].eep_man);
        printf("  Product Code    : 0x%08X\n", ec_slave[i].eep_id);
        printf("  Revision        : 0x%08X\n", ec_slave[i].eep_rev);
        printf("  State           : 0x%02X (%s)\n", ec_slave[i].state,
               (ec_slave[i].state == EC_STATE_OPERATIONAL) ? "OPERATIONAL" :
               (ec_slave[i].state == EC_STATE_SAFE_OP) ? "SAFE-OP" :
               (ec_slave[i].state == EC_STATE_PRE_OP) ? "PRE-OP" : "INIT");
        printf("  Output bytes    : %d\n", ec_slave[i].Obytes);
        printf("  Input bytes     : %d\n", ec_slave[i].Ibytes);
        printf("  Has DC          : %s\n", ec_slave[i].hasdc ? "YES" : "NO");
        printf("  Config address  : 0x%04X\n", ec_slave[i].configadr);
        printf("\n");
    }
}

/* ───────────────────────────────────────────────────────────────
 * Main EtherCAT initialization and run
 * ─────────────────────────────────────────────────────────────── */
static int init_ethercat(const char *ifname)
{
    init_context();
    
    if (!ec_init(ifname)) {
        printf("ERROR: ec_init gagal pada '%s'\n", ifname);
        return -1;
    }
    printf("✓ ec_init berhasil pada '%s'\n", ifname);
    
    if (ec_config_init(FALSE) <= 0) {
        printf("ERROR: Tidak ada slave EtherCAT ditemukan!\n");
        ec_close();
        return -1;
    }
    printf("✓ %d slave ditemukan\n", ec_slavecount);
    
    // Register config callback
    for (int i = 1; i <= ec_slavecount; i++) {
        ec_slave[i].PO2SOconfig = diagnostic_config;
    }
    
    int iomap_size = ec_config_map(IOmap);
    printf("✓ IOmap size: %d bytes\n", iomap_size);
    
    ec_statecheck(0, EC_STATE_SAFE_OP, EC_TIMEOUTSTATE * 3);
    if (ec_slave[0].state != EC_STATE_SAFE_OP) {
        printf("ERROR: Slave tidak mencapai Safe-Op\n");
        ec_close();
        return -1;
    }
    printf("✓ Semua slave di Safe-Op\n");
    
    ec_send_processdata();
    ec_receive_processdata(EC_TIMEOUTRET);
    
    ec_slave[0].state = EC_STATE_OPERATIONAL;
    ecx_writestate(&ecx_context, 0);
    ec_statecheck(0, EC_STATE_OPERATIONAL, EC_TIMEOUTSTATE);
    
    if (ec_slave[0].state != EC_STATE_OPERATIONAL) {
        printf("ERROR: Gagal masuk Operational\n");
        ec_close();
        return -1;
    }
    printf("✓ Semua slave di Operational\n\n");
    
    return 0;
}

/* ───────────────────────────────────────────────────────────────
 * Main function
 * ─────────────────────────────────────────────────────────────── */
int main(int argc, char *argv[])
{
    setvbuf(stdout, nullptr, _IOLBF, 0);
    if (argc < 2) {
        printf("EtherCAT Diagnostic Tool - Lichuan LC10E Servo\n\n");
        printf("Penggunaan: sudo %s <interface> [mode]\n\n", argv[0]);
        printf("Modes:\n");
        printf("  --monitor     Monitor real-time data (default)\n");
        printf("  --test-csp    Test Cyclic Synchronous Position\n");
        printf("  --test-csv    Test Cyclic Synchronous Velocity\n");
        printf("  --read-sdo    Read SDO parameters\n");
        printf("  --info        Show device information\n\n");
        printf("Contoh:\n");
        printf("  sudo %s eth0 --monitor\n", argv[0]);
        printf("  sudo %s eth0 --test-csp\n", argv[0]);
        return 1;
    }
    
    const char *ifname = argv[1];
    const char *mode = (argc >= 3) ? argv[2] : "--monitor";
    if (argc > 3 || (strcmp(mode, "--monitor") != 0 && strcmp(mode, "--test-csp") != 0 &&
        strcmp(mode, "--test-csv") != 0 && strcmp(mode, "--read-sdo") != 0 && strcmp(mode, "--info") != 0)) {
        printf("Mode/argumen tidak valid; dibatalkan.\n");
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║        EtherCAT Diagnostic Tool - Lichuan LC10E Servo         ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    printf("Interface: %s\n", ifname);
    printf("Mode: %s\n\n", mode);

    // Even legacy --monitor enables the drive. Ask before any bus setup.
    if (strcmp(mode, "--info") != 0 && strcmp(mode, "--read-sdo") != 0) {
        printf("Mode ini dapat enable/menggerakkan motor; mapping lama belum divalidasi dengan XML.\n");
        if (!confirm_brake_released(&g_running)) return 2;
    }
    if (!g_running) return 2;
    
    // Open log file
    char logname[256];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(logname, sizeof(logname), "diagnostic_%Y%m%d_%H%M%S.log", t);
    log_file = fopen(logname, "w");
    if (log_file) {
        printf("Log file: %s\n\n", logname);
    }
    
    // Initialize EtherCAT
    if (init_ethercat(ifname) != 0) {
        if (log_file) fclose(log_file);
        return 1;
    }
    
    // Get PDO pointers
    OutputPDO *output = (OutputPDO *)(ec_slave[1].outputs);
    InputPDO *input = (InputPDO *)(ec_slave[1].inputs);
    
    if (!output || !input) {
        printf("ERROR: PDO pointer NULL\n");
        ec_close();
        if (log_file) fclose(log_file);
        return 1;
    }
    
    // Initialize output PDO
    memset(output, 0, sizeof(*output));
    output->control_word = CW_DISABLE_VOLTAGE;
    
    // Run selected mode
    if (strcmp(mode, "--info") == 0) {
        mode_info();
        mode_read_sdo();
    }
    else if (strcmp(mode, "--read-sdo") == 0) {
        mode_read_sdo();
    }
    else if (strcmp(mode, "--test-csp") == 0) {
        mode_test_csp(output, input);
        disable_servo(output);
    }
    else if (strcmp(mode, "--test-csv") == 0) {
        mode_test_csv(output, input);
        disable_servo(output);
    }
    else {  // default: --monitor
        mode_monitor(output, input);
        disable_servo(output);
    }
    
    // Cleanup
    ec_slave[0].state = EC_STATE_INIT;
    ecx_writestate(&ecx_context, 0);
    ec_close();
    
    if (log_file) {
        fclose(log_file);
        printf("\nLog tersimpan di: %s\n", logname);
    }
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                      Diagnostic Selesai                        ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
