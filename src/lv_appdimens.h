/**
 * @file lv_appdimens.h
 * AppDimens for LVGL v8.3+ and v9.x — responsive sizes for MCUs, SBCs and embedded Linux.
 *
 * Write the size once (`lv_ad_sdp(16)`), get the right number of pixels on a 240×240 round
 * watch, a 320×240 TFT, an 800×480 panel, a 1024×600 HMI or a 1920×1080 kiosk — and it
 * updates by itself when the display resolution or rotation changes.
 *
 *   lv_appdimens_init(NULL);                       // default display
 *   lv_obj_set_size(btn, lv_ad_sdp(120), lv_ad_asdp(44));
 *   lv_ad_style_bind_pad_all(&style, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 12);
 *
 * Integer hot path (no FPU needed): one 64-bit multiply over a Q16.16 table built once per
 * resolution change. Zero heap allocation. Version 3.2.0 · Apache-2.0.
 */
#ifndef LV_APPDIMENS_H
#define LV_APPDIMENS_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#include "appdimens.h"

/*********************
 *      CONFIG
 *********************/
/** Max style bindings re-applied on resolution change (static storage, 12 bytes each). */
#ifndef LV_APPDIMENS_MAX_BINDINGS
#define LV_APPDIMENS_MAX_BINDINGS 64
#endif
/** Max change listeners. */
#ifndef LV_APPDIMENS_MAX_LISTENERS
#define LV_APPDIMENS_MAX_LISTENERS 8
#endif

#if LVGL_VERSION_MAJOR >= 9
typedef lv_display_t lv_ad_display_t;
typedef int32_t lv_ad_coord_t;
#else
typedef lv_disp_t lv_ad_display_t;
typedef lv_coord_t lv_ad_coord_t;
#endif

/** How design values map to pixels. */
typedef enum {
    /** Design values are pixels of a 300 px-wide reference canvas (density 1). Default:
     *  what most embedded designers expect — `lv_ad_sdp(16)` is 16 px on a 300 px-wide
     *  screen and 25.6 px on a 480 px-wide one. */
    LV_APPDIMENS_MODE_PIXEL = 0,
    /** Android-like dp: density = display DPI / 160 (LVGL `lv_dpx` semantics). Use when the
     *  same firmware runs on panels with very different pixel densities. */
    LV_APPDIMENS_MODE_DPI = 1,
} lv_appdimens_mode_t;

typedef void (*lv_appdimens_listener_t)(const ad_metrics * m, void * user_data);

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
/** Tracks `disp` (NULL = default display). Safe to call again to switch display. */
void lv_appdimens_init(lv_ad_display_t * disp);
/** Stops tracking and clears bindings/listeners. */
void lv_appdimens_deinit(void);
/** Re-reads the display (call after a manual driver change on custom ports). */
bool lv_appdimens_refresh(void);
/** Sets the mapping mode (default PIXEL). Recomputes immediately. */
void lv_appdimens_set_mode(lv_appdimens_mode_t mode);
/** Overrides the text scale (accessibility / user setting). ≤ 0 → 1. */
void lv_appdimens_set_font_scale(float font_scale);
/** Sets the UI mode (e.g. AD_UI_WATCH, AD_UI_CAR, AD_UI_APPLIANCE). */
void lv_appdimens_set_ui_mode(ad_ui_mode mode);
/** Publishes a snapshot directly (simulators, tests, non-LVGL sources). Values in px. */
void lv_appdimens_set_resolution(int32_t hor_res, int32_t ver_res, int32_t dpi);
/** Current snapshot (logical units; px in PIXEL mode). */
const ad_metrics * lv_appdimens_get_metrics(void);
/** Registers a listener (returns false when the table is full). */
bool lv_appdimens_add_listener(lv_appdimens_listener_t cb, void * user_data);
void lv_appdimens_remove_listener(lv_appdimens_listener_t cb, void * user_data);

/** Pixels for `value` through `strategy` and table `slot` (AD_SLOT(variant, flags)). Integer only. */
lv_ad_coord_t lv_ad_px(ad_strategy strategy, int slot, int32_t value);
/** Text pixels (× font scale). */
lv_ad_coord_t lv_ad_sp_px(ad_strategy strategy, int slot, int32_t value);
/** Same as lv_ad_px for fractional design values (value in 1/256 units, e.g. 12.5 → 3200). */
lv_ad_coord_t lv_ad_px_q8(ad_strategy strategy, int slot, int32_t value_q8);
/** Resolves "16sdp", "14ssp", "50spaceW", "5mm" to pixels (parse cost: use for config, not per frame). */
lv_ad_coord_t lv_ad_dimen(const char * expr, lv_ad_coord_t fallback);

/**
 * Binds a numeric style property (pad, width, height, radius, border/outline width,
 * line/arc width, translate, shadow…) to a token: the value is set now and re-applied on
 * every resolution change, then lv_obj_report_style_change() refreshes the objects.
 * `kind` AD_KIND_SP multiplies by the font scale. Returns false when the table is full.
 */
bool lv_ad_style_bind(lv_style_t * style, lv_style_prop_t prop, ad_strategy strategy, int slot, int32_t value);
bool lv_ad_style_bind_ex(lv_style_t * style, lv_style_prop_t prop, ad_strategy strategy, int slot, ad_kind kind,
                         int32_t value);
/** Binds PAD_TOP/BOTTOM/LEFT/RIGHT at once (uses 4 binding slots). */
bool lv_ad_style_bind_pad_all(lv_style_t * style, ad_strategy strategy, int slot, int32_t value);
/** Removes every binding of `style` (call before lv_style_reset). */
void lv_ad_style_unbind(lv_style_t * style);

/** Picks, from `fonts` (any order), the largest font whose line height ≤ target_px; else the smallest. */
const lv_font_t * lv_ad_font_pick(const lv_font_t * const * fonts, uint32_t count, int32_t target_px);
/** Font for a text token: lv_ad_font_pick(fonts, n, lv_ad_sp_px(strategy, slot, value)). */
const lv_font_t * lv_ad_font_for(const lv_font_t * const * fonts, uint32_t count, ad_strategy strategy, int slot,
                                 int32_t value);

/**********************
 *      MACROS
 **********************/
/* The usual tokens as integer pixel macros. */
#define lv_ad_sdp(v)    lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_hdp(v)    lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_H, 0), (v))
#define lv_ad_wdp(v)    lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_W, 0), (v))
#define lv_ad_sdpa(v)   lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_S, AD_FLAG_A), (v))
#define lv_ad_ssp(v)    lv_ad_sp_px(AD_SCALED, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_sem(v)    lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_asdp(v)   lv_ad_px(AD_AUTO, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_assp(v)   lv_ad_sp_px(AD_AUTO, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_ftsdp(v)  lv_ad_px(AD_FIT, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_flsdp(v)  lv_ad_px(AD_FILL, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_pwsdp(v)  lv_ad_px(AD_POWER, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_logsdp(v) lv_ad_px(AD_LOGARITHMIC, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_dgsdp(v)  lv_ad_px(AD_DIAGONAL, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_fsdp(v)   lv_ad_px(AD_FLUID, AD_SLOT(AD_VAR_S, 0), (v))
#define lv_ad_sdpPh(v)  lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_S_PH, 0), (v))
#define lv_ad_sdpLw(v)  lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_S_LW, 0), (v))
#define lv_ad_hdpLw(v)  lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_H_LW, 0), (v))
#define lv_ad_wdpLh(v)  lv_ad_px(AD_SCALED, AD_SLOT(AD_VAR_W_LH, 0), (v))
/** Literal percentage of the width/height (like lv_pct but resolved to px). */
#define lv_ad_space_w(p) ((lv_ad_coord_t)(((int64_t)(p) * lv_appdimens_get_metrics()->in.width + 50) / 100))
#define lv_ad_space_h(p) ((lv_ad_coord_t)(((int64_t)(p) * lv_appdimens_get_metrics()->in.height + 50) / 100))

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_APPDIMENS_H*/
