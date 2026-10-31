#!/bin/bash
set -e

NDK="${ANDROID_NDK_HOME:-$HOME/android-ndk}"

if [ ! -d "$NDK" ]; then
    echo "ERROR: Android NDK not found at $NDK"
    echo "Set ANDROID_NDK_HOME environment variable"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Download mbedTLS if not present
if [ ! -d "mbedtls" ]; then
    echo "Downloading mbedTLS..."
    MBEDTLS_VER="3.6.0"
    curl -L "https://github.com/Mbed-TLS/mbedtls/releases/download/v${MBEDTLS_VER}/mbedtls-${MBEDTLS_VER}.tar.bz2" \
        | tar -xj
    mv "mbedtls-${MBEDTLS_VER}" mbedtls
    echo "mbedTLS downloaded."
fi

echo "Building for arm64-v8a and armeabi-v7a..."
"$NDK/ndk-build" \
    NDK_PROJECT_PATH="$SCRIPT_DIR" \
    APP_BUILD_SCRIPT="$SCRIPT_DIR/Android.mk" \
    NDK_APPLICATION_MK="$SCRIPT_DIR/Application.mk" \
    NDK_OUT="$SCRIPT_DIR/build" \
    NDK_LIBS_OUT="$SCRIPT_DIR/libs" \
    -j$(nproc 2>/dev/null || echo 4)

echo "Stripping binaries..."
find libs -name "rpcdroid" -exec "$NDK/toolchains/llvm/prebuilt/"*/bin/llvm-strip {} \;

echo ""
echo "Done! Binaries in:"
find libs -name "rpcdroid" -exec echo "  {}" \;

# Copy to module
mkdir -p ../module/libs/arm64-v8a ../module/libs/armeabi-v7a ../module/libs/x86_64
cp -f libs/arm64-v8a/rpcdroid   ../module/libs/arm64-v8a/ 2>/dev/null || true
cp -f libs/armeabi-v7a/rpcdroid ../module/libs/armeabi-v7a/ 2>/dev/null || true
cp -f libs/x86_64/rpcdroid      ../module/libs/x86_64/ 2>/dev/null || true
echo "Copied to module/libs/"
