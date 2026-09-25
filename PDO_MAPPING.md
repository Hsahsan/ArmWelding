# Lichuan LC10E Servo Drive - PDO Mapping Documentation

## Overview

Dokumen ini menjelaskan struktur PDO (Process Data Object) untuk servo drive Lichuan LC10E V1.04 berdasarkan file ESI (EtherCAT Slave Information) XML resmi.

**Informasi Device:**
- Vendor: Shenzhen LC Electric Technology Co., Ltd
- Vendor ID: 0x00000766
- Product Code: 0x00000402
- Revision: 0x00000204
- Profile: CANopen CiA 402 (Motion Control)

## Default PDO Mapping

### RxPDO (0x1600) - Output PDO (Master → Slave)
Data yang dikirim dari master ke servo drive (13 bytes total)

| Entry | Index  | SubIndex | Bits | Bytes | Type  | Name                    | Description |
|-------|--------|----------|------|-------|-------|-------------------------|-------------|
| 1     | 0x6040 | 0        | 16   | 2     | UINT  | Control Word            | Status control dan command |
| 2     | 0x607A | 0        | 32   | 4     | DINT  | Target Position         | Posisi target (pulse) |
| 3     | 0x60B8 | 0        | 16   | 2     | UINT  | Touch Probe Function    | Konfigurasi touch probe |
| 4     | 0x6060 | 0        | 8    | 1     | USINT | Modes of Operation      | Mode operasi (PP, PV, CSP, CSV, CST) |
| 5     | 0x60FF | 0        | 32   | 4     | DINT  | Target Velocity         | Kecepatan target (rpm atau pulse/s) |

**Total Size: 104 bits = 13 bytes**

### TxPDO (0x1A00) - Input PDO (Slave → Master)
Data yang diterima dari servo drive ke master (23 bytes total)

| Entry | Index  | SubIndex | Bits | Bytes | Type  | Name                      | Description |
|-------|--------|----------|------|-------|-------|---------------------------|-------------|
| 1     | 0x603F | 0        | 16   | 2     | UINT  | Error Code                | Kode error aktif |
| 2     | 0x6041 | 0        | 16   | 2     | UINT  | Status Word               | Status servo drive |
| 3     | 0x6064 | 0        | 32   | 4     | DINT  | Position Actual Value     | Posisi aktual (pulse) |
| 4     | 0x606C | 0        | 32   | 4     | DINT  | Velocity Actual Value     | Kecepatan aktual |
| 5     | 0x60B9 | 0        | 16   | 2     | UINT  | Touch Probe Status        | Status touch probe |
| 6     | 0x60BA | 0        | 32   | 4     | DINT  | Touch Probe Pos1 Value    | Posisi capture probe 1 |
| 7     | 0x60FD | 0        | 32   | 4     | UDINT | Digital Inputs            | Status input digital |
| 8     | 0x6061 | 0        | 8    | 1     | SINT  | Modes of Operation Display| Mode operasi aktual |

**Total Size: 184 bits = 23 bytes**

## Control Word (0x6040) - Bits Definition

Control Word digunakan untuk mengontrol state machine servo drive (CiA 402).

| Bit | Name                  | Description |
|-----|-----------------------|-------------|
| 0   | Switch On             | Enable power stage |
| 1   | Enable Voltage        | Enable voltage pada motor |
| 2   | Quick Stop            | 0 = Quick stop aktif |
| 3   | Enable Operation      | Enable operasi |
| 4   | Operation Mode Specific | Tergantung mode (New setpoint, Halt, etc) |
| 5   | Operation Mode Specific | Tergantung mode |
| 6   | Operation Mode Specific | Tergantung mode |
| 7   | Fault Reset           | Reset error (rising edge) |
| 8   | Halt                  | Halt motion (untuk PV, PP mode) |
| 9-15| Reserved/Manufacturer | Reserved atau specific manufacturer |

### State Machine Commands

| State Transition | Control Word | Binary Pattern | Hex  |
|------------------|--------------|----------------|------|
| Shutdown         | 0bxxxx_xxx0_x110 | 0x0006     | 0x06 |
| Switch On        | 0bxxxx_xxx0_x111 | 0x0007     | 0x07 |
| Enable Operation | 0bxxxx_xxx0_1111 | 0x000F     | 0x0F |
| Disable Voltage  | 0bxxxx_xxx0_xx0x | 0x0000     | 0x00 |
| Quick Stop       | 0bxxxx_xxx0_x01x | 0x0002     | 0x02 |
| Fault Reset      | 0bxxxx_xxx1_xxxx | 0x0080+    | 0x80 |

## Status Word (0x6041) - Bits Definition

Status Word menunjukkan state servo drive saat ini.

| Bit | Name                    | Description |
|-----|-------------------------|-------------|
| 0   | Ready to Switch On      | 1 = Siap untuk switch on |
| 1   | Switched On             | 1 = Power stage aktif |
| 2   | Operation Enabled       | 1 = Operasi enabled |
| 3   | Fault                   | 1 = Ada fault/error |
| 4   | Voltage Enabled         | 1 = Voltage enabled |
| 5   | Quick Stop              | 0 = Quick stop aktif |
| 6   | Switch On Disabled      | 1 = Switch on disabled |
| 7   | Warning                 | 1 = Ada warning |
| 8   | Manufacturer Specific   | Specific untuk Lichuan |
| 9   | Remote                  | 1 = Remote mode aktif |
| 10  | Target Reached          | 1 = Target tercapai |
| 11  | Internal Limit Active   | 1 = Internal limit aktif |
| 12-13| Operation Mode Specific| Tergantung mode operasi |
| 14-15| Manufacturer Specific  | Specific untuk Lichuan |

### State Detection

| State               | Status Word Pattern | Mask  | Value |
|---------------------|---------------------|-------|-------|
| Not Ready to Switch On | xxxx_xxxx_x0xx_0000 | 0x004F| 0x0000|
| Switch On Disabled  | xxxx_xxxx_x1xx_0000 | 0x004F| 0x0040|
| Ready to Switch On  | xxxx_xxxx_x01x_0001 | 0x006F| 0x0021|
| Switched On         | xxxx_xxxx_x01x_0011 | 0x006F| 0x0023|
| Operation Enabled   | xxxx_xxxx_x01x_0111 | 0x006F| 0x0027|
| Quick Stop Active   | xxxx_xxxx_x00x_0111 | 0x006F| 0x0007|
| Fault Reaction Active| xxxx_xxxx_x0xx_1111| 0x004F| 0x000F|
| Fault               | xxxx_xxxx_x0xx_1000 | 0x004F| 0x0008|

## Modes of Operation (0x6060)

Mode operasi yang didukung oleh LC10E:

| Value | Mode | Description |
|-------|------|-------------|
| 1     | PP   | Profile Position - Posisi dengan profil kecepatan/akselerasi |
| 3     | PV   | Profile Velocity - Kecepatan dengan profil akselerasi |
| 6     | HM   | Homing - Mode homing/referensi |
| 8     | CSP  | Cyclic Synchronous Position - Posisi cyclic realtime |
| 9     | CSV  | Cyclic Synchronous Velocity - Kecepatan cyclic realtime |
| 10    | CST  | Cyclic Synchronous Torque - Torsi cyclic realtime |

## Contoh Struktur C/C++

### RxPDO Structure (Output)
```cpp
struct __attribute__((packed)) OutputPDO {
    uint16_t control_word;          // 0x6040 - Control Word
    int32_t  target_position;       // 0x607A - Target Position
    uint16_t touch_probe_function;  // 0x60B8 - Touch Probe Function
    uint8_t  modes_of_operation;    // 0x6060 - Modes of Operation
    int32_t  target_velocity;       // 0x60FF - Target Velocity
};
// Total: 13 bytes
```

### TxPDO Structure (Input)
```cpp
struct __attribute__((packed)) InputPDO {
    uint16_t error_code;              // 0x603F - Error Code
    uint16_t status_word;             // 0x6041 - Status Word
    int32_t  position_actual_value;   // 0x6064 - Position Actual Value
    int32_t  velocity_actual_value;   // 0x606C - Velocity Actual Value
    uint16_t touch_probe_status;      // 0x60B9 - Touch Probe Status
    int32_t  touch_probe_pos1_value;  // 0x60BA - Touch Probe Pos1 Value
    uint32_t digital_inputs;          // 0x60FD - Digital Inputs
    int8_t   modes_of_operation_display; // 0x6061 - Modes of Operation Display
};
// Total: 23 bytes
```

## Contoh Penggunaan

### 1. Inisialisasi dan Enable Servo

```cpp
OutputPDO output;
memset(&output, 0, sizeof(output));

// Set mode CSP (Cyclic Synchronous Position)
output.modes_of_operation = 8;

// State machine sequence
output.control_word = 0x0006;  // Shutdown
// ... tunggu status = Ready to Switch On
output.control_word = 0x0007;  // Switch On
// ... tunggu status = Switched On
output.control_word = 0x000F;  // Enable Operation
// ... tunggu status = Operation Enabled
```

### 2. Motion Control (CSP Mode)

```cpp
// Gerakan posisi
output.target_position = 10000;  // 10000 pulses
output.control_word = 0x000F;     // Operation Enabled

// Monitor posisi aktual
if (input.status_word & 0x0400) {  // Bit 10: Target Reached
    printf("Target position reached!\n");
}
```

### 3. Velocity Control (CSV Mode)

```cpp
// Set mode CSV
output.modes_of_operation = 9;

// Set kecepatan target
output.target_velocity = 3000;  // rpm atau pulse/s (tergantung konfigurasi)
output.control_word = 0x000F;
```

### 4. Error Handling

```cpp
if (input.status_word & 0x0008) {  // Bit 3: Fault
    printf("Error detected! Code: 0x%04X\n", input.error_code);
    
    // Reset fault
    output.control_word = 0x0080;  // Fault Reset
    usleep(10000);  // 10ms
    output.control_word = 0x0000;
}
```

## Alternative PDO Mappings

Device ini juga mendukung PDO mapping alternatif:

### RxPDO Alternatives
- **0x1701** (258th) - 4 entries dengan Physical Outputs
- **0x1702** (259th) - 6 entries dengan tambahan Digital Outputs
- **0x1703** (260th) - 7 entries dengan Profile Velocity
- **0x1704** (260th) - 10 entries mapping extended
- **0x1705** (260th) - 9 entries mapping extended

### TxPDO Alternatives
- **0x1B01** (258th) - Dengan tambahan Following Error Value
- **0x1B02** (259th) - Dengan tambahan Torque dan Digital Inputs
- **0x1B03** (260th) - Extended dengan Touch Probe
- **0x1B04** (261th) - Full extended dengan semua field

Untuk menggunakan mapping alternatif, perlu konfigurasi SDO 0x1C12 (RxPDO assign) dan 0x1C13 (TxPDO assign).

## Distributed Clock (DC) Configuration

LC10E mendukung DC Synchronization untuk presisi timing yang tinggi:

```cpp
// DC Mode configuration
// AssignActivate: 0x0300 (SYNC0 event)
// Cycle Time: Sesuai dengan cycle time master (misal: 1ms = 1000000 ns)
// Shift Time: 0 (atau disesuaikan untuk kompensasi delay)
```

## Notes dan Best Practices

1. **PDO Size**: Pastikan IOMap size di SOEM sesuai (13 bytes output, 23 bytes input)
2. **Alignment**: Gunakan `__attribute__((packed))` untuk struct agar tidak ada padding
3. **Byte Order**: Little-endian (LSB first) untuk semua data multi-byte
4. **Update Rate**: Minimum cycle time sesuai spesifikasi device (biasanya 1ms)
5. **State Machine**: Selalu ikuti urutan state transition CiA 402
6. **Error Handling**: Monitor bit Fault pada Status Word dan Error Code
7. **Target Reached**: Cek bit 10 pada Status Word untuk konfirmasi posisi tercapai
8. **Mode Switch**: Ubah mode operasi hanya dalam state "Switch On" atau lebih rendah

## References

- File ESI: `LC10E V1.04.xml`
- Standard: IEC 61800-7-201 (CiA 402 CANopen device profile for drives and motion control)
- SOEM Documentation: https://github.com/OpenEtherCATsociety/SOEM
- Lichuan LC-E Series User Manual: `LC-E_Series_Servo_User_Manual.md`

## Revision History

| Date | Version | Description |
|------|---------|-------------|
| 2024-09-24 | 1.0 | Initial documentation from ESI XML V1.04 |
