# AppDimens LVGL — guide

## Choosing strategies on embedded displays
- `sdp` — default; scales with the short side (works for both landscape panels and portrait phones-style UIs).
- `asdp` — products that ship small (320×240) **and** large (1024×600, 1280×800) panels: big panels get calmer growth.
- `ftsdp` — content that must never be cropped when the aspect ratio changes (4:3 → 16:9 → 32:9 bar displays).
- `flsdp` — backgrounds that must cover.
- `hdpLw` / `sdpPh` — elements that should follow the *long* side after rotation.

## Rotation and resolution
LVGL v9 sends `LV_EVENT_RESOLUTION_CHANGED` (also on rotation); v8 sends `LV_EVENT_SIZE_CHANGED` to the screens on
`lv_disp_drv_update()`. Both are hooked by `lv_appdimens_init()`. Custom ports can call `lv_appdimens_refresh()` or
`lv_appdimens_set_resolution(w, h, dpi)`.

## Fonts
LVGL bitmap fonts come in fixed sizes; `lv_ad_font_for()` picks the largest that fits the computed size. With
Tiny TTF / FreeType you can render the exact size: `lv_tiny_ttf_create_data(data, size, lv_ad_ssp(16))`.

## Memory
`ad_metrics` (float table, 2.1 KB) + Q16 pixel table (2.1 KB) + bindings (12 B each). Lower
`LV_APPDIMENS_MAX_BINDINGS` on very small parts.
