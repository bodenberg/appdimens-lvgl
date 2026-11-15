/**
 * @file lv_appdimens.c
 * AppDimens for LVGL v8.3+ / v9.x — implementation. Apache-2.0.
 */
#include "lv_appdimens.h"

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_style_t * style;
    int32_t value;
    uint16_t prop;
    uint8_t strategy;
    uint8_t slot;
    uint8_t kind;
} ad_binding_t;

typedef struct {
    lv_appdimens_listener_t cb;
    void * user;
} ad_listener_t;

typedef struct {
    lv_ad_display_t * disp;
    ad_metrics m;
    int32_t pxq[AD_STRATEGY_COUNT][AD_SLOT_COUNT]; /* factor × density, Q16.16 */
    int32_t font_q;                                 /* font scale, Q16.16 */
    float font_scale;
    ad_ui_mode ui_mode;
    lv_appdimens_mode_t mode;
    int32_t hor, ver, dpi;
    bool manual;
    ad_binding_t bindings[LV_APPDIMENS_MAX_BINDINGS];
    uint16_t binding_count;
    ad_listener_t listeners[LV_APPDIMENS_MAX_LISTENERS];
    uint8_t listener_count;
#if LVGL_VERSION_MAJOR < 9
    lv_obj_t * watched;
#endif
} ad_state_t;

/**********************
 *  STATIC VARIABLES
 **********************/
static ad_state_t ad_s;
static bool ad_ready;

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int32_t q16(double v)
{
    double r = v * 65536.0 + (v >= 0 ? 0.5 : -0.5);
    if(r > 2147483647.0) return 2147483647;
    if(r < -2147483647.0) return -2147483647;
    return (int32_t)r;
}

static lv_ad_coord_t clamp_coord(int64_t v)
{
    /* v8: lv_coord_t is int16_t unless LV_USE_LARGE_COORD; v9: int32_t. */
    const int64_t lim = (int64_t)LV_COORD_MAX;
    if(v > lim) return (lv_ad_coord_t)lim;
    if(v < -lim) return (lv_ad_coord_t)(-lim);
    return (lv_ad_coord_t)v;
}

static int32_t mul_q16(int32_t v, int32_t q)
{
    int64_t r = (int64_t)v * q;
    return (int32_t)((r + (r >= 0 ? 32768 : -32768)) / 65536);
}

static void read_display(int32_t * hor, int32_t * ver, int32_t * dpi)
{
    lv_ad_display_t * d = ad_s.disp;
#if LVGL_VERSION_MAJOR >= 9
    *hor = lv_display_get_horizontal_resolution(d);
    *ver = lv_display_get_vertical_resolution(d);
    *dpi = lv_display_get_dpi(d);
#else
    *hor = lv_disp_get_hor_res(d);
    *ver = lv_disp_get_ver_res(d);
    *dpi = lv_disp_get_dpi(d);
#endif
}

static void apply_bindings(void)
{
    for(uint16_t i = 0; i < ad_s.binding_count; i++) {
        ad_binding_t * b = &ad_s.bindings[i];
        lv_style_value_t v;
        v.num = b->kind == AD_KIND_SP ? lv_ad_sp_px((ad_strategy)b->strategy, b->slot, b->value)
                                      : lv_ad_px((ad_strategy)b->strategy, b->slot, b->value);
        lv_style_set_prop(b->style, (lv_style_prop_t)b->prop, v);
    }
    /* One report per distinct style. */
    for(uint16_t i = 0; i < ad_s.binding_count; i++) {
        bool seen = false;
        for(uint16_t j = 0; j < i; j++) if(ad_s.bindings[j].style == ad_s.bindings[i].style) { seen = true; break; }
        if(!seen) lv_obj_report_style_change(ad_s.bindings[i].style);
    }
}

static bool publish(int32_t hor, int32_t ver, int32_t dpi)
{
    ad_input in;
    ad_input_default(&in);
    const double density = (ad_s.mode == LV_APPDIMENS_MODE_DPI && dpi > 0) ? (double)dpi / 160.0 : 1.0;
    in.width = (ad_real)(hor / density);
    in.height = (ad_real)(ver / density);
    in.density = (ad_real)density;
    in.font_scale = (ad_real)(ad_s.font_scale > 0 ? ad_s.font_scale : 1.0f);
    in.dpi = (ad_real)(dpi > 0 ? dpi : 160);
    in.ui_mode = ad_s.ui_mode;
    ad_metrics next;
    ad_metrics_init(&next, &in);
    if(ad_ready && ad_metrics_same(&next, &ad_s.m)) return false;
    next.generation = ad_s.m.generation + 1;
    ad_s.m = next;
    ad_s.hor = hor;
    ad_s.ver = ver;
    ad_s.dpi = dpi;
    for(int s = 0; s < AD_STRATEGY_COUNT; s++)
        for(int i = 0; i < AD_SLOT_COUNT; i++) ad_s.pxq[s][i] = q16((double)next.table[s][i] * density);
    ad_s.font_q = q16((double)next.in.font_scale);
    ad_ready = true;
    apply_bindings();
    for(uint8_t i = 0; i < ad_s.listener_count; i++) ad_s.listeners[i].cb(&ad_s.m, ad_s.listeners[i].user);
    return true;
}

#if LVGL_VERSION_MAJOR >= 9
static void on_resolution(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_appdimens_refresh();
}
#else
static void on_size(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_appdimens_refresh();
}
#endif

static void detach(void)
{
    if(!ad_s.disp) return;
#if LVGL_VERSION_MAJOR >= 9
    lv_display_remove_event_cb_with_user_data(ad_s.disp, on_resolution, NULL);
#else
    if(ad_s.watched) lv_obj_remove_event_cb(ad_s.watched, on_size);
    ad_s.watched = NULL;
#endif
    ad_s.disp = NULL;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_appdimens_init(lv_ad_display_t * disp)
{
    if(!ad_ready) {
        ad_s.font_scale = 1.0f;
        ad_s.ui_mode = AD_UI_NORMAL;
        ad_s.mode = LV_APPDIMENS_MODE_PIXEL;
        ad_s.binding_count = 0;
        ad_s.listener_count = 0;
        ad_s.disp = NULL;
#if LVGL_VERSION_MAJOR < 9
        ad_s.watched = NULL;
#endif
        publish(300, 533, 160);
    }
    detach();
    ad_s.manual = false;
#if LVGL_VERSION_MAJOR >= 9
    ad_s.disp = disp ? disp : lv_display_get_default();
    if(ad_s.disp) lv_display_add_event_cb(ad_s.disp, on_resolution, LV_EVENT_RESOLUTION_CHANGED, NULL);
#else
    ad_s.disp = disp ? disp : lv_disp_get_default();
    if(ad_s.disp) {
        ad_s.watched = lv_disp_get_layer_sys(ad_s.disp);
        lv_obj_add_event_cb(ad_s.watched, on_size, LV_EVENT_SIZE_CHANGED, NULL);
    }
#endif
    lv_appdimens_refresh();
}

void lv_appdimens_deinit(void)
{
    detach();
    ad_s.binding_count = 0;
    ad_s.listener_count = 0;
    ad_ready = false;
}

bool lv_appdimens_refresh(void)
{
    if(!ad_s.disp) return false;
    int32_t hor, ver, dpi;
    read_display(&hor, &ver, &dpi);
    if(hor <= 0 || ver <= 0) return false;
    ad_s.manual = false;
    return publish(hor, ver, dpi);
}

static void republish(void)
{
    int32_t hor = ad_s.hor, ver = ad_s.ver, dpi = ad_s.dpi;
    if(ad_s.disp && !ad_s.manual) read_display(&hor, &ver, &dpi);
    ad_ready = false; /* force */
    publish(hor, ver, dpi);
}

void lv_appdimens_set_mode(lv_appdimens_mode_t mode)
{
    ad_s.mode = mode;
    republish();
}

void lv_appdimens_set_font_scale(float font_scale)
{
    ad_s.font_scale = font_scale > 0 ? font_scale : 1.0f;
    republish();
}

void lv_appdimens_set_ui_mode(ad_ui_mode mode)
{
    ad_s.ui_mode = mode;
    republish();
}

void lv_appdimens_set_resolution(int32_t hor_res, int32_t ver_res, int32_t dpi)
{
    if(!ad_ready) lv_appdimens_init(NULL);
    ad_s.manual = true;
    publish(hor_res, ver_res, dpi);
}

const ad_metrics * lv_appdimens_get_metrics(void)
{
    if(!ad_ready) lv_appdimens_init(NULL);
    return &ad_s.m;
}

bool lv_appdimens_add_listener(lv_appdimens_listener_t cb, void * user_data)
{
    if(!cb || ad_s.listener_count >= LV_APPDIMENS_MAX_LISTENERS) return false;
    ad_s.listeners[ad_s.listener_count].cb = cb;
    ad_s.listeners[ad_s.listener_count].user = user_data;
    ad_s.listener_count++;
    return true;
}

void lv_appdimens_remove_listener(lv_appdimens_listener_t cb, void * user_data)
{
    for(uint8_t i = 0; i < ad_s.listener_count; i++) {
        if(ad_s.listeners[i].cb == cb && ad_s.listeners[i].user == user_data) {
            ad_s.listeners[i] = ad_s.listeners[--ad_s.listener_count];
            return;
        }
    }
}

lv_ad_coord_t lv_ad_px(ad_strategy strategy, int slot, int32_t value)
{
    if(!ad_ready) lv_appdimens_init(NULL);
    if((unsigned)strategy >= AD_STRATEGY_COUNT || (unsigned)slot >= AD_SLOT_COUNT) return (lv_ad_coord_t)value;
    return clamp_coord(mul_q16(value, ad_s.pxq[strategy][slot]));
}

lv_ad_coord_t lv_ad_sp_px(ad_strategy strategy, int slot, int32_t value)
{
    if(!ad_ready) lv_appdimens_init(NULL);
    if((unsigned)strategy >= AD_STRATEGY_COUNT || (unsigned)slot >= AD_SLOT_COUNT) return (lv_ad_coord_t)value;
    /* |value| ≤ 2^15, factor ≤ 2^22 (64×), font ≤ 2^18 (4×) → product < 2^55: fits int64 (Q32). */
    const int64_t r = (int64_t)value * ad_s.pxq[strategy][slot] * ad_s.font_q;
    const int64_t half = (int64_t)1 << 31;
    return clamp_coord((r + (r >= 0 ? half : -half)) / ((int64_t)1 << 32));
}

lv_ad_coord_t lv_ad_px_q8(ad_strategy strategy, int slot, int32_t value_q8)
{
    if(!ad_ready) lv_appdimens_init(NULL);
    if((unsigned)strategy >= AD_STRATEGY_COUNT || (unsigned)slot >= AD_SLOT_COUNT) return (lv_ad_coord_t)(value_q8 / 256);
    int64_t r = (int64_t)value_q8 * ad_s.pxq[strategy][slot]; /* Q24 */
    return clamp_coord((r + (r >= 0 ? (1 << 23) : -(1 << 23))) / (1 << 24));
}

lv_ad_coord_t lv_ad_dimen(const char * expr, lv_ad_coord_t fallback)
{
    if(!ad_ready) lv_appdimens_init(NULL);
    ad_real v;
    if(!expr || !ad_dimen(&ad_s.m, expr, &v)) return fallback;
    /* logical → px */
    double px = (double)v * (double)ad_s.m.in.density;
    return clamp_coord((int64_t)(px >= 0 ? px + 0.5 : px - 0.5));
}

bool lv_ad_style_bind_ex(lv_style_t * style, lv_style_prop_t prop, ad_strategy strategy, int slot, ad_kind kind,
                         int32_t value)
{
    if(!style || (unsigned)strategy >= AD_STRATEGY_COUNT || (unsigned)slot >= AD_SLOT_COUNT) return false;
    if(!ad_ready) lv_appdimens_init(NULL);
    ad_binding_t * b = NULL;
    for(uint16_t i = 0; i < ad_s.binding_count; i++)
        if(ad_s.bindings[i].style == style && ad_s.bindings[i].prop == (uint16_t)prop) { b = &ad_s.bindings[i]; break; }
    if(!b) {
        if(ad_s.binding_count >= LV_APPDIMENS_MAX_BINDINGS) return false;
        b = &ad_s.bindings[ad_s.binding_count++];
    }
    b->style = style;
    b->prop = (uint16_t)prop;
    b->strategy = (uint8_t)strategy;
    b->slot = (uint8_t)slot;
    b->kind = (uint8_t)kind;
    b->value = value;
    lv_style_value_t v;
    v.num = kind == AD_KIND_SP ? lv_ad_sp_px(strategy, slot, value) : lv_ad_px(strategy, slot, value);
    lv_style_set_prop(style, prop, v);
    lv_obj_report_style_change(style);
    return true;
}

bool lv_ad_style_bind(lv_style_t * style, lv_style_prop_t prop, ad_strategy strategy, int slot, int32_t value)
{
    return lv_ad_style_bind_ex(style, prop, strategy, slot, AD_KIND_DP, value);
}

bool lv_ad_style_bind_pad_all(lv_style_t * style, ad_strategy strategy, int slot, int32_t value)
{
    return lv_ad_style_bind(style, LV_STYLE_PAD_TOP, strategy, slot, value) &&
           lv_ad_style_bind(style, LV_STYLE_PAD_BOTTOM, strategy, slot, value) &&
           lv_ad_style_bind(style, LV_STYLE_PAD_LEFT, strategy, slot, value) &&
           lv_ad_style_bind(style, LV_STYLE_PAD_RIGHT, strategy, slot, value);
}

void lv_ad_style_unbind(lv_style_t * style)
{
    uint16_t w = 0;
    for(uint16_t i = 0; i < ad_s.binding_count; i++)
        if(ad_s.bindings[i].style != style) ad_s.bindings[w++] = ad_s.bindings[i];
    ad_s.binding_count = w;
}

const lv_font_t * lv_ad_font_pick(const lv_font_t * const * fonts, uint32_t count, int32_t target_px)
{
    const lv_font_t * best = NULL, * smallest = NULL;
    for(uint32_t i = 0; i < count; i++) {
        const lv_font_t * f = fonts[i];
        if(!f) continue;
        const int32_t h = (int32_t)f->line_height;
        if(!smallest || h < (int32_t)smallest->line_height) smallest = f;
        if(h <= target_px && (!best || h > (int32_t)best->line_height)) best = f;
    }
    return best ? best : smallest;
}

const lv_font_t * lv_ad_font_for(const lv_font_t * const * fonts, uint32_t count, ad_strategy strategy, int slot,
                                 int32_t value)
{
    return lv_ad_font_pick(fonts, count, lv_ad_sp_px(strategy, slot, value));
}
