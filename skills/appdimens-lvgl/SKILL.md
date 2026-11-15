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
