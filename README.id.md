**Bahasa Indonesia** | [English](README.md)

# RPCDroid

Modul root untuk Magisk, KernelSU, dan APatch yang mengirim aktivitas game Android ke Discord Rich Presence.

![Versi](https://img.shields.io/badge/versi-0.1.2-blue)
![Android](https://img.shields.io/badge/Android-10%2B-3DDC84)
![Root](https://img.shields.io/badge/root-Magisk%20%7C%20KernelSU%20%7C%20APatch-lightgrey)
![Daemon](https://img.shields.io/badge/daemon-C%20(NDK)-informational)
![Lisensi](https://img.shields.io/badge/lisensi-GPL--3.0-blue)

<!--
  Tambahkan screenshot di sini, misalnya:
  <p align="center">
    <img src="docs/screenshots/home.png" width="240" alt="Halaman Home">
    <img src="docs/screenshots/games.png" width="240" alt="Daftar game">
    <img src="docs/screenshots/discord.png" width="240" alt="Hasil di Discord">
  </p>
-->

## Ringkasan

Saat sebuah game dibuka di ponsel, status Discord akun menampilkan nama game, ikonnya, dan lama bermain. Status dihapus otomatis ketika game ditutup.

Fungsi tersebut dijalankan oleh sebuah daemon native (`rpcdroid`) yang berjalan sebagai service modul dan terhubung langsung ke Discord Gateway melalui WebSocket over TLS. Pengaturan dilakukan lewat WebUI yang dibuka dari aplikasi manajer root.

## Pendekatan

Modul ini tidak menggunakan service aplikasi Android atau izin aksesibilitas. Deteksi game dilakukan oleh daemon dengan membaca aplikasi foreground melalui `dumpsys`, dan koneksi ke Discord dikelola oleh daemon yang sama.

Pemakaian sumber daya daemon sekitar 1â€“2 MB RAM dengan beban CPU yang mendekati nol pada kondisi idle.

## Fitur

- Deteksi game yang sedang berjalan secara berkala (bawaan: setiap 3 detik).
- Database 470+ package game Android. Game yang terpasang di perangkat dipetakan otomatis saat instalasi.
- Dukungan ikon untuk game yang belum terdaftar di Discord (lihat [Mekanisme ikon](#mekanisme-ikon)).
- WebUI untuk mengatur token, mengaktifkan atau menonaktifkan game, mengubah nama tampilan, dan melihat log.
- Pemuatan ulang konfigurasi otomatis tanpa restart daemon.
- Binary untuk `arm64-v8a`, `armeabi-v7a`, dan `x86_64`.

> [!WARNING]
> Modul ini autentikasi ke Discord memakai **user token** akun, bukan token bot.
>
> - Token setara dengan kata sandi dan tidak boleh dibagikan. Token disimpan di `/data/adb/.config/drpc/token`, yang hanya dapat dibaca melalui akses root.
> - Ketentuan layanan Discord tidak mengizinkan otomasi akun pengguna (*self-bot*). Risiko terhadap akun ditanggung pengguna. Memakai akun sekunder dapat dipertimbangkan.

## Keterbatasan

- Memerlukan akses root. Modul tidak berfungsi pada perangkat tanpa root.
- Hanya game yang terdaftar di `gamelist.json` yang dilaporkan ke Discord.
- Deteksi bersifat polling, sehingga perubahan status dapat tertunda hingga sekitar satu interval (bawaan 3 detik).
- Bergantung pada Discord Gateway yang tidak didokumentasikan untuk akun pengguna. Perubahan di sisi Discord dapat memengaruhi fungsi modul sewaktu-waktu.
- Versi saat ini (0.1.2) masih awal dan belum diuji luas pada berbagai perangkat.

## Instalasi

### Persyaratan

- Android 10 atau lebih baru (API 29+)
- KernelSU, APatch, atau Magisk
- Akun Discord

### Langkah

1. Unduh `rpcdroid-v0.1.2.zip` dari halaman rilis.
2. Di aplikasi manajer root, buka **Modules**, pilih **Install from storage**, lalu pilih berkas zip tersebut.
3. Reboot perangkat setelah instalasi selesai.
4. Buka WebUI modul. Di KernelSU dan APatch, WebUI dibuka dari daftar modul. Pengguna Magisk memerlukan aplikasi tambahan, misalnya [KsuWebUI Standalone](https://github.com/KOWX712/KsuWebUIStandalone/releases).
5. Di menu **Settings**, isi Discord token lalu tekan **Save**. Cara memperolehnya dijelaskan di [bagian ini](#cara-mendapatkan-discord-token).
6. Di halaman **Home**, tekan **Start** jika status belum *Running*.
7. Jalankan game. Status Discord akan diperbarui otomatis.

### Pemecahan masalah

| Gejala | Pemeriksaan |
|---|---|
| Status tidak muncul | Pastikan daemon berstatus *Running* di halaman Home dan token sudah tersimpan. |
| Game tidak terdeteksi | Buka halaman **Games**, pastikan game ada di daftar dan togglenya aktif. |
| Ikon tidak tampil | Lihat [Mekanisme ikon](#mekanisme-ikon). Game yang belum terdaftar memerlukan URL ikon yang valid. |
| Penyebab tidak jelas | Periksa log di **Settings** atau berkas `/data/adb/.config/drpc/drpc.log`. |

### Melaporkan masalah

Saat membuka issue, sertakan informasi berikut agar masalah lebih mudah ditelusuri:

- Versi modul, versi Android, merek/tipe perangkat, dan arsitektur CPU
- Root solution yang dipakai beserta versinya (Magisk, KernelSU, atau APatch)
- Isi `drpc.log` (hapus token jika muncul) dan `status.json`
- Nama package game yang bermasalah
- Langkah untuk mereproduksi masalah

Jangan pernah menyertakan Discord token pada laporan.

---

# Dokumentasi teknis

## Cara kerja

```
WebUI  <-->  /data/adb/.config/drpc/  <-->  rpcdroid (daemon)  <-->  Discord Gateway
(atur)       (config, token, status)        (deteksi + koneksi)      (WSS, port 443)
```

WebUI dan daemon tidak berkomunikasi langsung. Keduanya berbagi berkas di `/data/adb/.config/drpc/`. WebUI menulis konfigurasi, daemon memantaunya dengan `inotify` dan memuat ulang saat ada perubahan, lalu menulis `status.json` yang dibaca kembali oleh WebUI.

### Alur dari instalasi hingga status tampil

1. **Instalasi.** `customize.sh` menyalin binary `rpcdroid` sesuai arsitektur CPU, lalu memindai `/data/data/<package>` untuk menemukan game terpasang. Hasilnya dicocokkan dengan `gamelist.txt` dan disusun menjadi `gamelist.json` beserta `app_id` tiap game.
2. **Konfigurasi.** Token yang diisi di WebUI ditulis ke berkas `token`.
3. **Koneksi.** Daemon membuka koneksi TLS ke `gateway.discord.gg:443` dan melakukan handshake WebSocket.
4. **Autentikasi.** Daemon menerima Opcode 10 (*Hello*), menjalankan thread heartbeat, lalu mengirim Opcode 2 (*Identify*).
5. **Deteksi.** Pada setiap interval, daemon membaca aplikasi foreground dan mencocokkannya dengan `gamelist.json`.
6. **Pengiriman.** Jika cocok dan game diaktifkan, daemon mengirim Opcode 3 (*Presence Update*) berisi nama game, waktu mulai (UNIX timestamp), dan `application_id`.

| Opcode | Arah | Fungsi |
|---|---|---|
| 10 Hello | Discord â†’ daemon | Menyampaikan interval heartbeat |
| 1 Heartbeat | Daemon â†’ Discord | Menjaga koneksi tetap aktif |
| 2 Identify | Daemon â†’ Discord | Autentikasi dengan token |
| 3 Presence Update | Daemon â†’ Discord | Mengirim aktivitas game |

## Mekanisme ikon

Ikon ditentukan melalui dua jalur, dengan urutan prioritas berikut.

**1. Game terdaftar di Discord.** Daemon memakai `app_id` resmi game tersebut (misalnya Mobile Legends, Genshin Impact, Arknights). Discord menampilkan ikon dari CDN-nya sendiri.

**2. Game belum terdaftar** (contoh: Monster Hunter Outlanders). Daemon memakai Application ID generik (`962990036020756480`) sebagai dasar. URL ikon dari konfigurasi, `gamelist.json`, atau Play Store dikonversi menjadi alamat `mp:external/...` melalui endpoint `external-assets` Discord. Permintaan HTTP POST ini dikirim langsung oleh daemon melalui TLS tanpa `curl`. Hasilnya dikirim ke gateway sebagai `large_image`.

Konversi pada jalur kedua diperlukan karena Discord menolak URL `https://` mentah yang dikirim langsung ke gateway, sehingga ikon tampil rusak.

## Tech stack

| Komponen | Teknologi | Peran |
|---|---|---|
| Daemon | C++ (main/config/gateway/log) + C (websocket, cJSON), Android NDK (Clang), mbedTLS 3.6.0, cJSON, inotify, Android Binder (BinderMonitor) | Koneksi Gateway, deteksi game berbasis event, pemrosesan JSON, hot-reload konfigurasi |
| WebUI | Vue 3 (Composition API), Vite 7, Pinia 2, Vue Router 4, Tailwind CSS 4, API `kernelsu` | Pengaturan token, daftar game, dan log |
| Instalasi dan service | POSIX shell, `pm`, `dumpsys` | `customize.sh`, `service.sh`, `action.sh`, `uninstall.sh` |
| Protokol | Discord Gateway API v10, WebSocket (RFC 6455) over TLS | Autentikasi dan pengiriman Rich Presence |

<details>
<summary><strong>Struktur proyek</strong></summary>

```
RPCDroid/
â”œâ”€â”€ build.sh                       # Builder utama seluruh modul
â”œâ”€â”€ README.md
â”œâ”€â”€ README_EN.md
â”œâ”€â”€ rpcdroid-v0.1.2.zip            # Paket modul siap flash
â”œâ”€â”€ .gitignore
â”‚
â”œâ”€â”€ daemon/                        # Source daemon (C, NDK)
â”‚   â”œâ”€â”€ main.c                     # Entry point dan hot-reload inotify
â”‚   â”œâ”€â”€ gateway.c / .h             # Protokol Discord Gateway v10
â”‚   â”œâ”€â”€ websocket.c / .h           # Klien WebSocket via mbedTLS + resolver REST
â”‚   â”œâ”€â”€ detector.c / .h            # Pendeteksi aplikasi foreground
â”‚   â”œâ”€â”€ config.c / .h              # Pemuat config, gamelist, dan token
â”‚   â”œâ”€â”€ log.c / .h                 # Logger berkas, aman untuk multi-thread
â”‚   â”œâ”€â”€ cJSON.c / .h               # Parser dan serializer JSON
â”‚   â”œâ”€â”€ mbedtls_config.h           # Konfigurasi TLS client minimal
â”‚   â”œâ”€â”€ entropy_hardware_alt.c     # Sumber acak /dev/urandom untuk mbedTLS
â”‚   â”œâ”€â”€ Android.mk                 # Flag dan target build NDK
â”‚   â”œâ”€â”€ Application.mk             # Platform dan ABI
â”‚   â””â”€â”€ build.sh                   # Kompilasi daemon via NDK
â”‚
â”œâ”€â”€ webui/                         # Source WebUI (Vue 3 + Vite)
â”‚   â”œâ”€â”€ index.html
â”‚   â”œâ”€â”€ package.json
â”‚   â”œâ”€â”€ vite.config.js             # Output build ke module/webroot/
â”‚   â””â”€â”€ src/
â”‚       â”œâ”€â”€ main.js                # Entry point Vue
â”‚       â”œâ”€â”€ App.vue                # Layout dan navigasi
â”‚       â”œâ”€â”€ router/index.js        # Rute: /, /games, /settings
â”‚       â”œâ”€â”€ views/
â”‚       â”‚   â”œâ”€â”€ HomeView.vue       # Status daemon, game aktif, tombol start/stop
â”‚       â”‚   â”œâ”€â”€ GamesView.vue      # Daftar game, toggle, ubah nama, ikon/appId
â”‚       â”‚   â””â”€â”€ SettingsView.vue   # Token, interval polling, app_id, log
â”‚       â”œâ”€â”€ stores/
â”‚       â”‚   â”œâ”€â”€ config.js          # Pinia: config.json dan token
â”‚       â”‚   â”œâ”€â”€ games.js           # Pinia: gamelist.json dan sinkronisasi otomatis
â”‚       â”‚   â””â”€â”€ status.js          # Pinia: status runtime dan log
â”‚       â””â”€â”€ helpers/
â”‚           â”œâ”€â”€ apps.js            # Database KNOWN_GAMES dan deteksi paket
â”‚           â””â”€â”€ exec.js            # Pembungkus shell root dan file I/O
â”‚
â””â”€â”€ module/                        # Struktur modul Magisk / KernelSU
    â”œâ”€â”€ module.prop                # Metadata modul
    â”œâ”€â”€ customize.sh               # Instalasi dan migrasi saat flash
    â”œâ”€â”€ service.sh                 # Menjalankan daemon saat boot
    â”œâ”€â”€ action.sh                  # Handler tombol Action
    â”œâ”€â”€ uninstall.sh               # Pembersihan saat modul dihapus
    â”œâ”€â”€ gamelist.txt               # 470+ package game populer
    â”œâ”€â”€ libs/                      # Binary hasil kompilasi
    â”‚   â”œâ”€â”€ arm64-v8a/rpcdroid
    â”‚   â”œâ”€â”€ armeabi-v7a/rpcdroid
    â”‚   â””â”€â”€ x86_64/rpcdroid
    â””â”€â”€ webroot/                   # Bundle WebUI statis
```

</details>

<details>
<summary><strong>Berkas konfigurasi di perangkat</strong></summary>

<br>

Seluruh data runtime disimpan di `/data/adb/.config/drpc/`.

| Berkas | Format | Isi |
|---|---|---|
| `token` | Teks biasa | Discord user token |
| `config.json` | JSON | `poll_interval`, `status` (status online), `log_level`, dan `application_id` cadangan |
| `gamelist.json` | JSON | Kunci berupa package name; nilai berisi nama game, `app_id` Discord, dan flag `enabled` |
| `status.json` | JSON | Ditulis daemon: running/stopped, game aktif, status koneksi gateway. Dibaca WebUI |
| `drpc.log` | Teks biasa | Log untuk diagnosa |

</details>

<details id="cara-mendapatkan-discord-token">
<summary><strong>Cara mendapatkan Discord token</strong></summary>

<br>

> Token setara dengan kata sandi akun. Jangan dibagikan, dan jangan menjalankan kode dari sumber yang tidak dipercaya di konsol atau bilah alamat.

**Melalui browser di ponsel**

1. Buka `https://discord.com/login` dan masuk.
2. Setelah berhasil, buka `https://discord.com/app`.
3. Di bilah alamat, ketik kode berikut. Pastikan awalan `javascript:` tidak hilang saat ditempel.

   ```javascript
   javascript:void(document.title=((webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken!==void 0).exports.default.getToken()))
   ```

4. Judul tab akan berubah menjadi token akun. Salin teks tersebut.

**Melalui PC (DevTools)**

1. Buka Discord Desktop, atau `https://discord.com/app` di browser.
2. Tekan `Ctrl + Shift + I` (atau `F12`) dan buka tab **Console**.
3. Tempelkan kode berikut lalu tekan Enter.

   ```javascript
   (webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken!==void 0).exports.default.getToken()
   ```

4. Salin token yang muncul, lalu isikan di WebUI modul.

</details>

<details>
<summary><strong>Kompilasi dari source</strong></summary>

<br>

**Kebutuhan**

- Android NDK r25c atau lebih baru (disarankan r28+)
- Node.js 18+ dan npm
- Python 3 atau utilitas `zip` untuk pengemasan

**Langkah manual**

```bash
# 1. Daemon
export ANDROID_NDK_HOME="/path/ke/ndk/<versi>"
cd daemon
ndk-build -j8
cd ..

# 2. WebUI
cd webui
npm install
npm run build
cd ..

# 3. Kemas menjadi zip modul
python3 - <<'EOF'
import os, zipfile

with zipfile.ZipFile("rpcdroid-v0.1.2.zip", "w", zipfile.ZIP_DEFLATED) as zf:
    for root, _, files in os.walk("module"):
        for name in files:
            path = os.path.join(root, name)
            arcname = os.path.relpath(path, "module").replace(os.sep, "/")
            zf.write(path, arcname)

print("Selesai: rpcdroid-v0.1.2.zip")
EOF
```

Ketiga langkah tersebut juga dirangkum dalam `build.sh` di root repositori.

</details>

---

## Kredit

- [Encore Tweaks](https://github.com/Rem01Gaming/encore) (Apache-2.0): rujukan untuk pendekatan deteksi proses berbasis event (BinderMonitor dengan `onForegroundActivitiesChanged` / `onProcessDied`), hot-reload berbasis inotify, dan arsitektur daemon.
- [navidrome/discord-rich-presence-plugin](https://github.com/navidrome/discord-rich-presence-plugin): rujukan alur pendaftaran gambar eksternal (`mp:external/...`) dan caching.
- [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) (Apache 2.0): TLS untuk koneksi ke Gateway.
- [cJSON](https://github.com/DaveGamble/cJSON) (MIT): parser JSON untuk C.
- [KernelSU](https://github.com/tiann/KernelSU): root solution dan API `kernelsu` untuk WebUI.
- [Vue](https://vuejs.org/), [Vite](https://vitejs.dev/), [Pinia](https://pinia.vuejs.org/), [Vue Router](https://router.vuejs.org/), dan [Tailwind CSS](https://tailwindcss.com/): fondasi WebUI.

## Lisensi dan disclaimer

Dirilis di bawah [GNU General Public License v3.0](LICENSE) (GPL-3.0). Pustaka pihak ketiga yang disertakan (Mbed TLS, cJSON) tetap mengikuti lisensinya masing-masing.

Daemon terhubung ke Discord Gateway memakai user token dengan identitas klien desktop, yang berada di luar API bot resmi Discord. Penggunaan sepenuhnya menjadi tanggung jawab pengguna.
