#!/bin/sh
# Build v1_dashboard for i686 Linux using musl.cc cross-compiler
# Run from MSYS2 shell (C:\msys64\ucrt64\bin\bash.exe)

set -e

CROSS_DIR="/tmp/cross-i686"
CROSS_URL="https://musl.cc/i686-linux-musl-cross.tgz"
CROSS_TGZ="/tmp/i686-linux-musl-cross.tgz"
SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SRC_DIR/build_linux"

# 1. Download cross-compiler if not present
if [ ! -f "$CROSS_DIR/bin/i686-linux-musl-gcc" ]; then
    echo "==> Downloading i686-linux-musl cross-compiler..."
    curl -L -o "$CROSS_TGZ" "$CROSS_URL"
    echo "==> Extracting..."
    mkdir -p "$CROSS_DIR"
    tar -xzf "$CROSS_TGZ" -C "$CROSS_DIR" --strip-components=1
    rm -f "$CROSS_TGZ"
fi

export PATH="$CROSS_DIR/bin:$PATH"
export CC="i686-linux-musl-gcc"
export CXX="i686-linux-musl-g++"
export AR="i686-linux-musl-ar"
export RANLIB="i686-linux-musl-ranlib"

# 2. Build LVGL
echo "==> Building LVGL..."
cd "$SRC_DIR/lvgl"
mkdir -p "$BUILD_DIR/lvgl"
cd "$BUILD_DIR/lvgl"
cmake "$SRC_DIR/lvgl" \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_AR="$AR" \
    -DCMAKE_RANLIB="$RANLIB" \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DLV_CONF_PATH="$SRC_DIR/lv_conf.h" \
    -DCMAKE_C_FLAGS="-DLV_CONF_PATH=\"$SRC_DIR/lv_conf.h\""
make -j$(nproc)

echo "==> Building dashboard..."
cd "$BUILD_DIR"
cmake "$SRC_DIR" \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_AR="$AR" \
    -DCMAKE_RANLIB="$RANLIB" \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DLV_CONF_PATH="$SRC_DIR/lv_conf.h" \
    -DCMAKE_C_FLAGS="-static -DLV_CONF_PATH=\"$SRC_DIR/lv_conf.h\""
make -j$(nproc)

echo "==> Done! Binary at: $BUILD_DIR/dashboard_app"
echo "==> Copy to target: scp $BUILD_DIR/dashboard_app tc@192.168.1.246:/home/tc/"
