# Changelog

## 3.2.0 — first release
- LVGL 8.3+ / 9.x integration: display tracking (resolution + rotation events), PIXEL and DPI modes, font scale, UI mode.
- Integer Q16.16 hot path (no FPU), Q8 fractional inputs, string expressions, literal percent space.
- Responsive style bindings (incl. pad_all), font picking, change listeners. Zero heap.
- Packaging: ESP-IDF component + Kconfig, PlatformIO, Arduino, Zephyr module, CMake.
- Tests against LVGL 8.3 and 9.2 (ASan/UBSan, 16/32-bit coords), core parity tests, Cortex-M cross-compile CI.
