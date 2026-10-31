#!/bin/bash
# build.sh — Build discord-rpc Magisk/KSU module
# Usage: ./build.sh [--skip-daemon] [--skip-webui] [--skip-zip]
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

SKIP_DAEMON=0
SKIP_WEBUI=0
SKIP_ZIP=0

for arg in "$@"; do
    case $arg in
        --skip-daemon) SKIP_DAEMON=1 ;;
        --skip-webui)  SKIP_WEBUI=1  ;;
        --skip-zip)    SKIP_ZIP=1    ;;
    esac
done

echo "=== RPCDroid Module Builder ==="

# ── 1. Build daemon (C / NDK) ────────────────────────────────────────────────
if [ "$SKIP_DAEMON" = "0" ]; then
    echo ""
    echo "[1/3] Building daemon..."
    cd "$SCRIPT_DIR/daemon"
    bash build.sh
    cd "$SCRIPT_DIR"
    echo "      Daemon built."

    # Copy & strip daemon binaries into module/libs
    for ABI in arm64-v8a armeabi-v7a x86_64; do
        mkdir -p "$SCRIPT_DIR/module/libs/$ABI"
        cp "$SCRIPT_DIR/daemon/libs/$ABI/rpcdroid" "$SCRIPT_DIR/module/libs/$ABI/rpcdroid"
    done
    echo "      Binaries copied to module/libs/"
else
    echo "[1/3] Daemon build skipped."
fi

# Verify binaries exist
for ABI in arm64-v8a armeabi-v7a; do
    BIN="$SCRIPT_DIR/module/libs/$ABI/rpcdroid"
    if [ ! -f "$BIN" ]; then
        echo "ERROR: Missing $BIN"
        echo "       Run without --skip-daemon or copy binaries manually."
        exit 1
    fi
done

# ── 2. Build WebUI (Vue 3 / Vite) ───────────────────────────────────────────
if [ "$SKIP_WEBUI" = "0" ]; then
    echo ""
    echo "[2/3] Building WebUI..."
    cd "$SCRIPT_DIR/webui"
    if ! command -v npm >/dev/null 2>&1; then
        echo "ERROR: npm not found. Install Node.js first."
        exit 1
    fi
    npm install --silent
    npm run build
    cd "$SCRIPT_DIR"
    echo "      WebUI built → module/webroot/"
else
    echo "[2/3] WebUI build skipped."
fi

# Verify webroot has content
if [ ! -f "$SCRIPT_DIR/module/webroot/index.html" ]; then
    echo "ERROR: module/webroot/index.html missing."
    echo "       Run without --skip-webui or build WebUI manually."
    exit 1
fi

# ── 3. Package module zip ────────────────────────────────────────────────────
if [ "$SKIP_ZIP" = "0" ]; then
    echo ""
    echo "[3/3] Packaging module zip..."

    VERSION=$(grep '^version=' "$SCRIPT_DIR/module/module.prop" | cut -d= -f2)
    OUT="$SCRIPT_DIR/rpcdroid-${VERSION}.zip"

    cd "$SCRIPT_DIR/module"

    # Remove old zip
    rm -f "$OUT"

    # Create zip — include everything in module/ plus daemon binaries
    zip -r "$OUT" \
        module.prop \
        customize.sh \
        service.sh \
        action.sh \
        uninstall.sh \
        gamelist.txt \
        bin/ \
        META-INF/ \
        webroot/ \
        libs/ \
        -x "*.gitkeep" \
        -x "*.DS_Store" \
        -x "__MACOSX/*" \
        >/dev/null

    cd "$SCRIPT_DIR"

    SIZE=$(du -sh "$OUT" | cut -f1)
    echo "      Done: $(basename "$OUT") ($SIZE)"
    echo ""
    echo "=== Build complete ==="
    echo "    Install via KSU/APatch/Magisk: $OUT"
else
    echo "[3/3] Zip packaging skipped."
fi
