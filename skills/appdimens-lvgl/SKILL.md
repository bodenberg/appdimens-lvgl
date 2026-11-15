---
name: appdimens-lvgl
description: Use for responsive LVGL UIs (v8.3 / v9.x) on MCUs and embedded Linux — ESP32, STM32, RP2040, NXP, Zephyr, Arduino, PlatformIO, fbdev/DRM/SDL — whenever the user mentions appdimens-lvgl, lv_ad_sdp, lv_appdimens_init, supporting several display resolutions/rotations with one firmware, or porting AppDimens sdp/ssp sizes to LVGL.
---

# appdimens-lvgl — workflow

1. Install for the toolchain (ESP-IDF component, PlatformIO `bodenberg/appdimens-lvgl`, Arduino, Zephyr module, CMake).
2. Call `lv_appdimens_init(NULL)` after the display is registered.
3. Replace literal pixels: sizes/positions → `lv_ad_sdp(v)` (or `lv_ad_asdp` when the product spans small and large
   panels); text → pick fonts with `lv_ad_font_for(fonts, n, AD_SCALED, 0, size)`.
4. For styles, bind instead of setting: `lv_ad_style_bind(&st, LV_STYLE_RADIUS, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 8)`;
   they update on resolution/rotation change automatically.
5. Values are designed on a 300 px-wide reference (PIXEL mode). Use `LV_APPDIMENS_MODE_DPI` only when panels differ a lot in DPI.
6. Validate on the smallest and largest supported panel and after rotation.

Don'ts: don't call `lv_ad_dimen()` in draw loops (it parses); don't forget `lv_ad_style_unbind()` before `lv_style_reset()`.

## Custom strategies

Only when the project declares `customStrategies` (or the user asks for their own rule): one
`#define ADC_IMPLEMENTATION` + `#include "appdimens_custom.h"`, `adc_load_json(text, err, n)` at start-up, then
`adc_dp(adc_get("brand"), lv_appdimens_get_metrics(), 16)` / `adc_sp(...)`. One-off math: `adc_formula("base * pow(sw / 360, 0.8)", err, n)`. Never invent
ids; not available in `AD_FIXED_POINT` builds. Reference: `docs/CUSTOM.md`.

<!-- appdimens-family:begin -->
## Other AppDimens platforms

The same token names and math exist on Android (Compose, Views, XML), Kotlin Multiplatform, iOS/macOS, Flutter, React
Native, Web, .NET, Qt, GTK, LVGL, Roku, Python/Blender and games (Unity, Unreal, Godot, C/C++). For code on another
platform, use that library's skill — every skill, plus the layout-intent decision table, installs with
`npx appdimens-skills` (https://github.com/bodenberg/appdimens-skills). Hub: https://github.com/bodenberg/appdimens.
<!-- appdimens-family:end -->
