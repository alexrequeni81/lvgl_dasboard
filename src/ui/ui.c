#include "ui.h"
#include "dashboard_tab.h"
#include "calendar_tab.h"
#include "calc_tab.h"
#include "conv_tab.h"
#include "../wifi_manager.h"
#include "../net_manager.h"

/* ── Sidebar status bar widgets ── */
static lv_obj_t * sidebar_net_label = NULL;
static lv_obj_t * sidebar_net_icon  = NULL;

/* ── Settings button callback ── */
static void sidebar_settings_cb(lv_event_t * e)
{
    (void)e;
    wifi_show_config_modal(lv_screen_active());
}

static void kb_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_obj_t * kb = lv_event_get_target(e);
        lv_obj_delete(kb);
    }
}

void ui_attach_keyboard(lv_obj_t * ta, lv_keyboard_mode_t mode)
{
    /* Delete any existing keyboard first */
    lv_obj_t * scr = lv_screen_active();
    uint32_t c;
    for(c = 0; c < lv_obj_get_child_count(scr); c++) {
        lv_obj_t * child = lv_obj_get_child(scr, c);
        if(lv_obj_check_type(child, &lv_keyboard_class)) {
            lv_obj_delete(child);
            break;
        }
    }

    lv_obj_t * kb = lv_keyboard_create(scr);
    lv_keyboard_set_textarea(kb, ta);
    lv_keyboard_set_mode(kb, mode);
    lv_obj_set_style_bg_color(kb, UI_COLOR_CARD, 0);
    lv_obj_set_style_border_color(kb, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_text_color(kb, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_radius(kb, 0, 0);
    lv_obj_set_style_border_width(kb, 1, 0);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_CANCEL, NULL);
}

lv_obj_t * ui_create_card(lv_obj_t * parent, int32_t w, int32_t h)
{
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 12, 0);

    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    return card;
}

static void ui_update_sidebar_status(void)
{
    if(!sidebar_net_label) return;
    lv_label_set_text(sidebar_net_label, wifi_status_str());
}

static void sidebar_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    ui_update_sidebar_status();
}

void ui_init(void)
{
    lv_obj_set_style_bg_color(lv_screen_active(), UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0);

    /* ── Tabview ── */
    lv_obj_t * tv = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(tv, LV_DIR_LEFT);
    lv_tabview_set_tab_bar_size(tv, 160);

    lv_obj_t * tab_bar = lv_tabview_get_tab_bar(tv);
    lv_obj_set_style_bg_color(tab_bar, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(tab_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tab_bar, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(tab_bar, 1, 0);
    lv_obj_set_style_border_side(tab_bar, LV_BORDER_SIDE_RIGHT, 0);

    lv_obj_set_style_text_color(tab_bar, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(tab_bar, &lv_font_montserrat_14, 0);

    lv_obj_set_style_text_color(tab_bar, UI_COLOR_ACCENT, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tab_bar, UI_COLOR_CARD, LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(tab_bar, LV_OPA_COVER, LV_STATE_CHECKED);

    lv_obj_t * content = lv_tabview_get_content(tv);
    lv_obj_set_style_bg_color(content, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);

    lv_obj_t * tab_dash = lv_tabview_add_tab(tv, "Dashboard");
    lv_obj_t * tab_cal  = lv_tabview_add_tab(tv, "Calendario");
    lv_obj_t * tab_calc = lv_tabview_add_tab(tv, "Calculadora");
    lv_obj_t * tab_conv = lv_tabview_add_tab(tv, "Conversores");

    /* ── Sidebar bottom status bar (overlays the tab bar area) ── */
    lv_obj_t * sb = lv_obj_create(lv_screen_active());
    lv_obj_set_size(sb, 160, 44);
    lv_obj_set_pos(sb, 0, 556); /* 600 - 44 */
    lv_obj_set_style_bg_color(sb, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(sb, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sb, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(sb, 1, 0);
    lv_obj_set_style_border_side(sb, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_clear_flag(sb, LV_OBJ_FLAG_SCROLLABLE);

    /* Network icon + status */
    sidebar_net_icon = lv_label_create(sb);
    lv_label_set_text(sidebar_net_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(sidebar_net_icon, UI_COLOR_ACCENT, 0);
    lv_obj_align(sidebar_net_icon, LV_ALIGN_LEFT_MID, 8, 0);

    sidebar_net_label = lv_label_create(sb);
    lv_label_set_text(sidebar_net_label, wifi_status_str());
    lv_obj_set_style_text_font(sidebar_net_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sidebar_net_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align_to(sidebar_net_label, sidebar_net_icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    /* Settings gear button */
    lv_obj_t * gear_btn = lv_button_create(sb);
    lv_obj_set_size(gear_btn, 32, 28);
    lv_obj_set_style_bg_color(gear_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_radius(gear_btn, 4, 0);
    lv_obj_align(gear_btn, LV_ALIGN_RIGHT_MID, -6, 0);
    lv_obj_t * gear_icon = lv_label_create(gear_btn);
    lv_label_set_text(gear_icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(gear_icon, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(gear_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(gear_btn, sidebar_settings_cb, LV_EVENT_CLICKED, NULL);

    /* Update sidebar status every 2 seconds */
    lv_timer_create(sidebar_timer_cb, 2000, NULL);

    /* ── Init tabs ── */
    dashboard_tab_init(tab_dash);
    calendar_tab_init(tab_cal);
    calc_tab_init(tab_calc);
    conv_tab_init(tab_conv);
}
