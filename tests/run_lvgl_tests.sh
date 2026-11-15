#!/usr/bin/env bash
# Usage: tests/run_lvgl_tests.sh <path-to-lvgl> <8|9> [build-dir]
set -euo pipefail
LVGL=$1; V=$2; OUT=${3:-build/lvgl$V}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$OUT/obj"
CFLAGS="-std=gnu99 -O1 -g -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE -I$ROOT/tests/conf${CONF:-$V} -I$LVGL -I$LVGL/src -I$ROOT/src ${EXTRA_CFLAGS:-}"
find "$LVGL/src" -name '*.c' > "$OUT/srcs.txt"
xargs -P"$(nproc)" -I{} sh -c 'o="$0/obj/$(echo {} | md5sum | cut -c1-12).o"; [ -f "$o" ] || cc '"$CFLAGS"' -w -c {} -o "$o"' "$OUT" < "$OUT/srcs.txt"
cc $CFLAGS -Wall -Wextra -Werror -Wno-unused-parameter -c "$ROOT/src/lv_appdimens.c" -o "$OUT/lv_appdimens.o"
cc $CFLAGS -Wall -Wno-unused-parameter "$ROOT/tests/test_lvgl.c" "$OUT/lv_appdimens.o" "$OUT"/obj/*.o -o "$OUT/test_lvgl" -lm
"$OUT/test_lvgl"
