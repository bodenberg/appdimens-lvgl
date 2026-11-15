# Changelog

## Latest — in production (`main`)
- Custom scaling strategies: `appdimens_custom.h` (C99 single header, C++ compatible) ports `appdimens-dynamic` library-custom — shared JSON document, text formulas, code factors/formulas, rules, built-ins and presets; `adc_dp/adc_sp/adc_px`; `tests/test_custom.c` reproduces the 540-case cross-platform fixture.

## 3.2.0 — first release
- LVGL 8.3+ / 9.x integration: display tracking (resolution + rotation events), PIXEL and DPI modes, font scale, UI mode.
- Integer Q16.16 hot path (no FPU), Q8 fractional inputs, string expressions, literal percent space.
- Responsive style bindings (incl. pad_all), font picking, change listeners. Zero heap.
- Packaging: ESP-IDF component + Kconfig, PlatformIO, Arduino, Zephyr module, CMake.
- Tests against LVGL 8.3 and 9.2 (ASan/UBSan, 16/32-bit coords), core parity tests, Cortex-M cross-compile CI.
