#include "calendar_tab.h"
#include "ui.h"
#include "../agenda_manager.h"
#include <time.h>
#include <stdio.h>

/* ── Static UI Handles ── */
static lv_obj_t * g_calendar_obj      = NULL;
static lv_obj_t * g_date_label        = NULL;
static lv_obj_t * g_event_desc_label  = NULL;
static lv_obj_t * g_btn_add           = NULL;
static lv_obj_t * g_btn_del           = NULL;
static lv_obj_t * g_overlay_obj       = NULL;

/* ── State variables ── */
static uint16_t g_selected_year  = 2026;
static uint8_t  g_selected_month = 6;
static uint8_t  g_selected_day   = 13;

static lv_calendar_date_t g_highlighted_days[MAX_EVENTS];

/* ── Functions ── */
static void update_calendar_highlights(void)
{
    if(!g_calendar_obj) return;
    int count = agenda_get_highlighted_dates(g_highlighted_days, MAX_EVENTS);
    lv_calendar_set_highlighted_dates(g_calendar_obj, g_highlighted_days, count);
}

static void update_detail_panel(void)
{
    if(!g_date_label || !g_event_desc_label) return;

    char date_str[64];
    static const char * months[] = {
        "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
        "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
    };

    snprintf(date_str, sizeof(date_str), "%d de %s de %04d", 
             g_selected_day, 
             g_selected_month >= 1 && g_selected_month <= 12 ? months[g_selected_month - 1] : "Mes",
             g_selected_year);
    lv_label_set_text(g_date_label, date_str);

    const char * desc = agenda_get_event(g_selected_year, g_selected_month, g_selected_day);
    if(desc) {
        lv_label_set_text(g_event_desc_label, desc);
        lv_obj_set_style_text_color(g_event_desc_label, UI_COLOR_ALERT, 0); /* Amber Alert color */
        if(g_btn_del) {
            lv_obj_remove_state(g_btn_del, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(g_btn_del, lv_color_hex(0xEF4444), 0); /* Red-500 */
        }
    } else {
        lv_label_set_text(g_event_desc_label, "Sin eventos para este día.");
        lv_obj_set_style_text_color(g_event_desc_label, UI_COLOR_TEXT_MUTED, 0);
        if(g_btn_del) {
            lv_obj_add_state(g_btn_del, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(g_btn_del, UI_COLOR_CARD_BORDER, 0);
        }
    }
}

static void calendar_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_current_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        lv_calendar_date_t date;
        if(lv_calendar_get_pressed_date(obj, &date)) {
            g_selected_year = date.year;
            g_selected_month = date.month;
            g_selected_day = date.day;
            update_detail_panel();
        }
    }
}

static void delete_click_cb(lv_event_t * e)
{
    (void)e;
    agenda_delete_event(g_selected_year, g_selected_month, g_selected_day);
    update_calendar_highlights();
    update_detail_panel();
}

static void save_click_cb(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_user_data(e);
    const char * text = lv_textarea_get_text(ta);
    
    agenda_set_event(g_selected_year, g_selected_month, g_selected_day, text);
    update_calendar_highlights();
    update_detail_panel();
    
    if(g_overlay_obj) {
        lv_obj_delete(g_overlay_obj);
        g_overlay_obj = NULL;
    }
}

static void cancel_click_cb(lv_event_t * e)
{
    (void)e;
    if(g_overlay_obj) {
        lv_obj_delete(g_overlay_obj);
        g_overlay_obj = NULL;
    }
}


static void add_click_cb(lv_event_t * e)
{
    (void)e;

    /* Create fullscreen semi-transparent overlay as child of screen */
    g_overlay_obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_overlay_obj, 800, 600);
    lv_obj_set_pos(g_overlay_obj, 0, 0);
    lv_obj_set_style_bg_color(g_overlay_obj, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g_overlay_obj, LV_OPA_60, 0);
    lv_obj_set_style_border_width(g_overlay_obj, 0, 0);
    lv_obj_set_style_radius(g_overlay_obj, 0, 0);
    lv_obj_clear_flag(g_overlay_obj, LV_OBJ_FLAG_SCROLLABLE);

    /* Modal card at top center — leaves room for keyboard at bottom */
    lv_obj_t * modal = ui_create_card(g_overlay_obj, 480, 185);
    lv_obj_set_pos(modal, (800 - 480) / 2, 10);
    lv_obj_set_layout(modal, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(modal, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(modal, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Modal title */
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "Evento — %02d/%02d/%04d",
             g_selected_day, g_selected_month, g_selected_year);
    lv_obj_t * title = lv_label_create(modal);
    lv_label_set_text(title, title_buf);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_TEXT_MAIN, 0);

    /* Text area */
    lv_obj_t * ta = lv_textarea_create(modal);
    lv_obj_set_size(ta, 440, 70);
    lv_textarea_set_one_line(ta, false);
    lv_textarea_set_placeholder_text(ta, "Escribe la descripcion del evento...");
    lv_obj_set_style_text_color(ta, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_bg_color(ta, UI_COLOR_BG, 0);
    lv_obj_set_style_border_color(ta, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_14, 0);

    const char * cur_desc = agenda_get_event(g_selected_year, g_selected_month, g_selected_day);
    if(cur_desc) lv_textarea_set_text(ta, cur_desc);

    /* Buttons row */
    lv_obj_t * btn_box = lv_obj_create(modal);
    lv_obj_set_size(btn_box, 440, 45);
    lv_obj_set_layout(btn_box, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_box, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(btn_box, 0, 0);
    lv_obj_set_style_bg_opa(btn_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_box, 0, 0);

    lv_obj_t * btn_save = lv_button_create(btn_box);
    lv_obj_set_size(btn_save, 150, 35);
    lv_obj_set_style_bg_color(btn_save, UI_COLOR_ACCENT, 0);
    lv_obj_t * lbl_save = lv_label_create(btn_save);
    lv_label_set_text(lbl_save, LV_SYMBOL_OK " Guardar");
    lv_obj_set_style_text_font(lbl_save, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_save, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_save, save_click_cb, LV_EVENT_CLICKED, ta);

    lv_obj_t * btn_cancel = lv_button_create(btn_box);
    lv_obj_set_size(btn_cancel, 150, 35);
    lv_obj_set_style_bg_color(btn_cancel, UI_COLOR_CARD_BORDER, 0);
    lv_obj_t * lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, LV_SYMBOL_CLOSE " Cancelar");
    lv_obj_set_style_text_font(lbl_cancel, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_cancel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_cancel, cancel_click_cb, LV_EVENT_CLICKED, NULL);

    /* Embed keyboard directly inside overlay (child of overlay, not screen)
       so it gets automatically deleted when overlay is deleted.            */
    lv_obj_t * kb = lv_keyboard_create(g_overlay_obj);
    lv_keyboard_set_textarea(kb, ta);
    lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_set_size(kb, 800, 300);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(kb, UI_COLOR_CARD, 0);
    lv_obj_set_style_border_color(kb, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_text_color(kb, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_set_style_radius(kb, 0, 0);
    lv_obj_set_style_border_width(kb, 1, 0);
    /* NOTE: no READY/CANCEL handlers on keyboard — modal lifecycle is
       controlled exclusively by the Save / Cancel buttons above.       */
}

void calendar_tab_init(lv_obj_t * parent)
{
    /* Initialize default selected date to today */
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    g_selected_year  = tm_info->tm_year + 1900;
    g_selected_month = tm_info->tm_mon + 1;
    g_selected_day   = tm_info->tm_mday;

    /* Setup Split Layout: Flex Row */
    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(parent, 15, 0);

    /* Left Side: Calendar Container */
    lv_obj_t * left_container = lv_obj_create(parent);
    lv_obj_set_size(left_container, 320, 360);
    lv_obj_set_style_bg_opa(left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_container, 0, 0);
    lv_obj_set_style_pad_all(left_container, 0, 0);
    lv_obj_clear_flag(left_container, LV_OBJ_FLAG_SCROLLABLE);

    g_calendar_obj = lv_calendar_create(left_container);
    lv_obj_set_size(g_calendar_obj, 320, 310);
    lv_obj_align(g_calendar_obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(g_calendar_obj, calendar_event_cb, LV_EVENT_ALL, NULL);

    lv_calendar_set_today_date(g_calendar_obj, g_selected_year, g_selected_month, g_selected_day);
    lv_calendar_set_showed_date(g_calendar_obj, g_selected_year, g_selected_month);

    /* Add navigation arrows */
    lv_calendar_header_arrow_create(g_calendar_obj);

    /* Right Side: Detail Panel Card */
    lv_obj_t * detail_card = ui_create_card(parent, 270, 360);
    lv_obj_set_layout(detail_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(detail_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(detail_card, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);

    /* Date Label */
    g_date_label = lv_label_create(detail_card);
    lv_obj_set_style_text_font(g_date_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(g_date_label, UI_COLOR_TEXT_MAIN, 0);

    /* Event description box */
    lv_obj_t * desc_box = lv_obj_create(detail_card);
    lv_obj_set_size(desc_box, 246, 190);
    lv_obj_set_style_bg_color(desc_box, UI_COLOR_BG, 0);
    lv_obj_set_style_border_color(desc_box, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_border_width(desc_box, 1, 0);
    lv_obj_set_style_radius(desc_box, 6, 0);
    lv_obj_set_style_pad_all(desc_box, 10, 0);
    lv_obj_clear_flag(desc_box, LV_OBJ_FLAG_SCROLLABLE);

    g_event_desc_label = lv_label_create(desc_box);
    lv_obj_set_width(g_event_desc_label, 226);
    lv_obj_set_style_text_font(g_event_desc_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(g_event_desc_label, "");

    /* Action buttons row */
    lv_obj_t * detail_btn_box = lv_obj_create(detail_card);
    lv_obj_set_size(detail_btn_box, 246, 50);
    lv_obj_set_layout(detail_btn_box, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(detail_btn_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(detail_btn_box, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(detail_btn_box, 0, 0);
    lv_obj_set_style_bg_opa(detail_btn_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(detail_btn_box, 0, 0);

    /* Add Event button */
    g_btn_add = lv_button_create(detail_btn_box);
    lv_obj_set_size(g_btn_add, 115, 38);
    lv_obj_set_style_bg_color(g_btn_add, UI_COLOR_ACCENT, 0);
    lv_obj_t * lbl_add = lv_label_create(g_btn_add);
    lv_label_set_text(lbl_add, "Editar");
    lv_obj_set_style_text_font(lbl_add, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_add, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_btn_add, add_click_cb, LV_EVENT_CLICKED, NULL);

    /* Delete Event button */
    g_btn_del = lv_button_create(detail_btn_box);
    lv_obj_set_size(g_btn_del, 115, 38);
    lv_obj_t * lbl_del = lv_label_create(g_btn_del);
    lv_label_set_text(lbl_del, "Borrar");
    lv_obj_set_style_text_font(lbl_del, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_del, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(g_btn_del, delete_click_cb, LV_EVENT_CLICKED, NULL);

    /* Load initial events from manager and update view */
    update_calendar_highlights();
    update_detail_panel();
}
