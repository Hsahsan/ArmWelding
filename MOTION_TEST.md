# Uji putar terbatas LC10E

Untuk setup yang dikonfirmasi pengguna: LC10E-200W, motor
LCMT-02SLR17ZB-60M00630B, **poros bebas tanpa mekanisme/beban**,
brake 24 V dari PSU eksternal manual. Komunikasi PDO/DC sudah lulus dari log
pengguna. Mode baru ini belum diuji pada motor fisik oleh agen.

```bash
cd /home/robotwelder/Unduhan/EtherCatLichuan-main
set -o pipefail
sudo ./build/ethercat_servo enp2s0 --test-rotate 2>&1 | tee rotate-test.log
```

Program menjelaskan gerakan lalu meminta `REM LEPAS` + Enter sebelum membuka
koneksi bus. Konfirmasi selalu wajib pada `--test-rotate`; tidak perlu menambah
`--confirm-brake`. Pertahankan rem terbuka selama gerakan. Siapkan penghentian
daya/E-stop yang dapat dijangkau; Ctrl+C meminta quick stop melalui komunikasi.
Hilangnya komunikasi tidak dapat menjamin perintah stop diterima drive.

## Gerakan

- Satu target **+32768 unit posisi** dari posisi PDO aktual, tanpa kembali
  otomatis atau mengulangi gerakan. Kira-kira +90° untuk encoder R17 (17 bit)
  dengan gear elektronik 1:1; satuan yang benar-benar diperintahkan adalah
  unit posisi drive, bukan derajat.
- Mode **Profile Position (PP, 6060=1)**: drive membentuk profil internal,
  sesuai manual lokal bagian 7.6. Ini mengurangi ketergantungan bentuk lintasan
  terhadap jitter loop komputer, tetapi tidak menghilangkan kebutuhan bus sehat.
- Kecepatan 8192 unit/s; percepatan, perlambatan, dan quick-stop deceleration
  16384 unit/s². Perkiraan waktu gerak sekitar 4,5 detik ditambah setup/stop.
- Batas torsi 6072 paling tinggi 200 (20%); jika batas awal lebih rendah,
  batas tersebut dipertahankan. Batas kecepatan awal yang terlalu rendah
  membatalkan uji, bukan dinaikkan otomatis.

## Pemeriksaan dan penghentian

Identitas 766/402/204, assignment 1702/1B02, jumlah 6/9, PDO 15/28 byte,
SM/FMMU, DC 1 ms, dan WKC diverifikasi melalui setup yang sama dengan uji PDO.
Gear 6091 harus 1:1, offset 60B0 nol, polaritas posisi normal, dan software
limits 607D valid. Nilai gear, polaritas, offset dan batas posisi tidak diubah.

Parameter profil dibaca dahulu, setiap penulisan dibaca balik. Setelah masuk
OP, 100 frame WKC valid diperiksa dengan CW nol. Mode display 6061 harus PP.
Target awal memakai feedback PDO terbaru sebelum enable; tidak memakai nol
atau posisi SDO lama. Enable dilakukan bertahap dengan HALT tetap aktif,
kemudian satu setpoint absolut baru dipicu. Tidak ada fault reset/re-enable.

Uji berhenti pada fault/limit/following error, WKC salah, hilang OP/enable,
posisi keluar rentang (512 unit toleransi), jeda kontrol >20 ms, atau timeout.
Handshake setpoint harus diakui dan kembali nol. Sukses membutuhkan status
target-reached dan error posisi <=64 unit selama 200 ms, bukan hanya enable.

Sesudah selesai/gagal/Ctrl+C, program mengirim quick stop lalu disable secara
siklik dengan waktu terbatas, memeriksa feedback stabil dan bit enable padam.
Dalam PRE-OP, parameter yang disentuh dipulihkan dan diverifikasi: 6060, 607F,
6081, 6083, 6084, 6085, 605A, 6072. Jika stop atau PRE-OP tidak terkonfirmasi,
pemulihan parameter dilewati dan dilaporkan. Tidak ada save-to-flash/EEPROM.
Terakhir program meminta INIT. `Hasil=LULUS` hanya jika gerakan, stop, pemulihan
parameter dan cleanup berhasil. Simpan log jika hasilnya belum lulus.

## Validasi tanpa perangkat

Build berhasil; 27 skenario bus simulasi mode gerak, 49 skenario diagnostik
tanpa enable, dan 32 skenario CLI/terminal lulus. Kasus gerak mencakup fault,
hilang komunikasi/enable/OP, pembatalan, tidak ada acknowledge, timeout target,
overflow/batas posisi, posisi salah arah/kelewatan, parameter gagal, serta
pemulihan setelah tulis yang diterapkan tetapi tidak diakui.

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Iinclude/osal -Iinclude/oshw \
  src/pdo_check.cpp src/lc10e_esi.cpp src/motion_trial.cpp \
  tests/motion_trial_test.cpp -o build/motion_trial_test
./build/motion_trial_test
```

Mock mengganti seluruh akses bus dan waktu; tidak memerlukan sudo atau motor.
Jangan gunakan jalur gerak lama tanpa opsi atau executable `diagnostic` untuk
uji ini: keduanya masih menggunakan asumsi mapping lama. Gunakan `--test-rotate`.
