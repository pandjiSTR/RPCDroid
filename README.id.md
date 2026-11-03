# RPCDroid

**Bahasa Indonesia** | [English](README.md)

Modul root untuk Magisk, KernelSU, dan APatch yang mengirim aktivitas game Android ke Discord Rich Presence.

![Versi](https://img.shields.io/badge/versi-0.1.4-blue)
![Android](https://img.shields.io/badge/Android-10%2B-3DDC84)
![Daemon](https://img.shields.io/badge/daemon-C%2B%2B%20(NDK)-informational)
![Lisensi](https://img.shields.io/badge/lisensi-GPL--3.0-blue)

## Ringkasan

Saat sebuah game dibuka di ponsel, status Discord akun menampilkan nama game, ikonnya, dan lama bermain (misalnya *Playing Mobile Legends*). Status dihapus otomatis ketika game ditutup.

Fungsi tersebut dijalankan oleh sebuah daemon native (`rpcdroid`) yang berjalan sebagai service modul dan terhubung langsung ke Discord Gateway melalui WebSocket over TLS. Pengaturan dilakukan lewat WebUI yang dibuka dari aplikasi manajer root.

## Pendekatan

Modul ini tidak menggunakan service aplikasi Android atau izin aksesibilitas. Deteksi game dilakukan oleh daemon dengan membaca aplikasi foreground melalui `dumpsys`, dan koneksi ke Discord dikelola oleh daemon yang sama.

Pemakaian sumber daya daemon sekitar 1-2 MB RAM dengan beban CPU yang mendekati nol pada kondisi idle.

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
- Versi saat ini (0.1.4) masih awal dan belum diuji luas pada berbagai perangkat.

## Instalasi

### Persyaratan

- Android 10 atau lebih baru (API 29+)
- KernelSU, APatch, atau Magisk
- Akun Discord

### Langkah

1. Unduh `rpcdroid-v0.1.4.zip` dari halaman rilis.
2. Di aplikasi manajer root, buka **Modules**, pilih **Install from storage**, lalu pilih berkas zip tersebut.
3. Reboot perangkat setelah instalasi selesai.
4. Buka WebUI modul (tombol WebUI di KernelSU/APatch, atau tombol Action di Magisk).
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
(settings)   (config, token, status)        (detection + connection) (WSS, port 443)
```

WebUI dan daemon tidak berkomunikasi langsung. Keduanya berbagi berkas di `/data/adb/.config/drpc/`. WebUI menulis konfigurasi, daemon memantaunya dengan `inotify` dan memuat ulang saat ada perubahan, lalu menulis `status.json` yang dibaca kembali oleh WebUI.

Daemon dikelola oleh `service.sh` (dimulai saat boot). Tombol **Start/Stop** di WebUI hanya mengatur berkas flag `enabled`; tidak menjalankan daemon langsung. Ini memastikan daemon bertahan saat aplikasi manajer root ditutup.

### Alur dari instalasi hingga status tampil

1. **Instalasi.** `customize.sh` menyalin binary `rpcdroid` sesuai arsitektur CPU, lalu memindai `/data/data/<package>` untuk menemukan game terpasang. Hasilnya dicocokkan dengan `gamelist.txt` dan disusun menjadi `gamelist.json` beserta `app_id` tiap game.
2. **Konfigurasi.** Token yang diisi di WebUI ditulis ke berkas `token`.
3. **Koneksi.** Daemon membuka koneksi TLS ke `gateway.discord.gg:443` dan melakukan handshake WebSocket.
4. **Autentikasi.** Daemon menerima Opcode 10 (*Hello*), menjalankan thread heartbeat, lalu mengirim Opcode 2 (*Identify*).
5. **Deteksi.** Pada setiap interval, daemon membaca aplikasi foreground dan mencocokkannya dengan `gamelist.json`.
6. **Pengiriman.** Jika cocok dan game diaktifkan, daemon mengirim Opcode 3 (*Presence Update*) berisi nama game, waktu mulai (UNIX timestamp), dan `application_id`.

| Opcode | Arah | Fungsi |
|---|---|---|
| 10 Hello | Discord -> daemon | Menyampaikan interval heartbeat |
| 1 Heartbeat | Daemon -> Discord | Menjaga koneksi tetap aktif |
| 2 Identify | Daemon -> Discord | Autentikasi dengan token |
| 3 Presence Update | Daemon -> Discord | Mengirim aktivitas game |

## Mekanisme ikon

Ikon ditentukan melalui dua jalur, dengan urutan prioritas berikut.

**1. Game terdaftar di Discord.** Daemon memakai `app_id` resmi game tersebut (misalnya Mobile Legends, Genshin Impact, Arknights). Discord menampilkan ikon dari CDN-nya sendiri.

**2. Game belum terdaftar** (contoh: Monster Hunter Outlanders). Daemon memakai Application ID generik (`962990036020756480`) sebagai dasar. URL ikon dari konfigurasi, `gamelist.json`, atau Play Store dikonversi menjadi alamat `mp:external/...` melalui endpoint `external-assets` Discord. Permintaan HTTP POST ini dikirim langsung oleh daemon melalui TLS tanpa `curl`. Hasilnya dikirim ke gateway sebagai `large_image`.

Konversi pada jalur kedua diperlukan karena Discord menolak URL `https://` mentah yang dikirim langsung ke gateway, sehingga ikon tampil rusak.

## Tech stack

| Komponen | Teknologi | Peran |
|---|---|---|
| Daemon | C++ (main/config/gateway/log) + C (websocket, cJSON), Android NDK (Clang), mbedTLS 3.6.0, cJSON, inotify, Android Binder (BinderMonitor) | Koneksi ke Gateway, deteksi game berbasis event, pemrosesan JSON, hot-reload konfigurasi |
| WebUI | Vue 3 (Composition API), Vite 7, Pinia 2, Vue Router 4, Tailwind CSS 4, `kernelsu` API | Pengaturan token, daftar game, dan log |
| Instalasi dan service | POSIX shell, `pm`, `dumpsys` | `customize.sh`, `service.sh`, `uninstall.sh` |
| Protokol | Discord Gateway API v10, WebSocket (RFC 6455) over TLS | Autentikasi dan pengiriman Rich Presence |

<details>
<summary><strong>Struktur proyek</strong></summary>

```
RPCDroid/
|-- build.sh                       # Builder utama seluruh modul
|-- README.md
|-- README.id.md
|-- rpcdroid-v0.1.4.zip            # Paket modul siap flash
|-- .gitignore
|
|-- daemon/                        # Source daemon (C++/C, NDK)
|   |-- main.cpp                    # Entry point (C++, BinderMonitor)
|   |-- gateway.cpp / .h           # Protokol Discord Gateway v10
|   |-- websocket.c / .h           # Klien WebSocket via mbedTLS + REST resolver
|   |-- config.cpp / .h            # Config, gamelist, dan token loader
|   |-- log.cpp / .h               # Logger berkas, aman untuk multi-thread
|   |-- cJSON.c / .h               # Parser dan serializer JSON
|   |-- mbedtls_config.h           # Konfigurasi TLS client minimal
|   |-- entropy_hardware_alt.c     # Sumber acak /dev/urandom untuk mbedTLS
|   |-- include/                   # Header (Encore.hpp, EncoreLog.hpp, ...)
|   |-- base/                      # BinderMonitor, BinderNDK, InotifyWatcher (dari Encore)
|   |-- external/                  # rapidjson, spdlog
|   |-- binder_resolver.apk        # Resolver kode transaksi (dari Encore)
|   |-- Android.mk                 # Flag dan target build NDK
|   |-- Application.mk             # Platform dan ABI
|   `-- build.sh                   # Kompilasi daemon via NDK
|
|-- webui/                         # Source WebUI (Vue 3 + Vite)
|   |-- index.html
|   |-- package.json
|   |-- vite.config.js             # Output build ke module/webroot/
|   `-- src/
|       |-- main.js                # Entry point Vue
|       |-- App.vue                # Layout dan navigasi
|       |-- router/index.js        # Rute: /, /games, /settings
|       |-- views/
|       |   |-- HomeView.vue       # Status daemon, game aktif, tombol start/stop
|       |   |-- GamesView.vue      # Daftar game, toggle, ubah nama, ikon/appId
|       |   `-- SettingsView.vue   # Token, interval polling, app_id, log
|       |-- stores/
|       |   |-- config.js          # Pinia: config.json dan token
|       |   |-- games.js           # Pinia: gamelist.json dan sinkronisasi otomatis
|       |   `-- status.js          # Pinia: status runtime dan log
|       `-- helpers/
|           |-- apps.js            # Database KNOWN_GAMES dan deteksi paket
|           `-- exec.js            # Pembungkus shell root dan file I/O
|
`-- module/                        # Struktur modul Magisk / KernelSU
    |-- module.prop                # Metadata modul
    |-- customize.sh               # Instalasi dan migrasi saat flash
    |-- service.sh                 # Menjalankan daemon saat boot (watchdog)
    |-- uninstall.sh               # Pembersihan saat modul dihapus
    |-- gamelist.txt               # 470+ package game populer
    |-- binder_resolver.apk        # Resolver kode transaksi
    |-- libs/                      # Binary hasil kompilasi
    |   |-- arm64-v8a/rpcdroid
    |   |-- armeabi-v7a/rpcdroid
    |   `-- x86_64/rpcdroid
    `-- webroot/                   # Bundle WebUI statis
```

</details>

<details>
<summary><strong>Berkas konfigurasi di perangkat</strong></summary>

<br>

Seluruh data runtime disimpan di `/data/adb/.config/drpc/`.

| Berkas | Format | Isi |
|---|---|---|
| `token` | Teks biasa | Discord user token |
| `config.json` | JSON | `poll_interval`, `log_level`, dan `application_id` cadangan |
| `gamelist.json` | JSON | Kunci berupa package name; nilai berisi nama game, `app_id` Discord, dan flag `enabled` |
| `status.json` | JSON | Ditulis daemon: running/stopped, game aktif, status koneksi gateway, pid, avatar. Dibaca WebUI |
| `drpc.log` | Teks biasa | Log untuk diagnosa (dipotong otomatis jika >512 KB) |
| `runtime.json` | JSON | Waktu mulai bermain yang tersimpan (untuk timer yang persisten) |
| `icon_cache.json` | JSON | Cache URL ikon `mp:external/...` |

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

with zipfile.ZipFile("rpcdroid-v0.1.4.zip", "w", zipfile.ZIP_DEFLATED) as zf:
    for root, _, files in os.walk("module"):
        for name in files:
            path = os.path.join(root, name)
            arcname = os.path.relpath(path, "module").replace(os.sep, "/")
            zf.write(path, arcname)

print("Selesai: rpcdroid-v0.1.4.zip")
EOF
```

Ketiga langkah tersebut juga dirangkum dalam `build.sh` di root repositori.

</details>

---

## Kredit

- [Kizzy](https://github.com/dead8309/Kizzy) dan [KizzyRPC](https://github.com/dead8309/KizzyRPC): rujukan pendekatan deteksi aplikasi foreground, struktur Activity/Assets, serta penggunaan `application_id` dan endpoint `external-assets`.
- [navidrome/discord-rich-presence-plugin](https://github.com/navidrome/discord-rich-presence-plugin): rujukan alur pendaftaran gambar eksternal (`mp:external/...`) dan caching.
- [Encore Tweaks](https://github.com/Rem01Gaming/encore) (Apache-2.0): rujukan untuk pendekatan deteksi proses berbasis event (BinderMonitor dengan `onForegroundActivitiesChanged` / `onProcessDied`), hot-reload berbasis inotify, dan arsitektur daemon.
- [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) (Apache 2.0): TLS untuk koneksi ke Gateway.
- [cJSON](https://github.com/DaveGamble/cJSON) (MIT): parser JSON untuk C.
- [KernelSU](https://github.com/tiann/KernelSU): root solution dan API `kernelsu` untuk WebUI.
- [Vue](https://vuejs.org/), [Vite](https://vitejs.dev/), [Pinia](https://pinia.vuejs.org/), [Vue Router](https://router.vuejs.org/), dan [Tailwind CSS](https://tailwindcss.com/): fondasi WebUI.

## Lisensi dan penafian

Proyek ini bersifat open-source dan didistribusikan untuk tujuan edukasi serta optimasi sistem Android. Daemon terhubung ke Discord Gateway memakai user token dengan identitas klien desktop, yang berada di luar API bot resmi Discord. Penggunaan sepenuhnya menjadi tanggung jawab pengguna.

<!-- Tambahkan nama lisensi dan berkas LICENSE di sini, misalnya: "Dirilis di bawah lisensi MIT. Lihat LICENSE." -->