# Lichuan LC10E EtherCAT Servo - Usage Guide

## Overview

Project ini berisi implementasi EtherCAT master untuk mengontrol servo drive Lichuan LC10E menggunakan library SOEM (Simple Open EtherCAT Master).

**Update terbaru:**
- ✅ Struktur PDO telah diperbaiki sesuai ESI XML V1.04
- ✅ RxPDO (0x1600): 13 bytes - Control Word, Target Position, Touch Probe, Mode, Target Velocity
- ✅ TxPDO (0x1A00): 23 bytes - Error Code, Status Word, Position, Velocity, Touch Probe, Digital Inputs, Mode Display
- ✅ Program diagnostic lengkap untuk testing dan monitoring

## Build Instructions

```bash
cd /home/robotwelder/Unduhan/EtherCatLichuan-main
mkdir -p build
cd build
cmake ..
make
```

Hasil build:
- `ethercat_servo` - Program utama untuk kontrol servo
- `diagnostic` - Tool diagnostic untuk testing dan monitoring

## Programs

### 1. ethercat_servo (Main Control Program)

Program utama untuk mengontrol servo drive dalam mode CSP (Cyclic Synchronous Position).

**Fitur:**
- State machine CiA402 lengkap
- Mode CSP dengan gerakan pulang-pergi otomatis
- Fault detection dan recovery
- Graceful shutdown

**Penggunaan:**

```bash
# Mode normal - jalankan program kontrol
sudo ./ethercat_servo eth0

# Scan slave saja (tidak enable motor)
sudo ./ethercat_servo eth0 --scan-only

# Check PDO mapping
sudo ./ethercat_servo eth0 --check-pdo
```

**Contoh output:**
```
ec_init pada 'eth0' berhasil.
1 slave ditemukan:
  Slave 1: LC10E_V1.04          VendorID=0x00000766  ProductCode=0x00000402
IOmap size: 36 byte
  Slave 1: Output=13 byte  Input=23 byte
[0] CiA402: Shutdown (0x06)
[10] CiA402: Switch On (0x07)
[25] CiA402: Enable Operation (0x0F)
[40] Servo ENABLED!
  Home position: 1234567 command unit
[1000] pos=1234567  target=1334567  vel=2500  mode=8 sw=0x0627
```

### 2. diagnostic (Diagnostic Tool)

Tool lengkap untuk testing, monitoring, dan diagnostic servo drive.

**Mode-mode yang tersedia:**

#### Mode 1: Monitor (default)
Monitor real-time data PDO dalam tabel yang rapi.

```bash
sudo ./diagnostic eth0 --monitor
```

Output:
```
┌────────┬───────────┬───────────┬──────────┬──────┬──────────┬────────┐
│ Cycle  │ Position  │  Target   │ Velocity │ Mode │  Status  │ Error  │
├────────┼───────────┼───────────┼──────────┼──────┼──────────┼────────┤
│      0 │   1234567 │   1234567 │        0 │   8  │ 0x0627 │ 0x0000 │
│    100 │   1234670 │   1234567 │      103 │   8  │ 0x0627 │ 0x0000 │
```

#### Mode 2: Test CSP
Test gerakan dalam mode Cyclic Synchronous Position.

```bash
sudo ./diagnostic eth0 --test-csp
```

Melakukan sequence:
1. Forward +50,000 pulses (3 detik)
2. Return to home (3 detik)
3. Forward +100,000 pulses (4 detik)
4. Return to home (4 detik)

#### Mode 3: Test CSV
Test kecepatan dalam mode Cyclic Synchronous Velocity.

```bash
sudo ./diagnostic eth0 --test-csv
```

Melakukan sequence:
1. Akselerasi ke +1000 rpm (hold 2 detik)
2. Reverse ke -1000 rpm (hold 2 detik)
3. Stop

#### Mode 4: Read SDO
Membaca parameter penting dari servo via SDO.

```bash
sudo ./diagnostic eth0 --read-sdo
```

Output:
```
╔════════════════════════════════════════════════════════════════╗
║              SDO PARAMETERS - Device Information               ║
╚════════════════════════════════════════════════════════════════╝

Device Name (1008h)         : LC10E_V1.04
Hardware Version (1009h)    : 1.04
Software Version (100Ah)    : 2.04
Vendor ID (1018h:01)        : 0x00000766
Product Code (1018h:02)     : 0x00000402

Supported Drive Modes (6502h):
  - Profile Position (PP)
  - Cyclic Sync Position (CSP)
  - Cyclic Sync Velocity (CSV)
Max Velocity (607Fh)        : 3000000
```

#### Mode 5: Device Info
Menampilkan informasi lengkap EtherCAT device.

```bash
sudo ./diagnostic eth0 --info
```

**Logging:**
Semua mode diagnostic akan membuat log file otomatis:
```
diagnostic_20240924_161430.log
```

## File Structure

```
.
├── CMakeLists.txt              # Build configuration
├── PDO_MAPPING.md              # ✨ Dokumentasi lengkap PDO mapping
├── README_USAGE.md             # ✨ Usage guide (file ini)
├── LC10E V1.04.xml             # ESI file dari vendor
├── src/
│   ├── main.cpp                # ✨ Main program (updated PDO)
│   ├── diagnostic.cpp          # ✨ Diagnostic tool (new)
│   ├── pdo_check.cpp           # PDO checking utility
│   └── sdo_trace.cpp           # SDO tracing utility
├── build/
│   ├── ethercat_servo          # Main executable
│   └── diagnostic              # Diagnostic executable
└── .deps/soem/                 # SOEM library
```

## PDO Structure (ESI XML V1.04)

### RxPDO (0x1600) - 13 bytes - Master → Slave

| Offset | Field                  | Type   | Size | Description                |
|--------|------------------------|--------|------|----------------------------|
| 0      | control_word           | UINT16 | 2    | Control Word (6040h)       |
| 2      | target_position        | INT32  | 4    | Target Position (607Ah)    |
| 6      | touch_probe_function   | UINT16 | 2    | Touch Probe Function (60B8h)|
| 8      | modes_of_operation     | UINT8  | 1    | Modes of Operation (6060h) |
| 9      | target_velocity        | INT32  | 4    | Target Velocity (60FFh)    |

### TxPDO (0x1A00) - 23 bytes - Slave → Master

| Offset | Field                        | Type   | Size | Description                     |
|--------|------------------------------|--------|------|---------------------------------|
| 0      | error_code                   | UINT16 | 2    | Error Code (603Fh)              |
| 2      | status_word                  | UINT16 | 2    | Status Word (6041h)             |
| 4      | position_actual_value        | INT32  | 4    | Position Actual (6064h)         |
| 8      | velocity_actual_value        | INT32  | 4    | Velocity Actual (606Ch)         |
| 12     | touch_probe_status           | UINT16 | 2    | Touch Probe Status (60B9h)      |
| 14     | touch_probe_pos1_value       | INT32  | 4    | Touch Probe Pos1 (60BAh)        |
| 18     | digital_inputs               | UINT32 | 4    | Digital Inputs (60FDh)          |
| 22     | modes_of_operation_display   | INT8   | 1    | Mode Display (6061h)            |

**Verifikasi:**
```bash
./build/verify_pdo_size
# Output:
#   OutputPDO: 13 bytes ✓ PASS
#   InputPDO:  23 bytes ✓ PASS
```

## CiA402 State Machine

State machine sequence untuk enable servo:

```
1. Shutdown (0x06)          → Ready to Switch On
2. Switch On (0x07)         → Switched On
3. Enable Operation (0x0F)  → Operation Enabled
```

Fault recovery:
```
Fault Reset (0x80)          → Clear fault, kembali ke awal
```

Quick stop:
```
Quick Stop (0x02)           → Motor melambat dengan ramp
Disable Voltage (0x00)      → Nonaktifkan motor
```

## Modes of Operation

| Value | Mode | Description                                    |
|-------|------|------------------------------------------------|
| 1     | PP   | Profile Position - posisi dengan profil        |
| 3     | PV   | Profile Velocity - kecepatan dengan profil     |
| 6     | HM   | Homing - mode referensi                        |
| 8     | CSP  | Cyclic Synchronous Position - posisi realtime  |
| 9     | CSV  | Cyclic Synchronous Velocity - kecepatan realtime|
| 10    | CST  | Cyclic Synchronous Torque - torsi realtime     |

## Troubleshooting

### Error: "No socket connection on eth0"
```bash
# Check interface name
ip link

# Pastikan interface aktif
sudo ip link set eth0 up

# Jalankan sebagai root
sudo ./ethercat_servo eth0
```

### Error: "No slaves found"
```bash
# Check kabel EtherCAT
# Check power servo
# Pastikan LED EtherCAT di servo menyala hijau

# Test dengan scan
sudo ./ethercat_servo eth0 --scan-only
```

### Error: "Slave not reaching Safe-Op"
```bash
# Check ALstatus code
sudo ./diagnostic eth0 --info

# Lihat error detail
sudo ./ethercat_servo eth0 --check-pdo
```

### Fault saat operasi
```bash
# Check error code
sudo ./diagnostic eth0 --monitor

# Baca parameter servo
sudo ./diagnostic eth0 --read-sdo

# Test dengan mode test
sudo ./diagnostic eth0 --test-csp
```

## Common Error Codes

| Code   | Description                          | Solution                          |
|--------|--------------------------------------|-----------------------------------|
| 0x0000 | No error                             | -                                 |
| 0x2310 | Over current                         | Check wiring, reduce current limit|
| 0x3210 | Over voltage                         | Check power supply voltage        |
| 0x3220 | Under voltage                        | Check power supply                |
| 0x4310 | Over temperature (motor)             | Let motor cool down               |
| 0x7320 | Position error too large (Er.B00)    | Check mechanical load, tuning     |
| 0x8180 | CAN/EtherCAT communication timeout   | Check cable, cycle time           |
| 0xFF01 | Encoder error                        | Check encoder connection          |

## Safety Notes

⚠️ **PERINGATAN:**
1. Pastikan area kerja aman sebelum enable motor
2. Motor akan bergerak otomatis saat program berjalan
3. Tekan Ctrl+C untuk emergency stop
4. Program akan melakukan graceful shutdown (quick stop → disable voltage)
5. Gunakan emergency stop button hardware jika tersedia

## Development Notes

### Mengubah gerakan dalam main.cpp

Edit bagian kontrol posisi di `src/main.cpp`:

```cpp
// Contoh: ubah jarak gerakan
const int32_t MOVE_DISTANCE = 200000;  // 200k pulses

// Contoh: ubah periode
const int HALF_PERIOD = 5000;  // 5 detik per fase

// Atau buat logika custom
if (cycle < 3000) {
    output->target_position = home_position + 50000;
} else if (cycle < 6000) {
    output->target_position = home_position - 50000;
} else {
    output->target_position = home_position;
}
```

### Mengubah konfigurasi SDO

Edit fungsi `lichuan_po2so_config()` atau `diagnostic_config()`:

```cpp
// Contoh: ubah max velocity
u32val = 5000000;  // 5M pulses/s
ecx_SDOwrite(context, slave, 0x607F, 0x00, FALSE, sz, &u32val, EC_TIMEOUTRXM);

// Contoh: ubah gear ratio 2:1
u32val = 2;
ecx_SDOwrite(context, slave, 0x6091, 0x01, FALSE, sz, &u32val, EC_TIMEOUTRXM);
u32val = 1;
ecx_SDOwrite(context, slave, 0x6091, 0x02, FALSE, sz, &u32val, EC_TIMEOUTRXM);
```

## References

- **PDO_MAPPING.md** - Dokumentasi lengkap struktur PDO dan field-fieldnya
- **LC-E_Series_Servo_User_Manual.md** - Manual servo Lichuan LC-E
- **DIAGNOSIS.md** - Guide troubleshooting
- **SOEM_SETUP.md** - Setup SOEM library
- [SOEM GitHub](https://github.com/OpenEtherCATsociety/SOEM)
- [CiA402 Standard](https://www.can-cia.org/can-knowledge/canopen/cia402/) - CANopen device profile for drives

## Version History

| Date       | Version | Changes                                      |
|------------|---------|----------------------------------------------|
| 2024-09-24 | 2.0     | Update PDO structure dari ESI XML V1.04      |
|            |         | Tambah program diagnostic lengkap            |
|            |         | Tambah dokumentasi PDO_MAPPING.md            |
|            |         | Verifikasi struktur 13/23 bytes              |
| 2024-xx-xx | 1.0     | Initial version                              |

## Support

Untuk pertanyaan atau issue:
1. Check dokumentasi di PDO_MAPPING.md
2. Run diagnostic tool untuk troubleshooting
3. Check log file untuk detail error
4. Lihat manual servo LC-E_Series_Servo_User_Manual.md

---

**Status Build:** ✅ Compiled successfully  
**PDO Verification:** ✅ 13 bytes (Output) / 23 bytes (Input)  
**Last Updated:** 2024-09-24
