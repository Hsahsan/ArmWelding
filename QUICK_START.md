# Quick Start Guide - Lichuan LC10E EtherCAT Servo

## ✅ Status Verifikasi

Berdasarkan test `--check-pdo`, servo Anda terdeteksi dengan konfigurasi:

```
✓ Interface: enp2s0 (PROMISC mode aktif)
✓ Vendor ID: 0x00000766
✓ Product Code: 0x00000402
✓ Revision: 0x00000204
✓ Model: LC10E-200W
✓ Slave ditemukan: 1 device
```

## ⚠️ PENTING: PDO Mapping Configuration

**Servo saat ini menggunakan mapping NON-DEFAULT:**

### Mapping Aktual di Servo (Hard-coded oleh pdo_check.cpp)
- **RxPDO 0x1702**: 26 bytes (8 entries dengan padding)
  ```
  Entry 1: 6040:00 (16-bit) Control Word
  Entry 2: 607A:00 (32-bit) Target Position  
  Entry 3: 60B8:00 (16-bit) Touch Probe Function
  Entry 4-7: Padding (0000:00, 32-bit each)
  Entry 8: Padding (0000:00, 16-bit)
  ```

- **TxPDO 0x1B02**: 24 bytes (8 entries)
  ```
  Entry 1: 603F:00 (16-bit) Error Code
  Entry 2: 6041:00 (16-bit) Status Word
  Entry 3: 6064:00 (32-bit) Position Actual
  Entry 4: 6077:00 (16-bit) Torque Actual
  Entry 5: 60F4:00 (32-bit) Position Deviation (Following Error)
  Entry 6: 60B9:00 (16-bit) Touch Probe Status
  Entry 7: 60BA:00 (32-bit) Touch Probe Pos1
  Entry 8: 60BC:00 (32-bit) Touch Probe Pos2
  ```

### Program Yang Harus Digunakan

Program `ethercat_servo` dan `diagnostic` **TIDAK KOMPATIBEL** dengan mapping aktual servo karena menggunakan struktur 13/23 bytes (default mapping 0x1600/0x1A00).

## 🚀 Solusi: Gunakan Program Existing yang Sudah Bekerja

Berdasarkan log, program dengan **hardcoded layout** sudah ada dan berfungsi. Gunakan executable yang sudah ada:

### Program yang Bekerja:

```bash
cd /home/robotwelder/Unduhan/EtherCatLichuan-main/build

# 1. Scan dan verifikasi koneksi
sudo ./ethercat_servo enp2s0 --scan-only

# 2. Check PDO mapping (sudah berhasil)
sudo ./ethercat_servo enp2s0 --check-pdo

# 3. Jalankan program utama (jika compatible)
sudo ./ethercat_servo enp2s0
```

**CATATAN:** Program `ethercat_servo` kemungkinan masih menggunakan struct lama (26/24 bytes) dari sebelumnya. Program baru yang kita buat (13/23 bytes) tidak akan berfungsi sampai:

1. **Opsi A**: Ubah PDO assignment di servo ke default (0x1600/0x1A00)
2. **Opsi B**: Update struct di code untuk match dengan mapping aktual (26/24 bytes)

## 📋 Opsi A: Mengubah ke Default PDO Mapping

Untuk menggunakan program baru (diagnostic tool dengan struktur 13/23 bytes), servo harus dikonfigurasi ulang ke default mapping.

### Langkah-langkah:

1. **Buat program konfigurasi PDO:**

```bash
cd /home/robotwelder/Unduhan/EtherCatLichuan-main
nano src/set_default_pdo.cpp
```

Isi dengan:

```cpp
#include <stdio.h>
#include "soem/ethercat.h"
#include "soem/soem.h"

ecx_contextt ecx_context;
char IOmap[4096];

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: sudo %s <interface>\n", argv[0]);
        return 1;
    }
    
    memset(&ecx_context, 0, sizeof(ecx_context));
    
    if (!ec_init(argv[1])) {
        printf("ec_init failed\n");
        return 1;
    }
    
    if (ec_config_init(FALSE) <= 0) {
        printf("No slaves found\n");
        ec_close();
        return 1;
    }
    
    printf("Setting default PDO mapping for slave 1...\n");
    
    // Disable PDO assignment temporarily
    uint8_t zero = 0;
    ecx_SDOwrite(&ecx_context, 1, 0x1C12, 0x00, FALSE, 1, &zero, EC_TIMEOUTRXM);
    ecx_SDOwrite(&ecx_context, 1, 0x1C13, 0x00, FALSE, 1, &zero, EC_TIMEOUTRXM);
    
    // Set RxPDO to 0x1600 (default)
    uint16_t rxpdo = 0x1600;
    if (ecx_SDOwrite(&ecx_context, 1, 0x1C12, 0x01, FALSE, 2, &rxpdo, EC_TIMEOUTRXM) <= 0) {
        printf("Failed to set RxPDO assignment\n");
    } else {
        printf("✓ RxPDO set to 0x1600\n");
    }
    
    // Set TxPDO to 0x1A00 (default)
    uint16_t txpdo = 0x1A00;
    if (ecx_SDOwrite(&ecx_context, 1, 0x1C13, 0x01, FALSE, 2, &txpdo, EC_TIMEOUTRXM) <= 0) {
        printf("Failed to set TxPDO assignment\n");
    } else {
        printf("✓ TxPDO set to 0x1A00\n");
    }
    
    // Enable PDO assignments
    uint8_t one = 1;
    ecx_SDOwrite(&ecx_context, 1, 0x1C12, 0x00, FALSE, 1, &one, EC_TIMEOUTRXM);
    ecx_SDOwrite(&ecx_context, 1, 0x1C13, 0x00, FALSE, 1, &one, EC_TIMEOUTRXM);
    
    printf("\nDefault PDO mapping configured.\n");
    printf("Power cycle the servo to apply changes.\n");
    
    ec_close();
    return 0;
}
```

2. **Compile:**

```bash
cd build
g++ -o set_default_pdo ../src/set_default_pdo.cpp -I../include -L../.deps/soem/lib -lsoem -lpthread -lrt
```

3. **Jalankan:**

```bash
sudo ./set_default_pdo enp2s0
```

4. **Power cycle servo** (matikan dan nyalakan power)

5. **Test dengan program baru:**

```bash
sudo ./diagnostic enp2s0 --info
```

## 📋 Opsi B: Update Code untuk Match Mapping Aktual (RECOMMENDED)

Ini lebih sederhana - update struct di code untuk match dengan mapping yang sudah aktif di servo (26/24 bytes).

**KEUNTUNGAN:**
- Tidak perlu reconfigure servo
- Tidak perlu power cycle
- Langsung bisa digunakan

**File yang perlu diupdate:**
1. `src/main.cpp` - Update struct OutputPDO dan InputPDO
2. `src/diagnostic.cpp` - Update struct yang sama

Mari kita lakukan ini!

## 🔧 Cara Test Koneksi Sekarang

Untuk saat ini, gunakan program yang sudah ada dan berfungsi:

```bash
# Test dasar - scan slave
sudo ./ethercat_servo enp2s0 --scan-only

# Check PDO mapping detail
sudo ./ethercat_servo enp2s0 --check-pdo 2>&1 | less
```

## 📊 Interface EtherCAT yang Terdeteksi

Sistem Anda memiliki interface:
- ✅ **enp2s0** - ACTIVE, PROMISC mode (untuk EtherCAT)
- ❌ **enp1s0** - DOWN, no carrier
- ❌ **wlp3s0** - WiFi (tidak untuk EtherCAT)

**Gunakan enp2s0 untuk semua command EtherCAT.**

## 🛠️ Troubleshooting

### Error: "Slave tidak mencapai Safe-Op"

Ini terjadi karena ketidakcocokan ukuran struct PDO. Solusi:
1. Ikuti Opsi B di atas untuk update struct
2. Atau reconfigure servo ke default PDO (Opsi A)

### Error: "ec_init gagal pada 'eth0'"

Interface salah. Gunakan `enp2s0`:
```bash
sudo ./ethercat_servo enp2s0
sudo ./diagnostic enp2s0 --info
```

### Servo tidak terdeteksi

```bash
# Check interface up
ip link show enp2s0

# Check kabel EtherCAT terhubung
# Check power servo ON
# Check LED EtherCAT di servo (harus hijau atau berkedip)
```

## 📝 Next Steps

1. **SEGERA**: Saya akan update code untuk match dengan mapping 26/24 bytes
2. Setelah update, rebuild: `cd build && make`
3. Test ulang: `sudo ./diagnostic enp2s0 --info`
4. Jika berhasil, lanjut test: `sudo ./diagnostic enp2s0 --monitor`

## 🎯 Summary

**Yang sudah berhasil:**
- ✅ Servo terdeteksi di enp2s0
- ✅ Communication EtherCAT OK
- ✅ SDO read/write berfungsi
- ✅ PDO mapping teridentifikasi (0x1702/0x1B02, 26/24 bytes)

**Yang perlu diperbaiki:**
- ⚠️ Struct PDO di code tidak match (13/23 vs 26/24 bytes)
- ⚠️ Program baru belum bisa masuk Safe-Op/Operational

**Solusi:**
- 🔧 Update struct di code ke 26/24 bytes (RECOMMENDED)
- 🔧 Atau reconfigure servo ke default 13/23 bytes

---

Saya akan segera membuat update untuk Opsi B agar program langsung bisa digunakan!
