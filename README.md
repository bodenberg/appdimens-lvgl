<div align="center">

# AppDimens for LVGL

**Design once. The same UI keeps its proportions on a 240×240 round watch, a 320×240 TFT, an 800×480 panel, a 1024×600 HMI and a 1080p kiosk — and adapts by itself when the resolution or rotation changes.**

[![Release](https://img.shields.io/badge/release-3.2.0-1d4ed8.svg)](CHANGELOG.md)
[![LVGL](https://img.shields.io/badge/LVGL-8.3%20%7C%209.x-15803d.svg)](#install)
[![MCU](https://img.shields.io/badge/hot%20path-integer%20only%20(no%20FPU)-f59e0b.svg)](#precision-and-performance)
[![License](https://img.shields.io/badge/license-Apache%202.0-15803d.svg)](LICENSE)

Part of the [AppDimens family](https://github.com/bodenberg/appdimens) — same tokens and math as Android, iOS, Flutter, React Native, Web, .NET, GTK, Qt, Python and Games.

🇧🇷 [Resumo em português](#-resumo-em-português)

</div>

## Install

| Toolchain | How |
|-----------|-----|
| **ESP-IDF** (ESP32, S2, S3, C3, C6, P4) | `idf.py add-dependency "bodenberg/appdimens-lvgl^3.2.0"` or copy into `components/` |
| **PlatformIO** | `lib_deps = bodenberg/appdimens-lvgl@^3.2.0` |
| **Arduino** | Library Manager → *AppDimens LVGL* (or zip install) |
| **Zephyr** | add as a module (`zephyr/module.yml`) |
| **CMake** (STM32CubeIDE, NXP MCUXpresso, RP2040 SDK, Linux fbdev/DRM/SDL) | `add_subdirectory(appdimens-lvgl)` → `appdimens::lvgl` |
| Anything else | add `src/lv_appdimens.c` and `src/` to your build |

Requires LVGL **8.3+ or 9.x**. No heap, no threads, no OS.

## First lines of code

```c
#include "lv_appdimens.h"

lv_appdimens_init(NULL);                          // default display; follows resolution + rotation

lv_obj_set_size(btn, lv_ad_sdp(120), lv_ad_asdp(44));
lv_obj_set_style_pad_all(card, lv_ad_sdp(12), 0);

static lv_style_t st;                              // styles that re-apply themselves on every change
lv_style_init(&st);
lv_ad_style_bind_pad_all(&st, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 12);
lv_ad_style_bind(&st, LV_STYLE_RADIUS, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 10);

static const lv_font_t * const fonts[] = { &lv_font_montserrat_14, &lv_font_montserrat_20, &lv_font_montserrat_28 };
lv_obj_set_style_text_font(label, lv_ad_font_for(fonts, 3, AD_SCALED, 0, 16), 0);  // best bitmap font for 16ssp
```

## Modes

| Mode | Design unit | Use when |
|------|-------------|----------|
| `LV_APPDIMENS_MODE_PIXEL` (default) | px of a **300 px-wide** reference | one product family, panels of similar pixel density |
| `LV_APPDIMENS_MODE_DPI` | Android dp (density = `lv_display_get_dpi() / 160`) | the same firmware on panels with very different DPI |

Text scale (accessibility): `lv_appdimens_set_font_scale(1.25f)`. UI mode: `lv_appdimens_set_ui_mode(AD_UI_WATCH)`.

## API at a glance

| Call | Returns |
|------|---------|
| `lv_ad_sdp/hdp/wdp/sdpa/asdp/ftsdp/flsdp/pwsdp/logsdp/dgsdp/fsdp(v)` | px (integer) |
| `lv_ad_ssp(v)`, `lv_ad_assp(v)` | text px × font scale |
| `lv_ad_sdpPh/sdpLw/hdpLw/wdpLh(v)` | orientation inverters |
| `lv_ad_px(strategy, AD_SLOT(variant, flags), v)` | any of the 528 combinations |
| `lv_ad_px_q8(strategy, slot, v * 256)` | fractional design values |
| `lv_ad_dimen("16sdp", fallback)` | parse from config/JSON (not per frame) |
| `lv_ad_space_w(50)`, `lv_ad_space_h(10)` | literal % of the display |
| `lv_ad_style_bind[_ex]`, `lv_ad_style_bind_pad_all`, `lv_ad_style_unbind` | responsive styles |
| `lv_ad_font_pick`, `lv_ad_font_for` | nearest bitmap font |
| `lv_appdimens_add_listener(cb, user)` | re-layout hook |

The portable core (`src/appdimens.h`) is also available directly: `ad_sdp(m, 16)` etc.

## Custom strategies

Your own scaling rule, shared with every AppDimens platform (same JSON document and text formulas),
through the C99 single header `appdimens_custom.h`:

```c
#define ADC_IMPLEMENTATION
#include "appdimens_custom.h"
adc_load_json(json_text, err, sizeof err);                       /* ids + "default" */
float pad = adc_dp(adc_get("brand"), lv_appdimens_get_metrics(), 16);
adc_strategy *hud = adc_formula("base * pow(sw / 360, 0.8)", err, sizeof err);
```

Reproduces the cross-platform 540-case fixture. Details: [docs/CUSTOM.md](docs/CUSTOM.md).

## Precision and performance

- **Integer hot path:** a token is one 64-bit integer multiply over a Q16.16 table (verified: on Cortex-M0 the call
  compiles to `__aeabi_lmul` only — no soft-float). Factors are computed once per resolution change.
- **Footprint:** ~6 KB code (Cortex-M0, `-Os`), ~5.4 KB RAM with 64 bindings (`LV_APPDIMENS_MAX_BINDINGS`).
- **Parity:** core tests check 10 704 values against reference vectors in float, double, no-libm and fixed point;
  the fixed path is within ±0.5 px of the exact value.
- **Tested** against LVGL 8.3 (16- and 32-bit coords) and 9.2 with a headless display, under ASan/UBSan; cross-compiled
  for Cortex-M0/M4/M7/M33.

Docs: [Guide](docs/GUIDE.md) · [Skill](skills/appdimens-lvgl/SKILL.md) · [LLMS.txt](LLMS.txt)

## 🇧🇷 Resumo em português

**AppDimens para LVGL** (v8.3 e v9): use `lv_ad_sdp(16)` em vez de `16` e a interface mantém a proporção em qualquer
display (relógio redondo, TFT 320×240, painel 800×480, HMI 1024×600…), atualizando sozinha quando a resolução ou a
rotação mudam. Caminho quente só com inteiros (funciona em MCU sem FPU), sem alocação dinâmica, ~6 KB. Pacotes para
ESP-IDF, PlatformIO, Arduino, Zephyr e CMake.

## Author

**Jean Bodenberg** — [@bodenberg](https://github.com/bodenberg) · Apache 2.0
