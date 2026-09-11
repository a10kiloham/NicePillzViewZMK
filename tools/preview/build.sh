#!/bin/bash
# Builds and runs the host preview. Needs gcc and a checkout of LVGL 8.3
# (default: the west workspace's copy in .zmk/modules/lib/gui/lvgl).
set -e
cd "$(dirname "$0")"
LVGL=${LVGL:-../../.zmk/modules/lib/gui/lvgl}
SHIELD=../../boards/shields/nicepillz
mkdir -p build/obj
CFLAGS="-O1 -w -DLV_CONF_INCLUDE_SIMPLE -I. -I$LVGL -I$SHIELD"
if [ ! -f build/liblvgl.a ]; then
  find "$LVGL/src" -name '*.c' | xargs -P "$(nproc)" -I{} sh -c 'gcc '"$CFLAGS"' -c "$1" -o build/obj/$(echo "$1" | md5sum | cut -c1-16).o' _ {}
  ar rcs build/liblvgl.a build/obj/*.o
fi
gcc $CFLAGS -o build/preview main.c "$SHIELD/view_draw.c" build/liblvgl.a -lm
./build/preview
python3 compose.py
