# RPCDroid

[Bahasa Indonesia](README.id.md) | **English**

A root module for Magisk, KernelSU, and APatch that sends Android game activity to Discord Rich Presence.

![Version](https://img.shields.io/badge/version-0.1.5-blue)
![Android](https://img.shields.io/badge/Android-10%2B-3DDC84)
![Daemon](https://img.shields.io/badge/daemon-C%2B%2B%20(NDK)-informational)
![License](https://img.shields.io/badge/license-GPL--3.0-blue)

## Overview

When a game is opened on the phone, the account's Discord status shows the game name, its icon, and the play time. The status is cleared automatically when the game is closed.

This is handled by a native daemon (`rpcdroid`) that runs as a module service and connects directly to the Discord Gateway over WebSocket on TLS. Settings are managed through a WebUI opened from the root manager app.

## Approach

The module does not use an Android app service or the accessibility permission. The daemon detects games by reading the foreground app through `dumpsys`, and the same daemon manages the connection to Discord.

The daemon uses roughly 1-2 MB of RAM, with CPU load close to zero while idle.

## Features

- Periodic detection of the running game (default: every 3 seconds).
- Built-in database of 470+ Android game packages. Games installed on the device are mapped automatically during installation.
- Icon support for games that are not registered with Discord (see [Icon mechanism](#icon-mechanism)).
- WebUI for setting the token, enabling or disabling games, changing display names, and viewing logs.
- Automatic configuration reload without restarting the daemon.
- Binaries for `arm64-v8a`, `armeabi-v7a`, and `x86_64`.

> [!WARNING]
> This module authenticates to Discord with the account's **user token**, not a bot token.
>
> - A token is equivalent to a password and must not be shared. It is stored at `/data/adb/.config/drpc/token`, which is readable only with root access.
> - Discord's terms of service do not allow automating user accounts (*self-bots*). Any risk to the account is borne by the user. Consider using a secondary account.

## Limitations

- Requires root access. The module does not work on devices without root.
- Only games listed in `gamelist.json` are reported to Discord.
- Detection is event-driven via Binder (IProcessObserver). A lightweight fallback polls the game process state every few seconds for close detection. Status changes may be delayed by up to one poll interval in the worst case.
- Depends on the Discord Gateway, which is not documented for user accounts. Changes on Discord's side may break the module at any time.
- The current version (0.1.5) is an early release and has not been widely tested across devices.

## Installation

### Requirements

- Android 10 or later (API 29+)
- KernelSU, APatch, or Magisk
- A Discord account

### Steps

1. Download `rpcdroid-v0.1.5.zip` from the releases page.
2. In the root manager app, open **Modules**, choose **Install from storage**, and select the zip file.
3. Reboot the device after installation finishes.
4. Open the module's WebUI. On KernelSU and APatch, the WebUI opens from the module list. Magisk users need an additional app, for example [KsuWebUI Standalone](https://github.com/KOWX712/KsuWebUIStandalone/releases).
5. In **Settings**, enter your Discord token and press **Save**. See [this section](#how-to-get-a-discord-token) for how to obtain it.
6. On the **Home** page, press **Start** if the status is not *Running*.
7. Launch a game. The Discord status updates automatically.

### Troubleshooting

| Symptom | What to check |
|---|---|
| Status does not appear | Make sure the daemon shows *Running* on the Home page and the token is saved. |
| Game is not detected | Open the **Games** page, make sure the game is listed and its toggle is on. |
| Icon does not show | See [Icon mechanism](#icon-mechanism). Games that are not registered need a valid icon URL. |
| Cause is unclear | Check the log in **Settings** or the file `/data/adb/.config/drpc/drpc.log`. |

### Reporting issues

When opening an issue, include the following so it is easier to investigate:

- Module version, Android version, device brand/model, and CPU architecture
- The root solution in use and its version (Magisk, KernelSU, or APatch)
- Contents of `drpc.log` (remove the token if it appears) and `status.json`
- Package name of the affected game
- Steps to reproduce the problem

Never include your Discord token in a report.

---

# Technical documentation

## How it works

```
WebUI  <-->  /data/adb/.config/drpc/  <-->  rpcdroid (daemon)  <-->  Discord Gateway
(settings)   (config, token, status)        (detection + connection) (WSS, port 443)
```

The WebUI and the daemon do not communicate directly. They share files in `/data/adb/.config/drpc/`. The WebUI writes the configuration, the daemon watches it with `inotify` and reloads on change, then writes `status.json`, which the WebUI reads back.

The daemon is managed by `service.sh` (started at boot). The **Start/Stop** buttons in the WebUI only toggle the `enabled` flag file; they do not spawn the daemon directly. This ensures the daemon survives when the root manager app is closed.

### Flow from installation to status display

1. **Installation.** `customize.sh` copies the `rpcdroid` binary matching the CPU architecture, then scans `/data/data/<package>` to find installed games. The results are matched against `gamelist.txt` and compiled into `gamelist.json` along with each game's `app_id`.
2. **Configuration.** The token entered in the WebUI is written to the `token` file.
3. **Connection.** The daemon opens a TLS connection to `gateway.discord.gg:443` and performs the WebSocket handshake.
4. **Authentication.** The daemon receives Opcode 10 (*Hello*), starts the heartbeat thread, then sends Opcode 2 (*Identify*).
5. **Detection.** At each interval, the daemon reads the foreground app and matches it against `gamelist.json`.
6. **Dispatch.** If there is a match and the game is enabled, the daemon sends Opcode 3 (*Presence Update*) containing the game name, start time (UNIX timestamp), and `application_id`.

| Opcode | Direction | Purpose |
|---|---|---|
| 10 Hello | Discord -> daemon | Provides the heartbeat interval |
| 1 Heartbeat | Daemon -> Discord | Keeps the connection alive |
| 2 Identify | Daemon -> Discord | Authenticates with the token |
| 3 Presence Update | Daemon -> Discord | Sends the game activity |

## Icon mechanism

Icons are resolved through two paths, in the following order of priority.

**1. The game is registered with Discord.** The daemon uses the game's official `app_id` (for example Mobile Legends, Genshin Impact, Arknights). Discord serves the icon from its own CDN.

**2. The game is not registered** (example: Monster Hunter Outlanders). The daemon uses a generic Application ID (`962990036020756480`) as the base. The icon URL from the configuration, `gamelist.json`, or the Play Store is converted into an `mp:external/...` address through Discord's `external-assets` endpoint. This HTTP POST request is sent directly by the daemon over TLS, without `curl`. The result is sent to the gateway as `large_image`.

The conversion in the second path is required because Discord rejects raw `https://` URLs sent directly to the gateway, which results in a broken icon.

## Tech stack

| Component | Technology | Role |
|---|---|---|
| Daemon | C++ (main/config/gateway/log) + C (websocket, cJSON), Android NDK (Clang), mbedTLS 3.6.0, cJSON, inotify, Android Binder (BinderMonitor) | Gateway connection, event-driven game detection, JSON processing, config hot-reload |
| WebUI | Vue 3 (Composition API), Vite 7, Pinia 2, Vue Router 4, Tailwind CSS 4, `kernelsu` API | Token settings, game list, and logs |
| Installation and service | POSIX shell, `pm`, `dumpsys` | `customize.sh`, `service.sh`, `uninstall.sh` |
| Protocol | Discord Gateway API v10, WebSocket (RFC 6455) over TLS | Authentication and Rich Presence delivery |

<details>
<summary><strong>Project structure</strong></summary>

```
RPCDroid/
|-- build.sh                       # Main builder for the whole module
|-- README.md
|-- README.id.md
|-- rpcdroid-v0.1.5.zip            # Flashable module package
|-- .gitignore
|
|-- daemon/                        # Daemon source (C++/C, NDK)
|   |-- main.cpp                    # Entry point (C++, BinderMonitor)
|   |-- gateway.cpp / .h           # Discord Gateway v10 protocol
|   |-- websocket.c / .h           # WebSocket client via mbedTLS + REST resolver
|   |-- config.cpp / .h            # Config, gamelist, and token loader
|   |-- log.cpp / .h               # File logger, thread-safe
|   |-- cJSON.c / .h               # JSON parser and serializer
|   |-- mbedtls_config.h           # Minimal TLS client configuration
|   |-- entropy_hardware_alt.c     # /dev/urandom entropy source for mbedTLS
|   |-- include/                   # Headers (Encore.hpp, EncoreLog.hpp, ...)
|   |-- base/                      # BinderMonitor, BinderNDK, InotifyWatcher (from Encore)
|   |-- external/                  # rapidjson, spdlog
|   |-- binder_resolver.apk        # Transaction code resolver (from Encore)
|   |-- Android.mk                 # NDK build flags and targets
|   |-- Application.mk             # Platform and ABI
|   `-- build.sh                   # Daemon compilation via NDK
|
|-- webui/                         # WebUI source (Vue 3 + Vite)
|   |-- index.html
|   |-- package.json
|   |-- vite.config.js             # Build output goes to module/webroot/
|   `-- src/
|       |-- main.js                # Vue entry point
|       |-- App.vue                # Layout and navigation
|       |-- router/index.js        # Routes: /, /games, /settings
|       |-- views/
|       |   |-- HomeView.vue       # Daemon status, active game, start/stop buttons
|       |   |-- GamesView.vue      # Game list, toggle, rename, icon/appId
|       |   `-- SettingsView.vue   # Token, polling interval, app_id, log
|       |-- stores/
|       |   |-- config.js          # Pinia: config.json and token
|       |   |-- games.js           # Pinia: gamelist.json and automatic sync
|       |   `-- status.js          # Pinia: runtime status and log
|       `-- helpers/
|           |-- apps.js            # KNOWN_GAMES database and package detection
|           `-- exec.js            # Root shell wrapper and file I/O
|
`-- module/                        # Magisk / KernelSU module structure
    |-- module.prop                # Module metadata
    |-- customize.sh               # Installation and migration at flash time
    |-- service.sh                 # Starts the daemon at boot (watchdog)
    |-- uninstall.sh               # Cleanup when the module is removed
    |-- gamelist.txt               # 470+ popular game packages
    |-- libs/                      # Compiled binaries
    |   |-- arm64-v8a/rpcdroid
    |   |-- armeabi-v7a/rpcdroid
    |   `-- x86_64/rpcdroid
    `-- webroot/                   # Static WebUI bundle
```

</details>

<details>
<summary><strong>Configuration files on the device</strong></summary>

<br>

All runtime data is stored in `/data/adb/.config/drpc/`.

| File | Format | Contents |
|---|---|---|
| `token` | Plain text | Discord user token |
| `config.json` | JSON | `poll_interval`, `log_level`, and a fallback `application_id` |
| `gamelist.json` | JSON | Keys are package names; values hold the game name, Discord `app_id`, and an `enabled` flag |
| `status.json` | JSON | Written by the daemon: running/stopped, active game, gateway connection state, pid, avatar. Read by the WebUI |
| `drpc.log` | Plain text | Diagnostic log (rotated at 512 KB) |
| `runtime.json` | JSON | Persisted playtime (game + start timestamp) |
| `icon_cache.json` | JSON | Cached `mp:external/...` icon URLs |

</details>

<details id="how-to-get-a-discord-token">
<summary><strong>How to get a Discord token</strong></summary>

<br>

> A token is equivalent to the account password. Do not share it, and do not run code from untrusted sources in the console or address bar.

**Using a browser on the phone**

1. Open `https://discord.com/login` and sign in.
2. Once signed in, open `https://discord.com/app`.
3. In the address bar, type the code below. Make sure the `javascript:` prefix is not lost when pasting.

   ```javascript
   javascript:void(document.title=((webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken!==void 0).exports.default.getToken()))
   ```

4. The tab title changes to the account token. Copy that text.

**Using a PC (DevTools)**

1. Open Discord Desktop, or `https://discord.com/app` in a browser.
2. Press `Ctrl + Shift + I` (or `F12`) and open the **Console** tab.
3. Paste the code below and press Enter.

   ```javascript
   (webpackChunkdiscord_app.push([[''],{},e=>{m=[];for(let c in e.c)m.push(e.c[c])}]),m).find(m=>m?.exports?.default?.getToken!==void 0).exports.default.getToken()
   ```

4. Copy the token that appears, then enter it in the module's WebUI.

</details>

<details>
<summary><strong>Building from source</strong></summary>

<br>

**Requirements**

- Android NDK r25c or later (r28+ recommended)
- Node.js 18+ and npm
- Python 3 or the `zip` utility for packaging

**Manual steps**

```bash
# 1. Daemon
export ANDROID_NDK_HOME="/path/to/ndk/<version>"
cd daemon
ndk-build -j8
cd ..

# 2. WebUI
cd webui
npm install
npm run build
cd ..

# 3. Package into the module zip
python3 - <<'EOF'
import os, zipfile

with zipfile.ZipFile("rpcdroid-v0.1.5.zip", "w", zipfile.ZIP_DEFLATED) as zf:
    for root, _, files in os.walk("module"):
        for name in files:
            path = os.path.join(root, name)
            arcname = os.path.relpath(path, "module").replace(os.sep, "/")
            zf.write(path, arcname)

print("Done: rpcdroid-v0.1.5.zip")
EOF
```

These three steps are also bundled in `build.sh` at the repository root.

</details>

---

## Credits

- [Kizzy](https://github.com/dead8309/Kizzy) and [KizzyRPC](https://github.com/dead8309/KizzyRPC): reference for the foreground app detection approach, the Activity/Assets structure, and the use of `application_id` and the `external-assets` endpoint.
- [navidrome/discord-rich-presence-plugin](https://github.com/navidrome/discord-rich-presence-plugin): reference for the external image registration flow (`mp:external/...`) and caching.
- [Encore Tweaks](https://github.com/Rem01Gaming/encore) (Apache-2.0): reference for the event-driven process detection approach (BinderMonitor with `onForegroundActivitiesChanged` / `onProcessDied`), the inotify-based hot-reload, and the daemon architecture.
- [Mbed TLS](https://github.com/Mbed-TLS/mbedtls) (Apache 2.0): TLS for the Gateway connection.
- [cJSON](https://github.com/DaveGamble/cJSON) (MIT): JSON parser for C.
- [KernelSU](https://github.com/tiann/KernelSU): root solution and the `kernelsu` API for the WebUI.
- [Vue](https://vuejs.org/), [Vite](https://vitejs.dev/), [Pinia](https://pinia.vuejs.org/), [Vue Router](https://router.vuejs.org/), and [Tailwind CSS](https://tailwindcss.com/): the WebUI foundation.

## License and disclaimer

Released under the [GNU General Public License v3.0](LICENSE) (GPL-3.0). Bundled third-party libraries (Mbed TLS, cJSON) remain under their own licenses.

The daemon connects to the Discord Gateway using a user token with a desktop client identity, which is outside Discord's official bot API. Use is entirely at the user's own responsibility.
