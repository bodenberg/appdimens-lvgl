/* LVGL integration test — builds against LVGL v8.3 and v9.x with a headless display. */
#include <stdio.h>
#include <stdlib.h>
#include "lv_appdimens.h"

static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); fails++; } } while (0)
static int heard = 0;
static void listener(const ad_metrics *m, void *u) { (void)m; (*(int *)u)++; }

#if LVGL_VERSION_MAJOR >= 9
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *px) { (void)a; (void)px; lv_display_flush_ready(d); }
static uint8_t buf[1920 * 40 * 4];
static lv_display_t *make_display(int w, int h) {
    lv_display_t *d = lv_display_create(w, h);
    lv_display_set_flush_cb(d, flush);
    lv_display_set_buffers(d, buf, NULL, sizeof buf, LV_DISPLAY_RENDER_MODE_PARTIAL);
    return d;
}
static void set_res(lv_display_t *d, int w, int h) { lv_display_set_resolution(d, w, h); }
static void set_rot90(lv_display_t *d) { lv_display_set_rotation(d, LV_DISPLAY_ROTATION_90); }
#else
static void flush(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *px) { (void)a; (void)px; lv_disp_flush_ready(d); }
static lv_color_t buf[1920 * 40];
static lv_disp_draw_buf_t dbuf;
static lv_disp_drv_t drv;
static lv_disp_t *make_display(int w, int h) {
    lv_disp_draw_buf_init(&dbuf, buf, NULL, sizeof buf / sizeof buf[0]);
    lv_disp_drv_init(&drv);
    drv.hor_res = w; drv.ver_res = h; drv.flush_cb = flush; drv.draw_buf = &dbuf;
    return lv_disp_drv_register(&drv);
}
static void set_res(lv_disp_t *d, int w, int h) { drv.hor_res = w; drv.ver_res = h; lv_disp_drv_update(d, &drv); }
static void set_rot90(lv_disp_t *d) { drv.sw_rotate = 1; drv.rotated = LV_DISP_ROT_90; lv_disp_drv_update(d, &drv); }
#endif

int main(void) {
    lv_init();
    lv_ad_display_t *disp = make_display(320, 240);
    lv_appdimens_init(NULL);
    lv_appdimens_add_listener(listener, &heard);
    const ad_metrics *m = lv_appdimens_get_metrics();
    CHECK(m->in.width == 320 && m->in.height == 240, "size %g x %g", (double)m->in.width, (double)m->in.height);
    /* 320x240 → sw 240 → 16 * 240/300 = 12.8 → 13 */
    CHECK(lv_ad_sdp(16) == 13, "sdp %d", (int)lv_ad_sdp(16));
    CHECK(lv_ad_wdp(30) == 32, "wdp %d", (int)lv_ad_wdp(30));
    CHECK(lv_ad_ftsdp(100) == 60, "ftsdp %d", (int)lv_ad_ftsdp(100));  /* fit: min(240/300, 320/533) = 0.6004 */
    CHECK(lv_ad_dimen("16sdp", -1) == 13, "dimen");
    CHECK(lv_ad_dimen("nope", -1) == -1, "dimen fallback");
    CHECK(lv_ad_space_w(50) == 160, "space_w");

    static lv_style_t st;
    lv_style_init(&st);
    CHECK(lv_ad_style_bind(&st, LV_STYLE_PAD_TOP, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 20), "bind");
    CHECK(lv_ad_style_bind_ex(&st, LV_STYLE_WIDTH, AD_AUTO, AD_SLOT(AD_VAR_S, 0), AD_KIND_DP, 100), "bind2");
    CHECK(lv_ad_style_bind_pad_all(&st, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 20), "pad_all");
    lv_obj_t *box = lv_obj_create(lv_scr_act());
    lv_obj_add_style(box, &st, 0);
    lv_style_value_t v;
    lv_style_get_prop(&st, LV_STYLE_PAD_TOP, &v);
    CHECK(v.num == 16, "bound pad %d", (int)v.num);

    /* resolution change → automatic update */
    heard = 0;
    set_res(disp, 800, 480);
    m = lv_appdimens_get_metrics();
    CHECK(m->in.width == 800 && m->in.height == 480, "after resize %g x %g", (double)m->in.width, (double)m->in.height);
    CHECK(heard >= 1, "listener not called");
    CHECK(lv_ad_sdp(16) == 26, "sdp@800x480 %d", (int)lv_ad_sdp(16)); /* 16*480/300 = 25.6 */
    lv_style_get_prop(&st, LV_STYLE_PAD_TOP, &v);
    CHECK(v.num == 32, "rebound pad %d", (int)v.num);
    lv_style_get_prop(&st, LV_STYLE_WIDTH, &v);
    CHECK(v.num == 160, "rebound width (auto at sw 480 = 1.6) %d", (int)v.num);
    CHECK(lv_obj_get_style_pad_top(box, 0) == 32, "object sees new pad %d", (int)lv_obj_get_style_pad_top(box, 0));
    CHECK(lv_obj_get_style_pad_left(box, 0) == 32, "object sees new pad_left %d", (int)lv_obj_get_style_pad_left(box, 0));

    /* rotation: hor/ver swap → landscape → portrait; Ph/Lw inverters follow */
    set_rot90(disp);
    m = lv_appdimens_get_metrics();
    CHECK(m->in.width == 480 && m->in.height == 800 && m->is_portrait, "rotation %g x %g", (double)m->in.width, (double)m->in.height);
    CHECK(lv_ad_sdpPh(10) == 27, "sdpPh in portrait uses height %d", (int)lv_ad_sdpPh(10));

    /* font scale + font pick */
    lv_appdimens_set_font_scale(1.5f);
    CHECK(lv_ad_ssp(10) == 24, "ssp ×1.5 %d", (int)lv_ad_ssp(10)); /* 10*1.6*1.5 */
    const lv_font_t *fonts[] = {&lv_font_montserrat_28, &lv_font_montserrat_12, &lv_font_montserrat_14, &lv_font_montserrat_20};
    CHECK(lv_ad_font_pick(fonts, 4, 22) == &lv_font_montserrat_20, "font pick 22");
    CHECK(lv_ad_font_pick(fonts, 4, 5) == &lv_font_montserrat_12, "font pick smallest");
    CHECK(lv_ad_font_for(fonts, 4, AD_SCALED, 0, 18) == &lv_font_montserrat_28, "font for 18ssp (=43px)");

    /* DPI mode: density = dpi/160 */
    lv_appdimens_set_font_scale(1.0f);
    lv_appdimens_set_mode(LV_APPDIMENS_MODE_DPI);
    m = lv_appdimens_get_metrics();
    double dens = m->in.density;
    CHECK(dens > 0.5 && dens < 1.0, "density from LV_DPI_DEF=130 → 0.8125, got %g", dens);
    lv_appdimens_set_mode(LV_APPDIMENS_MODE_PIXEL);
    /* manual source + q8 */
    lv_appdimens_set_resolution(1024, 600, 160);
    CHECK(lv_ad_px_q8(AD_SCALED, 0, 12 * 256 + 128) == 25, "q8 %d", (int)lv_ad_px_q8(AD_SCALED, 0, 12 * 256 + 128)); /* 12.5*2 */
    lv_ad_style_unbind(&st);
    lv_appdimens_deinit();
    printf("LVGL %d.%d: %s\n", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, fails ? "FAILED" : "OK");
    return fails != 0;
}
