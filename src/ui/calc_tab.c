#include "calc_tab.h"
#include "ui.h"

void calc_tab_init(lv_obj_t * parent)
{
    /* Center the content */
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * card = ui_create_card(parent, 450, 250);

    lv_obj_t * label = lv_label_create(card);
    lv_label_set_text(label, "CALCULADORA");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label, UI_COLOR_ACCENT, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t * desc = lv_label_create(card);
    lv_label_set_text(desc, "Aqui se integrara la calculadora matematica.\n\nSe usara el modulo lv_100ask_calc de la libreria lv_lib_100ask o una botonera personalizada integrada de forma nativa.");
    lv_obj_set_style_text_font(desc, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(desc, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(desc, 400);
    lv_obj_align(desc, LV_ALIGN_CENTER, 0, 20);
}
