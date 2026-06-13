#!/usr/bin/env bash
set -euo pipefail

# Cross-compile dashboard_app for i686 Linux using the apt toolchain.
# Usage: ./build_cross.sh
# Output: build_linux/dashboard_app (statically linked)

CC="${CC:-i686-linux-gnu-gcc}"
AR="${AR:-i686-linux-gnu-ar}"
BUILD_DIR="build_linux"
NAME="dashboard_app"

CFLAGS="-Os -std=c99 -D_GNU_SOURCE -DLV_CONF_PATH=$(pwd)/lv_conf.h"
CFLAGS="$CFLAGS -I. -I./lvgl"
LDFLAGS="-static -lm -lpthread -lrt"

# Files to exclude from LVGL (platform-specific drivers and optional libs)
EXCLUDE=(
  lvgl/src/draw/sdl/
  lvgl/src/drivers/sdl/
  lvgl/src/drivers/windows/
  lvgl/src/drivers/nuttx/
  lvgl/src/drivers/libinput/
  lvgl/src/drivers/x11/
  lvgl/src/libs/freetype/
  lvgl/src/libs/ffmpeg/
  lvgl/src/libs/libjpeg_turbo/
  lvgl/src/libs/libpng/
  lvgl/src/libs/lodepng/
  lvgl/src/libs/rlottie/
  lvgl/src/libs/tiny_ttf/
  lvgl/src/libs/bmp/
  lvgl/src/libs/gif/
  lvgl/src/libs/qrcode/
  lvgl/src/libs/barcode/
  lvgl/src/libs/fsdrv/
  lvgl/src/others/
)

echo "==> Compiling dashboard_app for i686 Linux..."
mkdir -p "$BUILD_DIR"

# Build the list of source files
SRC=""
SRC="$SRC main.c"
SRC="$SRC $(find src -name '*.c')"
SRC="$SRC $(find lvgl/src -name '*.c' | grep -vFf <(printf '%s\n' "${EXCLUDE[@]}") || true)"

OBJ=""
for f in $SRC; do
  o="$BUILD_DIR/${f//\//_}.o"
  OBJ="$OBJ $o"
  if [ ! -f "$o" ]; then
    echo "  CC $f"
    $CC $CFLAGS -c "$f" -o "$o"
  fi
done

echo "  LD $NAME"
$CC $LDFLAGS -o "$BUILD_DIR/$NAME" $OBJ

echo "==> Done: $BUILD_DIR/$NAME"
file "$BUILD_DIR/$NAME"
ls -lh "$BUILD_DIR/$NAME"
