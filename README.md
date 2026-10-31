# RPCDroid — Magisk / KernelSU / APatch Module (v0.0.1)

Modul root tingkat sistem (Magisk, KernelSU, APatch) yang secara otomatis menampilkan aktivitas bermain game Android ke **Discord Rich Presence** secara *native* dan *real-time*.

Berbeda dengan aplikasi Android biasa yang memerlukan izin aksesibilitas berat, boros baterai, dan rentan dimatikan oleh sistem (low memory killer), modul ini bekerja melalui **daemon background C native** (~1–2 MB RAM, CPU ~0%) yang langsung terhubung ke Discord Gateway WebSocket via TLS terenkripsi, dikendalikan melalui antarmuka **WebUI modern (Vue 3)**.

---

## Daftar Isi

1. [Tech Stack & Dependensi](#tech-stack--dependensi)
2. [Arsitektur & Cara Kerja](#arsitektur--cara-kerja)
3. [Struktur File](#struktur-file)
4. [Alur Kerja Sistem](#alur-kerja-sistem)
5. [Mekanisme Icon Discord & Fallback](#mekanisme-icon-discord--fallback)
6. [Struktur File Konfigurasi di Perangkat](#struktur-file-konfigurasi-di-perangkat)
7. [Instalasi & Penggunaan](#instalasi--penggunaan)
8. [Panduan Mendapatkan Discord Token](#panduan-mendapatkan-discord-token)
9. [Kompilasi dari Source](#kompilasi-dari-source)
10. [Kredit & Referensi](#kredit--referensi)
11. [Lisensi & Penafian](#lisensi--penafian)

---

## Tech Stack & Dependensi

| Komponen | Teknologi / Bahasa | Pustaka / Dependensi | Fungsi Utama |
|---|---|---|---|
| **Daemon (Core Engine)** | C (C99), Android NDK (Clang) | • **mbedTLS (v3.6.0)**<br>• **cJSON**<br>• Linux POSIX Sockets<br>• Linux `sys/inotify.h` | Daemon latar belakang mandiri (`rpcdroid`). Mengelola koneksi aman WebSocket ke Discord, memantau foreground window Android, memproses JSON payload, dan hot-reload konfigurasi tanpa restart. |
| **WebUI (Frontend)** | JavaScript (ESNext), HTML5, CSS3 | • **Vue 3** (Composition API, `<script setup>`)<br>• **Vite 7** (Bundler & HMR)<br>• **Pinia 2** (State Management)<br>• **Vue Router 4** (Client-side Routing)<br>• **Tailwind CSS 4** (Styling)<br>• **kernelsu** (Root bridge & Package Manager API) | Antarmuka manajemen visual di dalam KernelSU / Magisk Manager untuk pengaturan token, switch game, rename game, dan melihat log status daemon. |
| **Module Packaging & Installer** | POSIX Shell Scripting (sh/bash) | • Android Busybox / Toybox shell<br>• Android `pm` (Package Manager)<br>• Android `dumpsys` | Skrip instalasi (`customize.sh`), automasi boot (`service.sh`), aksi WebUI (`action.sh`), dan uninstaller (`uninstall.sh`). |
| **Network Protocol** | Discord Gateway API v10 | • WebSocket RFC 6455 over TLS (Port 443)<br>• JSON-based Opcodes | Otentikasi dan push status aktivitas Rich Presence ke server Discord. |

---

## Arsitektur & Cara Kerja

```
┌────────────────────────────────────────────────────────────────────────┐
│                          PERANGKAT ANDROID (ROOT)                      │
│                                                                        │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │                     WebUI (Vue 3 SPA)                          │   │
│   │   • HomeView.vue       • GamesView.vue     • SettingsView.vue  │   │
│   │   • apps.js            • games.js store    • status.js store   │   │
│   └────────────────────────────────┬───────────────────────────────┘   │
│                                    │ Root File I/O via ksuExec         │
│                                    ▼                                   │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │              Directory: /data/adb/.config/drpc/                │   │
│   │   • token              • config.json       • gamelist.json     │   │
│   │   • status.json        • drpc.log                              │   │
│   └────────────────────────────────┬───────────────────────────────┘   │
│                                    │ Linux inotify (File Watcher)      │
│                                    ▼                                   │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │               rpcdroid (C Background Daemon)               │   │
│   │                                                                │   │
│   │   ┌─────────────────────┐            ┌─────────────────────┐   │   │
│   │   │    detector.c       │            │      gateway.c      │   │   │
│   │   │ (dumpsys poll ~5s)  ├───────────►│ (Op 3 Presence)     │   │   │
│   │   └─────────────────────┘            └──────────┬──────────┘   │   │
│   │              ▲                                  │              │   │
│   │              │                                  ▼              │   │
│   │      Android OS Framework              websocket.c (mbedTLS)   │   │
│   │    (Activity / WindowManager)                   │              │   │
│   └─────────────────────────────────────────────────┼──────────────┘   │
└─────────────────────────────────────────────────────┼──────────────────┘
                                                      │
                                                      │ WSS (Encrypted TLS 443)
                                                      ▼
                                       ┌─────────────────────────────┐
                                       │    Discord Gateway Server   │
                                       │ (gateway.discord.gg:443)    │
                                       └──────────────┬──────────────┘
                                                      │
                                                      ▼
                                       ┌─────────────────────────────┐
                                       │  Discord Desktop / Mobile   │
                                       │    Menampilkan Aktivitas    │
                                       │  Playing [Nama Game] + Icon │
                                       └─────────────────────────────┘
```

---

## Struktur File

```
RPCDroid/
├── build.sh                       # Skrip utama builder seluruh modul
├── README.md                      # Dokumentasi proyek
├── rpcdroid-v0.0.1.zip            # Paket ZIP modul siap flash
├── .gitignore
│
├── daemon/                        # SOURCE CODE DAEMON (C NDK)
│   ├── main.c                     # Entry point daemon & inotify hot-reload
│   ├── gateway.c / gateway.h      # Protokol Discord Gateway v10
│   ├── websocket.c / websocket.h  # RFC 6455 WebSocket client via mbedTLS + REST resolver
│   ├── detector.c / detector.h    # Pendeteksi aplikasi foreground Android
│   ├── config.c / config.h        # Loader JSON config, gamelist, & token
│   ├── log.c / log.h              # File logger thread-safe
│   ├── cJSON.c / cJSON.h          # Parser & serializer JSON C ultralight
│   ├── mbedtls_config.h           # Konfigurasi minimal TLS client
│   ├── entropy_hardware_alt.c     # Random generator /dev/urandom untuk mbedTLS
│   ├── Android.mk                 # Skrip build NDK compiler flags & target
│   ├── Application.mk             # Konfigurasi platform & arsitektur ABI
│   └── build.sh                   # Skrip kompilasi daemon via NDK
│
├── webui/                         # SOURCE CODE WEBUI (Vue 3 + Vite)
│   ├── index.html                 # Shell HTML WebUI
│   ├── package.json               # Dependensi & script WebUI
│   ├── vite.config.js             # Konfigurasi output Vite ke module/webroot/
│   └── src/
│       ├── main.js                # Entry point aplikasi Vue 3
│       ├── App.vue                # Root layout & navbar view
│       ├── router/index.js        # Vue Router (routes: /, /games, /settings)
│       ├── views/
│       │   ├── HomeView.vue       # Status daemon, active game, kontrol start/stop
│       │   ├── GamesView.vue      # List game, toggle, edit nama, link icon/appId
│       │   └── SettingsView.vue   # Token input, polling rate, app_id, log viewer
│       ├── stores/
│       │   ├── config.js          # Store Pinia manajemen config.json & token
│       │   ├── games.js           # Store Pinia manajemen gamelist.json & auto-sync
│       │   └── status.js          # Store Pinia pembaca status runtime & log
│       └── helpers/
│           ├── apps.js            # Database KNOWN_GAMES & deteksi paket Android
│           └── exec.js            # Wrapper perintah shell root & file I/O
│
└── module/                        # STRUKTUR MODUL MAGISK / KSU SIAP FLASH
    ├── module.prop                # Metadata modul (v0.0.1, id, author)
    ├── customize.sh               # Skrip instalasi & migrasi saat flash
    ├── service.sh                 # Background service runner saat boot
    ├── action.sh                  # Handler tombol "Action" di KernelSU Manager
    ├── uninstall.sh               # Skrip pembersih saat modul dihapus
    ├── gamelist.txt               # Daftar 470+ package game Android populer
    ├── libs/                      # Binary executable hasil kompilasi daemon
    │   ├── arm64-v8a/rpcdroid
    │   ├── armeabi-v7a/rpcdroid
    │   └── x86_64/rpcdroid
    └── webroot/                   # Bundle WebUI statis (index.html, JS, CSS)
```

---

## Alur Kerja Sistem

1. **Inisialisasi Pertama (First Install)**:
   * Pengguna menginstal file zip modul melalui KernelSU / Magisk.
   * `customize.sh` berjalan, menyalin binary `rpcdroid` yang cocok dengan CPU ponsel ke `$MODPATH/bin/`.
   * `customize.sh` memindai direktori `/data/data/<package>` untuk mendeteksi game yang sudah terinstall, mencocokkannya dengan `gamelist.txt`, lalu menyusun berkas `/data/adb/.config/drpc/gamelist.json` lengkap dengan `app_id` masing-masing game.
2. **Konfigurasi Akun Pengguna**:
   * Pengguna membuka WebUI modul, pada tab **Settings** menempelkan Discord User Token lalu menyimpannya.
   * WebUI menulis token ke `/data/adb/.config/drpc/token`.
3. **Daemon Berjalan & Autentikasi WebSocket**:
   * Daemon `rpcdroid` membaca token dan menyambungkan koneksi TLS ke `gateway.discord.gg:443`.
   * Setelah koneksi WebSocket terbuka, Discord mengirimkan *Opcode 10 Hello*. Daemon menyalakan thread *Heartbeat*.
   * Daemon mengirimkan *Opcode 2 Identify*. Discord memvalidasi token dan menerima sesi.
4. **Siklus Deteksi & Pengiriman Rich Presence**:
   * Setiap interval (default: 5 detik), modul mengecek jendela foreground Android.
   * Daemon mendeteksi package aktif, mencocokkannya ke `gamelist.json`.
   * Daemon membuat payload JSON *Opcode 3 Presence Update* yang memuat nama game, UNIX timestamp waktu mulai main, dan `application_id`.
   * Server Discord menampilkan Rich Presence dengan nama game dan icon.

---

## Mekanisme Icon Discord & Fallback

Hierarki resolusi icon (prioritas):

1. **Prioritas 1 — Game terdaftar di database Discord**:
   * Menggunakan `app_id` resmi (contoh: Mobile Legends, Genshin Impact, Arknights, dll.).
   * Resolusi media proxy dilewati total. Discord otomatis menampilkan icon resmi dari CDN Discord.
2. **Prioritas 2 — Fallback untuk game belum terdaftar (unlisted)**:
   * Game tidak punya `app_id` di database Discord (seperti Monster Hunter Outlanders).
   * Menggunakan generic Application ID (`962990036020756480`) sebagai base RPC.
   * Icon (URL `https://` dari config/gamelist atau Play Store) dikonversi native via endpoint Discord `external-assets` menjadi `mp:external/...` (HTTP POST over TLS langsung di daemon C, tanpa `curl`).
   * Mengirim `mp:external/...` ke gateway sebagai `large_image`, sehingga icon game tetap muncul.

Catatan: tautan gambar mentah (`https://play-lh.googleusercontent.com/...`) **tidak** bisa dikirim langsung ke gateway — Discord menolak dan icon menjadi pecah. Karena itu konversi ke `mp:external/...` wajib.

---

## Struktur File Konfigurasi di Perangkat

Semua data runtime modul disimpan terpusat di direktori `/data/adb/.config/drpc/`:

| Nama File | Format | Deskripsi |
|---|---|---|
| `token` | Plain text | Discord user authorization token. |
| `config.json` | JSON | Pengaturan daemon: polling interval (`poll_interval`), status online (`status`), log level (`log_level`), dan fallback application ID (`application_id`). |
| `gamelist.json` | JSON | Objek daftar game aktif: key berupa package name Android, value berisi nama game, Discord `app_id`, dan flag status `enabled`. |
| `status.json` | JSON | Berkas status runtime yang ditulis oleh daemon (running/stopped, active game, gateway connection status). Dibaca oleh WebUI. |
| `drpc.log` | Plain text | Berkas log sistem untuk diagnosa dan debugging error. |

---

## Instalasi & Penggunaan

### Persyaratan

* Perangkat Android 10 atau lebih baru (API Level 29+).
* Root solution: **KernelSU**, **APatch**, atau **Magisk**.
* Arsitektur CPU: `arm64-v8a`, `armeabi-v7a`, atau `x86_64`.
* Akun Discord aktif.

### Langkah Instalasi

1. Unduh rilis file zip modul terbaru: `rpcdroid-v0.0.1.zip`.
2. Buka aplikasi **KernelSU / APatch / Magisk Manager**.
3. Masuk ke tab **Modules** → **Install from storage** → pilih file zip tersebut.
4. Tunggu proses instalasi selesai, lalu **Reboot ponsel**.
5. Setelah reboot, buka **WebUI Modul** (melalui tombol WebUI di KernelSU/APatch atau tombol Action di Magisk).
6. Masuk ke menu **Settings**, tempelkan **Discord User Token** Anda, lalu tekan **Save**.
7. Masuk ke halaman **Home**, lalu tekan tombol **Start** jika status belum Running.
8. Jalankan game Anda di ponsel — status Discord Anda akan langsung berubah secara otomatis!

---

## Panduan Mendapatkan Discord Token

> ⚠️ **PERINGATAN KEAMANAN**: Token akun Discord setara dengan password Anda. Jangan pernah membagikan token Anda kepada siapapun. Modul ini hanya menyimpan token secara lokal di direktori `/data/adb/.config/drpc/token` yang hanya bisa diakses oleh akses ROOT.

### Cara 1: Menggunakan Browser di HP Android (Praktis)

1. Buka browser (Chrome/Kiwi/Brave) di ponsel Android Anda.
2. Buka `https://discord.com/login` dan masuk ke akun Anda.
3. Setelah login berhasil, arahkan URL ke `https://discord.com/app`.
4. Di bilah alamat browser, ketik kode berikut (pastikan awalan `javascript:` tidak terhapus saat di-paste):
   ```javascript
   javascript:void(document.title=((webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken!==void 0).exports.default.getToken()))
   ```
5. Tekan Enter / Kunjungi. Judul tab browser Anda akan otomatis berubah menjadi sederet teks token Discord Anda. Salin teks tersebut.

### Cara 2: Menggunakan PC (DevTools Console)

1. Buka aplikasi Discord Desktop atau buka `https://discord.com/app` di browser PC.
2. Tekan tombol `Ctrl + Shift + I` (atau `F12`) untuk membuka Developer Tools.
3. Klik tab **Console**.
4. Tempelkan kode berikut lalu tekan Enter:
   ```javascript
   (webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken!==void 0).exports.default.getToken()
   ```
5. Salin string token yang muncul di konsol dan masukkan ke WebUI modul di ponsel Anda.

---

## Kompilasi dari Source

### Kebutuhan Sistem

* **Android NDK** (r25c atau versi lebih baru, disarankan r28+).
* **Node.js** (v18+) dan **npm**.
* **Python 3** (untuk pengemasan zip) atau utilitas `zip`.

### Perintah Build Otomatis

```bash
# 1. Kompilasi Daemon C (menggunakan NDK ndk-build)
export ANDROID_NDK_HOME="C:/Users/<user>/AppData/Local/Android/Sdk/ndk/<versi>"
cd daemon
ndk-build -j8
cd ..

# 2. Kompilasi WebUI Frontend
cd webui
npm install
npm run build
cd ..

# 3. Paketkan ke ZIP Modul Magisk/KernelSU
python -c "
import zipfile, os
zip_path = 'rpcdroid-v0.0.1.zip'
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk('module'):
        for f in files:
            full_path = os.path.join(root, f)
            rel_path = os.path.relpath(full_path, 'module').replace('\\\\', '/')
            zf.write(full_path, rel_path)
print('Module packaged successfully:', zip_path)
"
```

---

## Kredit & Referensi

Proyek ini dibangun dengan memanfaatkan dan merujuk pada karya/proyek berikut. Terima kasih kepada para pembuatnya:

* **[Kizzy (dead8309/Kizzy)](https://github.com/dead8309/Kizzy)** — Aplikasi Android Discord Rich Presence; referensi untuk pendekatan deteksi aplikasi foreground, external proxy image, dan integrasi Discord Gateway pada Android.
* **[KizzyRPC (dead8309/KizzyRPC)](https://github.com/dead8309/KizzyRPC)** — Library Android Discord RPC; referensi struktur Activity/Assets dan penggunaan `application_id` + `external-assets` endpoint.
* **[navidrome/discord-rich-presence-plugin](https://github.com/navidrome/discord-rich-presence-plugin)** — Referensi untuk alur registrasi gambar eksternal via Discord external assets API (`mp:external/...`) dan caching.
* **[Mbed-TLS / mbedtls](https://github.com/Mbed-TLS/mbedtls)** — Library TLS yang digunakan daemon untuk koneksi terenkripsi ke Discord Gateway (lisensi Apache 2.0).
* **[cJSON (DaveGamble/cJSON)](https://github.com/DaveGamble/cJSON)** — Parser/serializer JSON ringan untuk C (lisensi MIT).
* **[KernelSU](https://github.com/tiann/KernelSU)** — Root solution & WebUI bridge (`kernelsu` JS API) yang digunakan modul.
* **[Vue.js](https://vuejs.org/)**, **[Vite](https://vitejs.dev/)**, **[Pinia](https://pinia.vuejs.org/)**, **[Vue Router](https://router.vuejs.org/)**, **[Tailwind CSS](https://tailwindcss.com/)** — Stack frontend WebUI.

---

## Lisensi & Penafian (Disclaimer)

Modul ini menghubungkan WebSocket client langsung ke Discord Gateway menggunakan user token dengan identifikasi desktop client. Hal ini secara teknis berada di luar API resmi bot Discord (*Self-bot presence*). Gunakan dengan bijak dan tanggung jawab Anda sendiri. Modul ini sepenuhnya *open-source* dan didistribusikan untuk tujuan edukasi dan optimasi performa sistem Android.