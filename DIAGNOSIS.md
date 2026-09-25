# Hasil pembacaan hardware: kegagalan introspeksi PDO

Sumber bukti: log `--check-pdo` yang diberikan pengguna. Tidak ada hasil uji
hardware baru yang diperoleh langsung oleh agen (sudo memerlukan password).

## Terverifikasi dari log

- Vendor ID `0x00000766`, product code `0x00000402`, EEPROM revision `0x00000204`.
  Pengguna mengonfirmasi model pada label **LC10E-200W**. Nama EEPROM hanya
  `EtherCAT Driver`; revision EEPROM tidak otomatis sama dengan versi firmware
  aplikasi atau software yang ditampilkan panel.
- Tampilan panel yang dilaporkan pengguna: `_10ry`. Berdasarkan tabel manual
  bagian 5.1.3, bila digit tersebut adalah kolom status/mode, `1` berarti INIT,
  `0` berarti belum ada mode operasi, dan `Ry` berarti drive ready. INIT setelah
  uji konsisten dengan cleanup program yang mengembalikan bus ke INIT.
- Slave mencapai PRE-OP (`0x02`), AL status `0x0000`. `write WKC=0` menunjukkan
  permintaan tulis itu tidak diakui, tetapi pembacaan state/SDO berikutnya
  berhasil. Log tidak membuktikan penyebab hilangnya acknowledgement tersebut.
- Upload `6041:00`, `603F:00`, `6064:00`, `6061:00` menghasilkan ukuran masing-masing
  2, 2, 4, dan 1 byte, dengan index/subindex/counter respons sesuai permintaan.
- Assignment aktif `1C12:01=1702`, `1C13:01=1B02`.
- Jumlah entri yang dilaporkan: `1702:00=6`, `1B02:00=9`.
- Balasan pertama kedua PDO memiliki command `0x4B`, index/subindex/counter
  yang benar, dengan dua byte data `00 00`.

Contoh mailbox RX untuk `1702:01`:

```text
0A 00 00 00 00 13 00 30 4B 02 17 01 00 00 00 00
```

Dekode: panjang payload mailbox 10 byte; tipe CoE/counter 1; service SDO
response; command upload expedited `4B` menyatakan 2 byte; index `1702`,
subindex `01`; nilai `0000`. Dua byte terakhir berada dalam slot data 4 byte
tetapi bukan data valid menurut command tersebut. Respons `1B02:01` memiliki
format sama dengan counter 5. Keduanya bukan SDO abort (`80`).

## Kesimpulan dan batas bukti

SOEM mengartikan panjang respons ini secara konsisten dengan command pada
mailbox. Tidak ada bukti bahwa masalah ini disebabkan salah subindex, counter,
endianness, atau rem motor. Perbaikan guard subindex tetap berguna, tetapi
bukan penjelasan untuk log hardware ini.

Descriptor mapping standar memerlukan index, subindex, dan panjang bit dalam
32 bit. Respons 16 bit bernilai nol tidak menyediakan descriptor tersebut.
Mengubah expected size menjadi 2 atau mengisi sisanya dengan nol tidak
menghasilkan mapping yang dapat dipakai. Tabel manual lokal berbeda dari
perangkat (misalnya jumlah entri RPDO1702); jangan mengasumsikan offsetnya.

Log belum menentukan apakah ini keterbatasan pembacaan fixed PDO pada firmware,
perbedaan dokumentasi/perangkat, atau kondisi lain. Tidak ada dasar untuk
menyatakan drive rusak atau mengharuskan upgrade firmware.

SYNC0 belum diaktifkan, PDO belum dikirim, dan mode diagnostik tidak menulis
assignment, control word enable, fault reset, atau parameter mekanik.
`SW=0000` sendiri tidak membuktikan servo ready atau main power siap.

## XML yang sekarang tersedia

File pengguna `LC10E V1.04.xml` cocok dengan ketiga ID di atas. Fixed PDO-nya:

| PDO | Objek berurutan (bit) | Ukuran |
| --- | --- | --- |
| 1702 | 6040:00 (16), 607A:00 (32), 60B8:00 (16), 6071:00 (16), 607F:00 (32), 6060:00 (8) | 15 byte |
| 1B02 | 603F:00 (16), 6041:00 (16), 6064:00 (32), 6077:00 (16), 60F4:00 (32), 60B9:00 (16), 60BA:00 (32), 60BC:00 (32), 60FD:00 (32) | 28 byte |

Jumlah 6/9 cocok dengan log. SM2: alamat 1200, control 64; SM3: alamat 1300,
control 20. Angka control/alamat dalam heksadesimal. Asumsi padding 26/24 byte
di kode sebelumnya tidak sesuai XML. Ukuran register SM saja tidak membuktikan
offset tiap objek. Klaim bahwa descriptor dipecah menjadi dua UINT16 berurutan
juga belum terbukti oleh log yang diberikan.

Opsi baru `--check-pdo-esi` menggunakan profil tetap dari XML tersebut setelah
memeriksa identitas/revision, assignment aktif 1702/1B02, dan jumlah entri 6/9.
Descriptor fixed PDO diambil dari profil, tanpa merekonstruksi balasan 2 byte.
SOEM tetap membuat konfigurasi SM/FMMU; hasil ukuran, offset, dan readback SM
harus sesuai. Control byte SM2 tetap 64 sesuai XML (tidak diganti 24).
Assignment dan jumlah entri diperiksa ulang sebelum pertukaran PDO.
Mode `--check-pdo` tetap membaca descriptor melalui SDO dan menolak data pendek.
Keduanya tidak menulis assignment, enable, atau fault reset.

Build dan uji tanpa NIC lulus: 49 skenario bus simulasi, 10 kasus parser SDO
dengan SOEM terpasang, kesesuaian profil terhadap XML, serta mapper SOEM asli
(jalur biasa/Complete Access) menghasilkan SM/FMMU 15/28 byte dan expected WKC 3.
Ini belum membuktikan hasil uji drive fisik; diperlukan log `--check-pdo-esi`.
Kecocokan ESI tidak menjelaskan penyebab firmware mengirim descriptor pendek.

## Log lanjutan: ukuran PDO benar, pemeriksaan IOmap berhenti

Log pengguna menunjukkan `Output=15 Input=28`, lalu berhenti pada pemeriksaan
gabungan ukuran/offset sebelum readback SM dan SYNC0. Log lama belum mencetak
total IOmap, jumlah bit, atau offset; belum membuktikan komponen mana yang berbeda.

Bug pemeriksaan berhasil direproduksi dengan library SOEM terpasang: saat
fixture memiliki mailbox 128 byte seperti XML, mapper menghasilkan **44 byte**:
15 output + 28 input + **1 byte status mailbox**. Kode sebelumnya keliru meminta
total tepat 43 byte. Fixture integrasi sebelumnya mengisi SM mailbox tetapi
tidak mengisi `mbx_l/mbx_rl`, sehingga kasus ini terlewat.
Sesuai [sumber SOEM v2.0.0](https://github.com/OpenEtherCATsociety/SOEM/blob/v2.0.0/src/ec_config.c),
nilai kembali mapper mencakup `mbxstatuslength` setelah data PDO.

Pemeriksaan sekarang menghitung status mailbox terpisah, memverifikasi ukuran
dan pointer slave/grup, jumlah bit, start bit, serta lokasi dan lookup status
mailbox. Tidak ada padding yang ditambahkan ke PDO. Log mencetak rincian semua
nilai itu. Untuk drive ini diharapkan `total=44 expected=44`,
`PDO=43 mailbox-status=1`, offset output/input 0/15 dan status mailbox 43.

Regresi lulus: 49 skenario diagnostik, termasuk penolakan total, offset, bit,
dan status mailbox salah; serta 4 kasus mapper SOEM asli (dengan/tanpa mailbox,
jalur biasa/Complete Access) menggunakan validator produksi yang sama.
Build berhasil. Uji fisik setelah koreksi masih perlu diulang; belum ada bukti
drive mencapai OP atau menyelesaikan 5000 siklus.

## Uji hardware berikutnya LULUS (log pengguna)

Setelah koreksi IOmap, pengguna melaporkan `Hasil=LULUS`: total 44 byte,
SM2 1200/15/64 dan SM3 1300/28/20 cocok saat readback, SYNC0 1 ms,
EtherCAT OP tercapai, 5000 siklus WKC 3/3, SW 0250, ERR 0000. Tidak ada
enable/fault reset; cleanup kembali PRE-OP lalu INIT berhasil. Posisi tercatat
26129–26130. Nilai SDO awal nol tidak lagi dipakai sebagai bukti posisi PDO.

Timing melaporkan 56 siklus terlambat >=1 ms dan maksimum 3974 us. Lulus ini
memverifikasi komunikasi selama pengujian tersebut; belum membuktikan timing
yang memadai untuk kontrol gerakan. Agen tidak menjalankan uji motor fisik.

Atas permintaan pengguna, ditambahkan konfirmasi pelepasan brake sebelum
koneksi bus: ketik `REM LEPAS` dari terminal, jawaban lain/EOF/interupsi
membatalkan. Opsi `--check-pdo-esi --confirm-brake` mengulangi uji yang sama
tanpa enable setelah konfirmasi. Jalur lama yang bisa enable juga wajib
meminta konfirmasi di awal. Build berhasil dan 23 kasus CLI/terminal lulus
dengan fungsi inisialisasi EtherCAT diganti stub, sehingga tidak membuka NIC.

## Konfirmasi rem tampak berhenti setelah pengetikan

Log pengguna berhenti pada prompt dan echo `REM LEPAS`; tidak ada pesan
penerimaan konfirmasi. Echo terminal belum membuktikan program telah menerima
seluruh baris. Penyebab tepat di terminal pengguna belum diobservasi langsung.

Regresi terminal berhasil mereproduksi hang kode lama bila stdin terhubung
ke PTY berbeda dari controlling terminal `/dev/tty`. Kode kini membuka ulang
terminal stdin dengan descriptor terpisah dan nonblocking, mencoba read sebelum
poll, serta menerima Enter LF/CR. Pipe/file tetap ditolak sebagai sumber jawaban.
Hasil konfirmasi di-flush segera; kedua executable menggunakan stdout per baris
agar kelanjutan log tampak saat memakai `tee`.

29 skenario CLI/terminal mencakup PTY terpisah, penerimaan CR dalam terminal raw,
EOF/Ctrl+C, dan bukti log diterima sebelum stub inisialisasi selesai menunggu.
Seluruh inisialisasi bus pada pengujian ini diganti stub, tanpa membuka socket.
Perubahan ini tidak menambahkan enable/gerakan ke `--check-pdo-esi`.

## Setup brake pengguna

Drive: LC10E-200W. Motor: LCMT-02SLR17ZB-60M00630B. Manual bagian 2.3
mengartikan `02` sebagai 0,2 kW, `R17` sebagai encoder absolut magnetik 17 bit,
dan `Z` sebagai motor dengan brake.

Brake mendapat 24 V dari PSU eksternal yang disambung manual, tidak dikendalikan
DO/BK drive; sekarang belum diberi tegangan. Brake boleh tetap engaged untuk
uji komunikasi tanpa enable. Pelepasan brake bukan solusi untuk respons SDO
berukuran salah. Program tidak memiliki feedback atau kendali atas PSU manual
ini. Uji gerak nanti memerlukan pengaturan urutan brake dan penopang beban;
manual bagian 4.4.2/6.2.3 menunjukkan pengendalian melalui relay dan timing BK.

Untuk tahap sekarang gunakan hanya opsi diagnostik lengkap. Jalur gerak default
di `src/main.cpp` dan executable `diagnostic` masih memakai asumsi mapping lama
dan dapat mengaktifkan motor; keduanya belum disesuaikan/divalidasi dengan XML.


## Permintaan uji putar

Pengguna mengonfirmasi poros bebas tanpa beban. Ditambahkan `--test-rotate`
untuk satu gerakan PP +32768 unit dari feedback PDO terbaru, speed 8192 unit/s,
accel/decel 16384 unit/s², torque cap maksimal 20%. Selalu meminta konfirmasi
rem sebelum membuka bus. Jalur diagnostik `--check-pdo-esi` tetap tanpa enable.
Rincian pemeriksaan, stop dan pemulihan parameter ada di `MOTION_TEST.md`.
Build dan 27 simulasi gerak, 49 simulasi diagnostik, 32 kasus CLI lulus.
Motor fisik belum dijalankan oleh agen; kelulusan gerak harus dilihat dari log
`--test-rotate` pengguna, bukan disimpulkan dari hasil komunikasi sebelumnya.
