# Instalasi SOEM

Proyek ini menggunakan SOEM **v2.0.0**, commit
`304d1c05eab77dc0d426f1a5cf09c8cc7dc03713`, dari
[repositori resmi SOEM](https://github.com/OpenEtherCATsociety/SOEM/tree/v2.0.0).
Header bawaan proyek cocok dengan versi ini, termasuk konfigurasi ukuran
struktur di `ec_options.h`; satu perbedaan pada `ec_eoe.h` hanya komentar.

Library statis dipasang lokal di `.deps/soem/lib/libsoem.a`, bersama header,
konfigurasi CMake, dan `LICENSE.md`. Folder `.deps/` tidak disimpan di Git.

Untuk memasang ulang di Linux, jalankan dari direktori utama proyek
(memerlukan Git, compiler C/C++, dan CMake minimal 3.28 untuk membangun SOEM):

```bash
git clone --depth 1 --branch v2.0.0 https://github.com/OpenEtherCATsociety/SOEM.git .deps/SOEM-source
cmake -S .deps/SOEM-source -B .deps/SOEM-build \
  -DSOEM_BUILD_SAMPLES=OFF -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/.deps/soem"
cmake --build .deps/SOEM-build -j2
cmake --install .deps/SOEM-build
cmake -S . -B build
cmake --build build -j2
```

Jika source sudah tersedia, gunakan direktori tersebut dan lewati `git clone`.
Instalasi lokal tidak memerlukan `sudo`. Untuk instalasi SOEM lain yang cocok,
berikan `-Dsoem_DIR=/lokasi/soem/cmake` saat konfigurasi proyek.

Pemeriksaan executable tanpa membuka koneksi ke servo:

```bash
./build/ethercat_servo
```

Tanpa argumen, program menampilkan petunjuk penggunaan dan keluar dengan kode 1.
Build berhasil belum membuktikan komunikasi EtherCAT atau kontrol motor.
Untuk memeriksa koneksi dan identitas slave tanpa enable motor:

```bash
sudo ./build/ethercat_servo enp2s0 --scan-only
```

Ganti antarmuka sesuai kabel ke drive. Mode ini membuka raw socket dan
menjalankan scan SOEM yang menyiapkan bus ke INIT/PRE-OP, kemudian menutup
koneksi. Mode ini tidak mengonfigurasi parameter servo, memetakan PDO,
meminta Operational, atau mengirim perintah enable/target posisi. Jangan
jalankan bersamaan dengan master lain pada bus yang sama.

## Uji dengan XML LC10E V1.04 (identitas 766/402/204)

File `LC10E V1.04.xml` yang diberikan pengguna menetapkan fixed PDO 1702/1B02
berukuran **15/28 byte**, dengan jumlah entri 6/9. Profil ini dikompilasi ke
program; opsi berikut bukan pembaca XML umum untuk drive lain.

```bash
cd /home/robotwelder/Unduhan/EtherCatLichuan-main
cmake -S . -B build
cmake --build build -j2
set -o pipefail
sudo ./build/ethercat_servo enp2s0 --check-pdo-esi 2>&1 | tee pdo-check-esi.log
```

Pemeriksaan identitas termasuk revision harus cocok persis, begitu pula
assignment dan jumlah entri pada drive. Profil menggantikan hanya penemuan
mapping SOEM selama konfigurasi diagnostik ini. Tidak ada perubahan EEPROM
atau penulisan SDO/assignment. SM/FMMU tetap dibuat oleh SOEM dan diverifikasi;
ukuran yang berbeda menghentikan uji, tanpa menambah padding atau menggeser
control word. Descriptor tidak diambil dari respons SDO 2 byte yang invalid.

Control word selalu nol, target posisi mengikuti feedback, mode dan batas
kecepatan dipertahankan, target torsi/probe nol. SYNC0 diuji pada 1 ms selama
5000 siklus di EtherCAT OP, lalu kembali INIT. OP bus bukan servo enable.
Brake dengan PSU manual boleh tetap tidak diberi 24 V untuk pengujian ini.
Program tidak dapat mengendalikan atau memastikan status brake eksternal.

Untuk mengulang uji komunikasi setelah operator melepas brake secara manual:

```bash
sudo ./build/ethercat_servo enp2s0 --check-pdo-esi --confirm-brake 2>&1 | tee pdo-brake-check.log
```

Sebelum koneksi EtherCAT dibuka, program menanyakan apakah brake sudah dilepas
dengan memberi 24 V. Ketik tepat `REM LEPAS` lalu Enter untuk melanjutkan.
Enter kosong, jawaban lain, EOF, Ctrl+C, atau SIGTERM membatalkan; tidak
ada konfirmasi yang disimpan untuk eksekusi berikutnya. Opsi ini tetap memakai
uji PDO tanpa enable/gerak. Konfirmasi adalah pernyataan operator, bukan
feedback sensor brake. Pastikan beban aman sebelum melepas brake.

Jawaban dibaca dari terminal yang terhubung ke stdin (namanya dicetak), sehingga
tidak bergantung pada kesamaan stdin dengan `/dev/tty`. Stdout dapat dicatat
lewat `tee`; setiap baris log langsung ditampilkan dan hasil konfirmasi di-flush.
Akhiri `REM LEPAS` dengan Enter (LF maupun CR diterima). Jika masih ada proses
lama yang menunggu, batalkan dengan Ctrl+C sebelum menjalankan binary baru.
Tanpa stdin terminal interaktif program menolak; input pipe/file tidak dapat
menggantikan konfirmasi. Exit code pembatalan adalah 2. Pada mode gerak lama
tanpa opsi dan mode `diagnostic` yang dapat enable (termasuk `--monitor`),
konfirmasi wajib meskipun opsi `--confirm-brake` tidak diberikan. Konfirmasi
ini tidak memvalidasi mapping atau algoritme gerak lama.

Harapkan `Output=15 Input=28`, `IOmap: total=44 expected=44` (43 byte PDO
ditambah 1 byte status mailbox SOEM), readback SM2/SM3 sesuai, DC 1000000 ns,
WKC 3/3 dan `Hasil=LULUS`. Build/simulasi belum membuktikan hasil hardware.
Gunakan opsi lengkap; mode gerak default dan executable `diagnostic` belum
menggunakan profil ini dan dapat enable motor.

## Uji motor berputar

Mode baru `--test-rotate` memakai profil XML yang sama dan meminta konfirmasi
brake. Mode ini khusus poros bebas tanpa beban yang telah dikonfirmasi pengguna.
Lihat [langkah dan batas uji putar](MOTION_TEST.md).

## Uji PDO dan Distributed Clock tanpa enable

```bash
cd /home/robotwelder/Unduhan/EtherCatLichuan-main
sudo ./build/ethercat_servo enp2s0 --check-pdo
```

Mode ini khusus satu drive dengan VendorID `0x766` dan ProductCode `0x402`.
Program menunggu PRE-OP dan membaca assignment **aktif** `1C12h/1C13h`, lalu
mencetak semua entri dari PDO yang ditunjuk. Nomor, jumlah entri, offset,
dan ukuran PDO dibaca dari drive, bukan diasumsikan `1701h/1B01h` atau 8/24 byte.
Log hardware sebelumnya menunjukkan assignment `1702h/1B02h` dan empat entri
pada PDO tidak aktif `1701h`, berbeda dari tabel manual yang dipakai kode lama.
XML sekarang menunjukkan entri keempat 1701 adalah 60FE:01 (output fisik).
Uji tidak memilih atau menulis output tersebut.

Sebelum mengirim data, program memvalidasi ukuran/subindex objek, alignment,
duplikasi, serta keberadaan `6040h/607Ah` pada output dan `603Fh/6041h/6064h`
pada input. Output yang belum dikenali menghentikan uji; input tambahan boleh
diabaikan. Uji mendukung satu PDO per arah dengan maksimum 32 entri/256 byte.
Ukuran dan pointer IOmap hasil SOEM diperiksa terhadap mapping yang dibaca,
lalu mapping dibaca ulang untuk memastikan tidak berubah selama setup.

Mode ini tidak menulis SDO, tidak mengganti assignment PDO, dan tidak memanggil
callback konfigurasi servo pada mode gerak. Jika `6060h` atau `607Fh` terdapat
dalam output, nilai awalnya dibaca melalui SDO dan dipertahankan dalam PDO.
Target kecepatan/torsi, offset kecepatan/torsi, dan probe diisi nol. DC SYNC0
diaktifkan dengan periode 1 ms dan registernya dibaca kembali.

Pengujian meminta EtherCAT Operational tetapi mengirim control word `0x0000`
di setiap frame. Target posisi mengikuti posisi aktual; tidak ada fault reset
atau perintah enable. Setelah sekitar 5000 siklus (nominal 5 detik, di luar
setup), program keluar otomatis. Ctrl+C membatalkan pengujian. Program mencoba
keluar dari OP, menonaktifkan SYNC0, dan mengembalikan bus ke INIT.
Assignment PDO tetap seperti sebelum uji. Kegagalan pemulihan state dilaporkan
dan menghasilkan exit code bukan nol.

Hasil yang diharapkan: ukuran Output/Input sesuai entri aktual,
`DC SYNC0 aktif: 1000000 ns`,
`EtherCAT OP tercapai`, WKC aktual sama dengan expected, lalu `Hasil=LULUS`.
WKC dihitung dari konfigurasi grup: `outputsWKC * 2 + inputsWKC` (biasanya 3
untuk satu drive). Fault, kehilangan WKC/OP, mapping salah, dan status enabled
menghentikan pengujian. Tanpa keberhasilan penuh hasilnya `BELUM LULUS`.

Jika transisi INIT ke PRE-OP gagal, log kini mencetak alamat slave dan
`write WKC` untuk permintaan state langsung ke tiap slave. Status aktual tetap
ditunggu meskipun balasan tulis hilang. Bit error AL ikut diperiksa; PRE-OP+ERROR
tidak dianggap PRE-OP berhasil. Saat gagal, register station address, AL control,
AL status, dan AL status code dibaca dengan WKC masing-masing. `read WKC <= 0`
berarti pembacaan gagal, bukan bukti register bernilai nol.
Log ini membedakan kegagalan akses register dari state yang belum
berubah; penyebab pada perangkat tetap memerlukan hasil uji hardware.

Timer memakai deadline absolut CLOCK_MONOTONIC dengan koreksi fase DC sederhana.
Statistik keterlambatan dilaporkan; pengujian ini bukan pembuktian kemampuan
real-time atau kelayakan kontrol gerakan. Implementasi mengacu pada
[contoh resmi SOEM v2.0.0](https://github.com/OpenEtherCATsociety/SOEM/blob/v2.0.0/samples/simple_ng/simple_ng.c)
dan manual LC-E bab 6.1, 6.3, 6.4.2.

Uji simulasi tanpa perangkat/root (dari folder proyek):

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Iinclude/osal -Iinclude/oshw \
  src/pdo_check.cpp src/lc10e_esi.cpp src/motion_trial.cpp tests/pdo_check_test.cpp -o build/pdo_check_test
./build/pdo_check_test
```

49 skenario simulasi mencakup profil ESI 15/28 byte beserta kegagalannya, tabel manual grup 1 dan 2, variasi sintetis
empat entri dengan offset berubah, kegagalan pembacaan, perubahan mapping,
objek/ukuran yang salah, dan gangguan komunikasi. Setiap byte output diperiksa
terhadap fixture yang ditulis terpisah, termasuk control word nol, posisi awal
non-nol, mode dan batas kecepatan yang dipertahankan. Tidak ada penulisan SDO
yang diizinkan oleh mock. Fixture bukan hasil pembacaan lengkap firmware drive;
keberhasilan perangkat fisik tetap harus dibuktikan dari log uji perangkat.

### Diagnostik respons SDO

Setiap pembacaan scalar oleh `--check-pdo` mencetak index/subindex permintaan
dan balasan, command SDO, WKC pengiriman/penerimaan mailbox, ukuran data, dan
counter mailbox. Jika balasan tidak cocok atau ukurannya salah, 16 byte awal
mailbox TX/RX dicetak dan uji dihentikan. Data 2 byte tidak dipanjangkan menjadi
descriptor PDO 4 byte dengan asumsi nilai yang hilang.

SOEM v2.0.0 memeriksa index respons upload, tetapi tidak memeriksa subindex pada
jalur ini. Pemeriksaan tambahan membandingkan header respons terhadap argumen
permintaan yang disimpan terpisah. Wrapper linker hanya merekam mailbox; tidak
mengubah isi paket, menambahkan retry, atau mengganti library yang terpasang.
Log lama yang hanya berisi `ukuran 2, expected 4` belum membuktikan apakah
firmware mengirim descriptor pendek atau respons mailbox tidak sesuai.

Uji tambahan memakai fungsi SDO dan mailbox dari library SOEM terpasang;
hanya akses register EtherCAT diganti oleh simulasi, tanpa membuka NIC:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Iinclude/osal -Iinclude/oshw \
  src/sdo_trace.cpp tests/sdo_trace_test.cpp .deps/soem/lib/libsoem.a \
  -Wl,--wrap=ecx_mbxsend -Wl,--wrap=ecx_mbxreceive \
  -Wl,--wrap=ecx_FPWR -Wl,--wrap=ecx_FPRD -lpthread -lrt -o build/sdo_trace_test
./build/sdo_trace_test
```

10 kasus mencakup respons valid 1/2/4 byte, descriptor pendek, index/subindex
salah, SDO abort, header terpotong, timeout, dan kegagalan kirim. Kasus subindex
salah membuktikan library menerima balasan tersebut, sedangkan guard menolaknya.
Ini membuktikan perilaku parser, bukan penyebab pasti masalah drive fisik.

Menjalankan program dengan argumen antarmuka tanpa opsi diagnostik akan menjalankan logika yang
mengaktifkan servo dan mengirim target gerakan otomatis.


Verifikasi profil terhadap XML dan integrasi dengan mapper SOEM asli (tanpa NIC):

```bash
python3 tests/verify_lc10e_xml.py
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude -Iinclude/osal -Iinclude/oshw \
  src/lc10e_esi.cpp tests/esi_map_test.cpp .deps/soem/lib/libsoem.a \
  -Wl,--wrap=ecx_readPDOmap -Wl,--wrap=ecx_readPDOmapCA \
  -Wl,--wrap=ecx_statecheck -Wl,--wrap=ecx_eeprom2pdi \
  -Wl,--wrap=ecx_FPWR -Wl,--wrap=ecx_FPRD \
  -Wl,--wrap=ecx_SDOread -Wl,--wrap=ecx_SDOwrite \
  -lpthread -lrt -o build/esi_map_test
./build/esi_map_test
```

Uji integrasi memeriksa SM/FMMU hasil mapper library terpasang dalam empat
kombinasi: mailbox ada/tidak ada dan penemuan PDO biasa/Complete Access.
Validator produksi memeriksa total IOmap 44/43 byte dan offset; expected WKC
juga diperiksa. Seluruh
akses bus diganti mock; tidak memerlukan sudo atau menyentuh drive fisik.
