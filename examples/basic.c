/* AppDimens + LVGL: a card that keeps its proportions on any panel and after rotation. */
#include "lv_appdimens.h"

static lv_style_t card, title;

void appdimens_example(void)
{
    lv_appdimens_init(NULL);                          /* default display, follows resolution/rotation */

    lv_style_init(&card);
    lv_ad_style_bind_pad_all(&card, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 12);
    lv_ad_style_bind(&card, LV_STYLE_RADIUS, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 10);
    lv_ad_style_bind(&card, LV_STYLE_WIDTH, AD_AUTO, AD_SLOT(AD_VAR_S, 0), 240);   /* calm growth on big panels */

    static const lv_font_t * const fonts[] = { &lv_font_montserrat_14, &lv_font_montserrat_20, &lv_font_montserrat_28 };
    lv_style_init(&title);
    lv_style_set_text_font(&title, lv_ad_font_for(fonts, 3, AD_SCALED, AD_SLOT(AD_VAR_S, 0), 16));

    lv_obj_t * box = lv_obj_create(lv_scr_act());
    lv_obj_add_style(box, &card, 0);
    lv_obj_set_height(box, LV_SIZE_CONTENT);
    lv_obj_center(box);

    lv_obj_t * label = lv_label_create(box);
    lv_obj_add_style(label, &title, 0);
    lv_label_set_text(label, "AppDimens");

    lv_obj_t * btn = lv_btn_create(box);                 /* v9: lv_button_create */
    lv_obj_set_size(btn, lv_ad_sdp(120), lv_ad_asdp(40));
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, lv_ad_sdp(4));
}
