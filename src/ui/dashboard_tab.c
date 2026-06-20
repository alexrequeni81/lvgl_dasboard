#include "dashboard_tab.h"
#include "ui.h"
#include "../time_manager.h"
#include "../weather.h"
#include "../radio_manager.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
  #define SETTINGS_PATH "settings.conf"
#else
  #define SETTINGS_PATH "/home/tc/settings.conf"
#endif

/* ── Static UI handles ── */
static lv_obj_t * time_label  = NULL;
static lv_obj_t * date_label  = NULL;

/* Weather UI */
static lv_obj_t * weather_temp_label   = NULL;
static lv_obj_t * weather_desc_label   = NULL;
static lv_obj_t * weather_extra_label  = NULL;
static lv_obj_t * weather_status_label = NULL;

/* Radio UI */
static lv_obj_t * radio_now_label    = NULL;
static lv_obj_t * radio_grid         = NULL;
static lv_obj_t * radio_btns[RADIO_MAX_STATIONS];
static lv_obj_t * radio_vol_label    = NULL;
static lv_obj_t * radio_vol_down_btn = NULL;
static lv_obj_t * radio_vol_up_btn   = NULL;
static lv_obj_t * radio_stop_btn     = NULL;

/* Per-slot alarm UI */
static lv_obj_t * alarm_labels[ALARM_SLOTS];
static lv_obj_t * alarm_switches[ALARM_SLOTS];

/* Ringing popup */
static lv_obj_t * ring_popup = NULL;

/* Modal adjust */
static lv_obj_t * modal_bg     = NULL;
static lv_obj_t * roller_hour  = NULL;
static lv_obj_t * roller_min   = NULL;
static int        editing_slot = 0;

/* ── Helpers ── */
static void update_alarm_slot_label(int i)
{
    if(alarm_labels[i]) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", g_alarms[i].hour, g_alarms[i].minute);
        lv_label_set_text(alarm_labels[i], buf);
    }
}

/* ── Ringing popup ── */
static void dismiss_ring_popup(void)
{
    if(ring_popup) {
        lv_obj_delete(ring_popup);
        ring_popup = NULL;
    }
}

static void btn_stop_cb(lv_event_t * e)
{
    (void)e;
    time_manager_stop_sound();
    dismiss_ring_popup();
}

static void btn_snooze_cb(lv_event_t * e)
{
    (void)e;
    int slot = g_ringing_slot;
    time_manager_snooze(slot);
    dismiss_ring_popup();
    /* Update UI for snoozed alarm */
    if(slot >= 0 && slot < ALARM_SLOTS)
        update_alarm_slot_label(slot);
}

static void show_ring_popup(int slot)
{
    if(ring_popup) return; /* Already showing */

    ring_popup = lv_obj_create(lv_screen_active());
    lv_obj_set_size(ring_popup, 360, 160);
    lv_obj_set_style_bg_color(ring_popup, lv_color_hex(0x7F1D1D), 0); /* red-900 */
    lv_obj_set_style_bg_opa(ring_popup, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ring_popup, lv_color_hex(0xEF4444), 0); /* red-500 */
    lv_obj_set_style_border_width(ring_popup, 2, 0);
    lv_obj_set_style_radius(ring_popup, 12, 0);
    lv_obj_set_style_pad_all(ring_popup, 15, 0);
    lv_obj_align(ring_popup, LV_ALIGN_CENTER, 80, 0); /* offset right of sidebar */
    lv_obj_clear_flag(ring_popup, LV_OBJ_FLAG_SCROLLABLE);

    /* Title */
    lv_obj_t * title = lv_label_create(ring_popup);
    char buf[48];
    snprintf(buf, sizeof(buf), "ALARMA %d  -  %02d:%02d",
             slot + 1, g_alarms[slot].hour, g_alarms[slot].minute);
    lv_label_set_text(title, buf);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFCA5A5), 0); /* red-300 */
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Subtitle */
    lv_obj_t * sub = lv_label_create(ring_popup);
    lv_label_set_text(sub, "Sonando... (30s)");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sub, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align_to(sub, title, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    /* Buttons container */
    lv_obj_t * btns = lv_obj_create(ring_popup);
    lv_obj_set_size(btns, 310, 55);
    lv_obj_set_style_bg_opa(btns, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btns, 0, 0);
    lv_obj_align(btns, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_layout(btns, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btns, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btns, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* STOP button */
    lv_obj_t * btn_stop = lv_button_create(btns);
    lv_obj_set_size(btn_stop, 120, 40);
    lv_obj_set_style_bg_color(btn_stop, lv_color_hex(0xDC2626), 0); /* red-600 */
    lv_obj_set_style_radius(btn_stop, 6, 0);
    lv_obj_t * lbl_stop = lv_label_create(btn_stop);
    lv_label_set_text(lbl_stop, "PARAR");
    lv_obj_set_style_text_font(lbl_stop, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_stop, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_stop, btn_stop_cb, LV_EVENT_CLICKED, NULL);

    /* SNOOZE button */
    lv_obj_t * btn_snz = lv_button_create(btns);
    lv_obj_set_size(btn_snz, 140, 40);
    lv_obj_set_style_bg_color(btn_snz, UI_COLOR_ALERT, 0);
    lv_obj_set_style_radius(btn_snz, 6, 0);
    lv_obj_t * lbl_snz = lv_label_create(btn_snz);
    lv_label_set_text(lbl_snz, "SNOOZE +5m");
    lv_obj_set_style_text_font(lbl_snz, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_snz, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_snz, btn_snooze_cb, LV_EVENT_CLICKED, NULL);
}

/* ── Weather display ── */
void update_weather_display(void)
{
    if(!weather_temp_label) return;

    char buf[128];
    if(g_weather.valid) {
        lv_obj_set_style_text_color(weather_temp_label, UI_COLOR_ALERT, 0);
        lv_obj_set_style_text_color(weather_desc_label, UI_COLOR_TEXT_MAIN, 0);
        snprintf(buf, sizeof(buf), "%.1f C", g_weather.temperature);
        lv_label_set_text(weather_temp_label, buf);
        lv_label_set_text(weather_desc_label, g_weather.description);

        snprintf(buf, sizeof(buf), "Humedad: %d%%\nViento: %.0f km/h",
                 g_weather.humidity, g_weather.wind_speed);
        lv_label_set_text(weather_extra_label, buf);

        time_t age = time(NULL) - g_weather.fetch_time;
        snprintf(buf, sizeof(buf), "Actualizado: %ld min ago\n%s",
                 age / 60, g_weather.city_name[0] ? g_weather.city_name : g_weather_zip);
        lv_label_set_text(weather_status_label, buf);
    } else {
        lv_obj_set_style_text_color(weather_temp_label, UI_COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_color(weather_desc_label, UI_COLOR_ALERT, 0);
        if(g_weather_last_error[0]) {
            lv_label_set_text(weather_desc_label, g_weather_last_error);
            lv_label_set_text(weather_status_label, g_weather_last_ok ? "" : "Error");
        } else {
            lv_label_set_text(weather_desc_label, "Sin datos de clima");
            lv_label_set_text(weather_status_label, "Conectando...");
        }
        lv_label_set_text(weather_temp_label, "--.- C");
        lv_label_set_text(weather_extra_label, "");
    }
}

/* ── Radio station button callback ── */
static void radio_btn_cb(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    radio_toggle(idx);
}

/* ── Radio volume button callbacks ── */
static void radio_vol_down_cb(lv_event_t * e)
{
    (void)e;
    int vol = g_radio_volume - 10;
    if(vol < 0) vol = 0;
    radio_set_volume(vol);
}

static void radio_vol_up_cb(lv_event_t * e)
{
    (void)e;
    int vol = g_radio_volume + 10;
    if(vol > 100) vol = 100;
    radio_set_volume(vol);
}

/* ── Radio stop button ── */
static void radio_stop_cb(lv_event_t * e)
{
    (void)e;
    radio_stop();
}

/* ── Refresh radio UI ── */
static void update_radio_display(void)
{
    if(!radio_now_label) return;

    if(g_radio_playing && g_radio_current >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Ahora: %s",
                 g_radio_presets[g_radio_current].name);
        lv_label_set_text(radio_now_label, buf);
        lv_obj_set_style_text_color(radio_now_label, UI_COLOR_ACCENT, 0);
    } else {
        lv_label_set_text(radio_now_label, "Detenido");
        lv_obj_set_style_text_color(radio_now_label, UI_COLOR_TEXT_MUTED, 0);
    }

    /* Highlight active station button */
    for(int i = 0; i < g_radio_preset_count && i < RADIO_MAX_STATIONS; i++) {
        if(!radio_btns[i]) continue;
        if(i == g_radio_current && g_radio_playing)
            lv_obj_set_style_bg_color(radio_btns[i], UI_COLOR_ACCENT, 0);
        else
            lv_obj_set_style_bg_color(radio_btns[i], UI_COLOR_CARD_BORDER, 0);
    }

    if(radio_vol_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "Vol: %d%%", g_radio_volume);
        lv_label_set_text(radio_vol_label, buf);
    }
}

/* ── Weather update timer (every 15 min) ── */
static void weather_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    weather_update();
    update_weather_display();
}

/* ── Manual refresh button callback ── */
static void weather_refresh_cb(lv_event_t * e)
{
    (void)e;
    lv_label_set_text(weather_status_label, "Actualizando...");
    weather_update();
    update_weather_display();
}

/* ── Update clock & date labels ── */
static void update_clock_display(void)
{
    time_t rawtime;
    struct tm * ti;
    time(&rawtime);
    ti = localtime(&rawtime);

    if(time_label) {
        char buf[16];
        strftime(buf, sizeof(buf), "%H:%M:%S", ti);
        lv_label_set_text(time_label, buf);
    }

    if(date_label) {
        const char * days[]   = {"Domingo","Lunes","Martes","Miercoles","Jueves","Viernes","Sabado"};
        const char * months[] = {"Enero","Febrero","Marzo","Abril","Mayo","Junio",
                                 "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre"};
        char buf[80];
        snprintf(buf, sizeof(buf), "%s, %02d de %s de %d",
                 days[ti->tm_wday], ti->tm_mday, months[ti->tm_mon], ti->tm_year + 1900);
        lv_label_set_text(date_label, buf);
    }

    /* Check alarms */
    int slot = time_manager_check_alarms(ti->tm_hour, ti->tm_min);
    if(slot >= 0) {
        g_ringing_slot = slot;
        time_manager_start_sound();
        show_ring_popup(slot);
    }

    /* Auto-dismiss popup when sound stops naturally */
    if(ring_popup && !g_alarm_ringing) {
        dismiss_ring_popup();
        g_ringing_slot = -1;
    }

    /* Refresh weather display (keeps "X min ago" current) */
    update_weather_display();
}

/* ── Clock timer (1 second) ── */
static void clock_timer_cb(lv_timer_t * timer)
{
    (void)timer;
    update_clock_display();
    update_radio_display();
}

/* ── Switch callback ── */
static void alarm_switch_cb(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t * sw = lv_event_get_target(e);
    g_alarms[idx].enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    time_manager_save();
}

/* ── Modal: save / cancel ── */
static void btn_save_alarm_cb(lv_event_t * e)
{
    (void)e;
    g_alarms[editing_slot].hour   = lv_roller_get_selected(roller_hour);
    g_alarms[editing_slot].minute = lv_roller_get_selected(roller_min);
    time_manager_save();
    update_alarm_slot_label(editing_slot);
    if(modal_bg) { lv_obj_delete(modal_bg); modal_bg = NULL; }
}

static void btn_cancel_alarm_cb(lv_event_t * e)
{
    (void)e;
    if(modal_bg) { lv_obj_delete(modal_bg); modal_bg = NULL; }
}

/* ── Modal: open adjust dialog ── */
static void btn_adjust_cb(lv_event_t * e)
{
    if(modal_bg) return;
    editing_slot = (int)(intptr_t)lv_event_get_user_data(e);

    /* Dim overlay */
    modal_bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(modal_bg, 800, 600);
    lv_obj_set_style_bg_color(modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modal_bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(modal_bg, 0, 0);
    lv_obj_set_style_radius(modal_bg, 0, 0);
    lv_obj_clear_flag(modal_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(modal_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(modal_bg, LV_ALIGN_CENTER, 0, 0);

    /* Dialog card */
    lv_obj_t * dlg = ui_create_card(modal_bg, 340, 250);
    lv_obj_align(dlg, LV_ALIGN_CENTER, 0, 0);

    /* Title */
    lv_obj_t * title = lv_label_create(dlg);
    char buf[32];
    snprintf(buf, sizeof(buf), "ALARMA %d", editing_slot + 1);
    lv_label_set_text(title, buf);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    /* Rollers container */
    lv_obj_t * rc = lv_obj_create(dlg);
    lv_obj_set_size(rc, 280, 110);
    lv_obj_set_style_bg_opa(rc, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rc, 0, 0);
    lv_obj_align(rc, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_layout(rc, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rc, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(rc, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(rc, 20, 0);

    /* Hour roller */
    roller_hour = lv_roller_create(rc);
    lv_obj_set_size(roller_hour, 70, 95);
    char opts_h[128] = "";
    for(int i = 0; i < 24; i++) { char b[8]; snprintf(b, sizeof(b), "%02d\n", i); strcat(opts_h, b); }
    opts_h[strlen(opts_h) - 1] = '\0';
    lv_roller_set_options(roller_hour, opts_h, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller_hour, g_alarms[editing_slot].hour, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(roller_hour, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_bg_color(roller_hour, UI_COLOR_ACCENT, LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller_hour, &lv_font_montserrat_14, 0);

    /* Separator */
    lv_obj_t * sep = lv_label_create(rc);
    lv_label_set_text(sep, ":");
    lv_obj_set_style_text_font(sep, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(sep, UI_COLOR_TEXT_MAIN, 0);

    /* Minute roller */
    roller_min = lv_roller_create(rc);
    lv_obj_set_size(roller_min, 70, 95);
    char opts_m[300] = "";
    for(int i = 0; i < 60; i++) { char b[8]; snprintf(b, sizeof(b), "%02d\n", i); strcat(opts_m, b); }
    opts_m[strlen(opts_m) - 1] = '\0';
    lv_roller_set_options(roller_min, opts_m, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(roller_min, g_alarms[editing_slot].minute, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(roller_min, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_bg_color(roller_min, UI_COLOR_ACCENT, LV_PART_SELECTED);
    lv_obj_set_style_text_font(roller_min, &lv_font_montserrat_14, 0);

    /* Buttons */
    lv_obj_t * bb = lv_obj_create(dlg);
    lv_obj_set_size(bb, 300, 45);
    lv_obj_set_style_bg_opa(bb, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bb, 0, 0);
    lv_obj_align(bb, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_layout(bb, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bb, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bb, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * bs = lv_button_create(bb);
    lv_obj_set_size(bs, 100, 35);
    lv_obj_set_style_bg_color(bs, UI_COLOR_ACCENT, 0);
    lv_obj_t * ls = lv_label_create(bs);
    lv_label_set_text(ls, "Aceptar");
    lv_obj_set_style_text_font(ls, &lv_font_montserrat_14, 0);
    lv_obj_align(ls, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bs, btn_save_alarm_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * bc = lv_button_create(bb);
    lv_obj_set_size(bc, 100, 35);
    lv_obj_set_style_bg_color(bc, UI_COLOR_CARD_BORDER, 0);
    lv_obj_t * lc = lv_label_create(bc);
    lv_label_set_text(lc, "Cancelar");
    lv_obj_set_style_text_font(lc, &lv_font_montserrat_14, 0);
    lv_obj_align(lc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bc, btn_cancel_alarm_cb, LV_EVENT_CLICKED, NULL);
}

/* ── Clock style globals ── */
static int    g_clock_font_size    = 24;
static int    g_clock_color_hex    = 0xFFFFFF;
static int    g_clock_letter_space = 0;
static int    g_clock_opa          = 255;

static const lv_font_t * font_for_size(int size)
{
    switch(size) {
#if LV_FONT_MONTSERRAT_14
        case 14:  return &lv_font_montserrat_14;
#endif
#if LV_FONT_MONTSERRAT_16
        case 16:  return &lv_font_montserrat_16;
#endif
#if LV_FONT_MONTSERRAT_18
        case 18:  return &lv_font_montserrat_18;
#endif
#if LV_FONT_MONTSERRAT_20
        case 20:  return &lv_font_montserrat_20;
#endif
#if LV_FONT_MONTSERRAT_22
        case 22:  return &lv_font_montserrat_22;
#endif
#if LV_FONT_MONTSERRAT_24
        case 24:  return &lv_font_montserrat_24;
#endif
#if LV_FONT_MONTSERRAT_26
        case 26:  return &lv_font_montserrat_26;
#endif
#if LV_FONT_MONTSERRAT_28
        case 28:  return &lv_font_montserrat_28;
#endif
#if LV_FONT_MONTSERRAT_30
        case 30:  return &lv_font_montserrat_30;
#endif
#if LV_FONT_MONTSERRAT_32
        case 32:  return &lv_font_montserrat_32;
#endif
#if LV_FONT_MONTSERRAT_34
        case 34:  return &lv_font_montserrat_34;
#endif
#if LV_FONT_MONTSERRAT_36
        case 36:  return &lv_font_montserrat_36;
#endif
#if LV_FONT_MONTSERRAT_38
        case 38:  return &lv_font_montserrat_38;
#endif
#if LV_FONT_MONTSERRAT_40
        case 40:  return &lv_font_montserrat_40;
#endif
#if LV_FONT_MONTSERRAT_42
        case 42:  return &lv_font_montserrat_42;
#endif
#if LV_FONT_MONTSERRAT_44
        case 44:  return &lv_font_montserrat_44;
#endif
#if LV_FONT_MONTSERRAT_46
        case 46:  return &lv_font_montserrat_46;
#endif
#if LV_FONT_MONTSERRAT_48
        case 48:  return &lv_font_montserrat_48;
#endif
        default:
#if LV_FONT_MONTSERRAT_24
            return &lv_font_montserrat_24;
#elif LV_FONT_MONTSERRAT_14
            return &lv_font_montserrat_14;
#else
            return NULL;
#endif
    }
}

static void apply_clock_style(void)
{
    if(!time_label) return;
    int fs = g_clock_font_size;
    if(fs < 14) fs = 14; if(fs > 48) fs = 48;
    if(fs % 2) fs++;
    lv_obj_set_style_text_font(time_label, font_for_size(fs), 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(g_clock_color_hex), 0);
    lv_obj_set_style_text_letter_space(time_label, g_clock_letter_space, 0);
    lv_obj_set_style_text_opa(time_label, g_clock_opa, 0);
}

static void clock_load_settings(void)
{
    FILE * f = fopen(SETTINGS_PATH, "r");
    if(!f) return;
    char line[128];
    while(fgets(line, sizeof(line), f)) {
        int val;
        if(sscanf(line, "clock_font_size=%d", &val) == 1) {
            g_clock_font_size = val;
            if(g_clock_font_size < 14) g_clock_font_size = 14;
            if(g_clock_font_size > 48) g_clock_font_size = 48;
            if(g_clock_font_size % 2) g_clock_font_size++;
        }
        if(sscanf(line, "clock_color_hex=%x", &val) == 1) g_clock_color_hex = val;
        if(sscanf(line, "clock_letter_space=%d", &val) == 1) g_clock_letter_space = val;
        if(sscanf(line, "clock_opa=%d", &val) == 1) g_clock_opa = val;
    }
    fclose(f);
}

static void clock_save_settings(void)
{
    FILE * f = fopen(SETTINGS_PATH, "r+");
    if(!f) f = fopen(SETTINGS_PATH, "a");
    if(!f) return;
    fseek(f, 0, SEEK_END); long flen = ftell(f); rewind(f);
    char * buf = NULL;
    if(flen > 0) { buf = (char *)malloc((size_t)flen + 1); size_t r = fread(buf, 1, (size_t)flen, f); buf[r] = '\0'; }
    fclose(f);

    f = fopen(SETTINGS_PATH, "w");
    if(!f) { free(buf); return; }

    int fs_found = 0, ch_found = 0, ls_found = 0, op_found = 0;
    if(buf) {
        char line[256];
        char * pos = buf;
        while(pos && *pos) {
            char * nl = strchr(pos, '\n');
            if(nl) { size_t len = (size_t)(nl - pos); if(len > 255) len = 255; strncpy(line, pos, len); line[len] = '\0'; pos = nl + 1; }
            else { strncpy(line, pos, 255); line[255] = '\0'; pos = NULL; }

            if(strncmp(line, "clock_font_size=", 16) == 0) {
                fprintf(f, "clock_font_size=%d\n", g_clock_font_size); fs_found = 1;
            } else if(strncmp(line, "clock_color_hex=", 16) == 0) {
                fprintf(f, "clock_color_hex=%06X\n", g_clock_color_hex); ch_found = 1;
            } else if(strncmp(line, "clock_letter_space=", 19) == 0) {
                fprintf(f, "clock_letter_space=%d\n", g_clock_letter_space); ls_found = 1;
            } else if(strncmp(line, "clock_opa=", 10) == 0) {
                fprintf(f, "clock_opa=%d\n", g_clock_opa); op_found = 1;
            } else {
                fprintf(f, "%s\n", line);
            }
        }
    }
    if(!fs_found) fprintf(f, "clock_font_size=%d\n", g_clock_font_size);
    if(!ch_found) fprintf(f, "clock_color_hex=%06X\n", g_clock_color_hex);
    if(!ls_found) fprintf(f, "clock_letter_space=%d\n", g_clock_letter_space);
    if(!op_found) fprintf(f, "clock_opa=%d\n", g_clock_opa);

    free(buf);
    fclose(f);
    printf("[clock] settings saved\n");
}

/* ── Clock edit modal ── */
static lv_obj_t * clock_modal_bg = NULL;
static lv_obj_t * clock_fs_slider = NULL;
static lv_obj_t * clock_ls_slider = NULL;
static lv_obj_t * clock_opa_slider = NULL;

static const uint32_t preset_colors_hex[] = {
    0xFFFFFF, /* white */
    0x06B6D4, /* cyan */
    0x22C55E, /* green */
    0xEF4444, /* red */
    0xEAB308, /* yellow */
    0xF97316, /* orange */
    0xA855F7, /* purple */
    0xEC4899, /* pink */
};
#define NUM_COLORS (sizeof(preset_colors_hex) / sizeof(preset_colors_hex[0]))

static void clock_color_btn_cb(lv_event_t * e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if(idx < 0 || idx >= (int)NUM_COLORS) return;
    g_clock_color_hex = (int)preset_colors_hex[idx];
}

static void clock_fs_cb(lv_event_t * e)
{
    g_clock_font_size = lv_slider_get_value(clock_fs_slider);
    apply_clock_style();
    printf("[clock] font_size=%d\n", g_clock_font_size);
}

static void clock_ls_cb(lv_event_t * e)
{
    g_clock_letter_space = lv_slider_get_value(clock_ls_slider);
    apply_clock_style();
    printf("[clock] letter_space=%d\n", g_clock_letter_space);
}

static void clock_opa_cb(lv_event_t * e)
{
    g_clock_opa = lv_slider_get_value(clock_opa_slider);
    apply_clock_style();
    printf("[clock] opa=%d\n", g_clock_opa);
}

static void clock_edit_close(void)
{
    if(clock_modal_bg) { lv_obj_delete(clock_modal_bg); clock_modal_bg = NULL; }
}

static void clock_edit_save_cb(lv_event_t * e)
{
    (void)e;
    if(g_clock_font_size < 14) g_clock_font_size = 14;
    if(g_clock_font_size > 48) g_clock_font_size = 48;
    if(g_clock_font_size % 2) g_clock_font_size++;
    apply_clock_style();
    clock_save_settings();
    clock_edit_close();
}

static void clock_edit_cancel_cb(lv_event_t * e)
{
    (void)e;
    clock_load_settings();
    apply_clock_style();
    clock_edit_close();
}

static void clock_click_cb(lv_event_t * e)
{
    (void)e;
    printf("[clock] click\n");
    if(clock_modal_bg) return;

    clock_modal_bg = lv_obj_create(lv_screen_active());
    lv_obj_set_size(clock_modal_bg, 800, 600);
    lv_obj_set_style_bg_color(clock_modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(clock_modal_bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(clock_modal_bg, 0, 0);
    lv_obj_set_style_radius(clock_modal_bg, 0, 0);
    lv_obj_clear_flag(clock_modal_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(clock_modal_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(clock_modal_bg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * dlg = ui_create_card(clock_modal_bg, 380, 370);
    lv_obj_align(dlg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * title = lv_label_create(dlg);
    lv_label_set_text(title, "ESTILO RELOJ");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    int lbl_x = 12;
    int ctrl_x = 120;
    int row_h = 38;
    int slider_w = 200;

    /* Font size */
    int y = 35;
    lv_obj_t * fs_lbl = lv_label_create(dlg);
    lv_label_set_text(fs_lbl, "Altura:");
    lv_obj_set_style_text_font(fs_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(fs_lbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(fs_lbl, LV_ALIGN_TOP_LEFT, lbl_x, y);

    clock_fs_slider = lv_slider_create(dlg);
    lv_obj_set_size(clock_fs_slider, slider_w, 12);
    lv_slider_set_range(clock_fs_slider, 14, 48);
    lv_slider_set_value(clock_fs_slider, g_clock_font_size, LV_ANIM_OFF);
    lv_obj_align(clock_fs_slider, LV_ALIGN_TOP_LEFT, ctrl_x, y + 2);
    lv_obj_set_style_bg_color(clock_fs_slider, UI_COLOR_CARD_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(clock_fs_slider, UI_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(clock_fs_slider, UI_COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(clock_fs_slider, clock_fs_cb, LV_EVENT_VALUE_CHANGED, NULL);

    char fs_val[8]; snprintf(fs_val, sizeof(fs_val), "%d", g_clock_font_size);
    lv_obj_t * fs_val_lbl = lv_label_create(dlg);
    lv_label_set_text(fs_val_lbl, fs_val);
    lv_obj_set_style_text_font(fs_val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(fs_val_lbl, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align_to(fs_val_lbl, clock_fs_slider, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    /* Letter spacing */
    y += row_h;
    lv_obj_t * ls_lbl = lv_label_create(dlg);
    lv_label_set_text(ls_lbl, "Espaciado:");
    lv_obj_set_style_text_font(ls_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ls_lbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(ls_lbl, LV_ALIGN_TOP_LEFT, lbl_x, y);

    clock_ls_slider = lv_slider_create(dlg);
    lv_obj_set_size(clock_ls_slider, slider_w, 12);
    lv_slider_set_range(clock_ls_slider, -2, 12);
    lv_slider_set_value(clock_ls_slider, g_clock_letter_space, LV_ANIM_OFF);
    lv_obj_align(clock_ls_slider, LV_ALIGN_TOP_LEFT, ctrl_x, y + 2);
    lv_obj_set_style_bg_color(clock_ls_slider, UI_COLOR_CARD_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(clock_ls_slider, UI_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(clock_ls_slider, UI_COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(clock_ls_slider, clock_ls_cb, LV_EVENT_VALUE_CHANGED, NULL);

    char ls_val[8]; snprintf(ls_val, sizeof(ls_val), "%d", g_clock_letter_space);
    lv_obj_t * ls_val_lbl = lv_label_create(dlg);
    lv_label_set_text(ls_val_lbl, ls_val);
    lv_obj_set_style_text_font(ls_val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ls_val_lbl, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align_to(ls_val_lbl, clock_ls_slider, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    /* Opacity */
    y += row_h;
    lv_obj_t * op_lbl = lv_label_create(dlg);
    lv_label_set_text(op_lbl, "Opacidad:");
    lv_obj_set_style_text_font(op_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(op_lbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(op_lbl, LV_ALIGN_TOP_LEFT, lbl_x, y);

    clock_opa_slider = lv_slider_create(dlg);
    lv_obj_set_size(clock_opa_slider, slider_w, 12);
    lv_slider_set_range(clock_opa_slider, 32, 255);
    lv_slider_set_value(clock_opa_slider, g_clock_opa, LV_ANIM_OFF);
    lv_obj_align(clock_opa_slider, LV_ALIGN_TOP_LEFT, ctrl_x, y + 2);
    lv_obj_set_style_bg_color(clock_opa_slider, UI_COLOR_CARD_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(clock_opa_slider, UI_COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(clock_opa_slider, UI_COLOR_ACCENT, LV_PART_KNOB);
    lv_obj_add_event_cb(clock_opa_slider, clock_opa_cb, LV_EVENT_VALUE_CHANGED, NULL);

    char op_val[8]; snprintf(op_val, sizeof(op_val), "%d", g_clock_opa);
    lv_obj_t * op_val_lbl = lv_label_create(dlg);
    lv_label_set_text(op_val_lbl, op_val);
    lv_obj_set_style_text_font(op_val_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(op_val_lbl, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align_to(op_val_lbl, clock_opa_slider, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

    /* Color presets */
    y += row_h + 4;
    lv_obj_t * co_lbl = lv_label_create(dlg);
    lv_label_set_text(co_lbl, "Color:");
    lv_obj_set_style_text_font(co_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(co_lbl, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(co_lbl, LV_ALIGN_TOP_LEFT, lbl_x, y + 6);

    int cb_x = ctrl_x;
    for(size_t i = 0; i < NUM_COLORS; i++) {
        lv_obj_t * btn = lv_button_create(dlg);
        lv_obj_set_size(btn, 24, 24);
        lv_obj_set_style_bg_color(btn, lv_color_hex(preset_colors_hex[i]), 0);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, cb_x, y + 4);
        lv_obj_add_event_cb(btn, clock_color_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        cb_x += 30;
    }

    /* Buttons */
    y += 48;
    lv_obj_t * bs = lv_button_create(dlg);
    lv_obj_set_size(bs, 130, 35);
    lv_obj_set_style_bg_color(bs, UI_COLOR_ACCENT, 0);
    lv_obj_align(bs, LV_ALIGN_BOTTOM_LEFT, 25, -5);
    lv_obj_t * ls = lv_label_create(bs);
    lv_label_set_text(ls, "Guardar");
    lv_obj_set_style_text_font(ls, &lv_font_montserrat_14, 0);
    lv_obj_align(ls, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bs, clock_edit_save_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * bc = lv_button_create(dlg);
    lv_obj_set_size(bc, 130, 35);
    lv_obj_set_style_bg_color(bc, UI_COLOR_CARD_BORDER, 0);
    lv_obj_align(bc, LV_ALIGN_BOTTOM_RIGHT, -25, -5);
    lv_obj_t * lc = lv_label_create(bc);
    lv_label_set_text(lc, "Cancelar");
    lv_obj_set_style_text_font(lc, &lv_font_montserrat_14, 0);
    lv_obj_align(lc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bc, clock_edit_cancel_cb, LV_EVENT_CLICKED, NULL);
}

/* ══════════════════════════════════════════════════════════
   dashboard_tab_init — main entry point
   ══════════════════════════════════════════════════════════ */
void dashboard_tab_init(lv_obj_t * parent)
{
    clock_load_settings();

    lv_obj_set_layout(parent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_style_pad_column(parent, 12, 0);
    lv_obj_set_style_pad_row(parent, 12, 0);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);

    /* ────────────────────────────────────────────
       CARD 1: RELOJ + FECHA + SISTEMA
       ──────────────────────────────────────────── */
    lv_obj_t * card_clock = ui_create_card(parent, 285, 120);

    time_label = lv_label_create(card_clock);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_add_flag(time_label, LV_OBJ_FLAG_CLICKABLE);
    apply_clock_style();
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_add_event_cb(time_label, clock_click_cb, LV_EVENT_CLICKED, NULL);

    date_label = lv_label_create(card_clock);
    lv_label_set_text(date_label, "Cargando...");
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(date_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align_to(date_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    lv_obj_t * sys = lv_label_create(card_clock);
    lv_label_set_text(sys, "SYS OK | toram");
    lv_obj_set_style_text_font(sys, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sys, UI_COLOR_ACCENT, 0);
    lv_obj_align(sys, LV_ALIGN_BOTTOM_LEFT, 5, -5);

    /* ────────────────────────────────────────────
       CARD 2: ALARMAS  (4 slots)
       ──────────────────────────────────────────── */
    lv_obj_t * card_alarms = ui_create_card(parent, 300, 120);

    lv_obj_t * atitle = lv_label_create(card_alarms);
    lv_label_set_text(atitle, "ALARMAS");
    lv_obj_set_style_text_font(atitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(atitle, UI_COLOR_ACCENT, 0);
    lv_obj_align(atitle, LV_ALIGN_TOP_LEFT, 5, 2);

    /* Create a scrollable list area for slots */
    lv_obj_t * slot_area = lv_obj_create(card_alarms);
    lv_obj_set_size(slot_area, 280, 85);
    lv_obj_set_style_bg_opa(slot_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(slot_area, 0, 0);
    lv_obj_set_style_pad_all(slot_area, 0, 0);
    lv_obj_set_style_pad_row(slot_area, 4, 0);
    lv_obj_align(slot_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_layout(slot_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(slot_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(slot_area, LV_SCROLLBAR_MODE_OFF);

    for(int i = 0; i < ALARM_SLOTS; i++) {
        /* Row per slot */
        lv_obj_t * row = lv_obj_create(slot_area);
        lv_obj_set_size(row, 270, 18);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /* Slot label:  "1  08:00" */
        alarm_labels[i] = lv_label_create(row);
        lv_obj_set_style_text_font(alarm_labels[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(alarm_labels[i], UI_COLOR_TEXT_MAIN, 0);
        update_alarm_slot_label(i);

        /* Switch */
        alarm_switches[i] = lv_switch_create(row);
        lv_obj_set_size(alarm_switches[i], 36, 18);
        if(g_alarms[i].enabled) lv_obj_add_state(alarm_switches[i], LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(alarm_switches[i], UI_COLOR_ACCENT, LV_STATE_CHECKED | LV_PART_INDICATOR);
        lv_obj_add_event_cb(alarm_switches[i], alarm_switch_cb, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);

        /* Adjust button */
        lv_obj_t * ab = lv_button_create(row);
        lv_obj_set_size(ab, 20, 18);
        lv_obj_set_style_bg_color(ab, UI_COLOR_CARD_BORDER, 0);
        lv_obj_set_style_pad_all(ab, 0, 0);
        lv_obj_set_style_radius(ab, 4, 0);
        lv_obj_t * al = lv_label_create(ab);
        lv_label_set_text(al, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_font(al, &lv_font_montserrat_14, 0);
        lv_obj_align(al, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(ab, btn_adjust_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    /* ────────────────────────────────────────────
       CARD 3: CLIMA (Open-Meteo) - ancho completo
       ──────────────────────────────────────────── */
    lv_obj_t * card_w = ui_create_card(parent, 600, 130);

    lv_obj_t * wt = lv_label_create(card_w);
    lv_label_set_text(wt, "CLIMA ACTUAL");
    lv_obj_set_style_text_font(wt, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wt, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(wt, LV_ALIGN_TOP_LEFT, 8, 5);

    /* Refresh button */
    lv_obj_t * ref_btn = lv_button_create(card_w);
    lv_obj_set_size(ref_btn, 32, 26);
    lv_obj_set_style_bg_color(ref_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_radius(ref_btn, 6, 0);
    lv_obj_align(ref_btn, LV_ALIGN_TOP_RIGHT, -8, 3);
    lv_obj_t * ref_icon = lv_label_create(ref_btn);
    lv_label_set_text(ref_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(ref_icon, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(ref_icon, UI_COLOR_ACCENT, 0);
    lv_obj_align(ref_icon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(ref_btn, weather_refresh_cb, LV_EVENT_CLICKED, NULL);

    /* Left column: temperature + description */
    weather_temp_label = lv_label_create(card_w);
    lv_label_set_text(weather_temp_label, "--.- C");
    lv_obj_set_style_text_font(weather_temp_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(weather_temp_label, UI_COLOR_ALERT, 0);
    lv_obj_align(weather_temp_label, LV_ALIGN_TOP_LEFT, 8, 32);

    weather_desc_label = lv_label_create(card_w);
    lv_label_set_text(weather_desc_label, "Cargando...");
    lv_obj_set_style_text_font(weather_desc_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(weather_desc_label, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align_to(weather_desc_label, weather_temp_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    /* Middle column: humidity + wind */
    weather_extra_label = lv_label_create(card_w);
    lv_label_set_text(weather_extra_label, "");
    lv_obj_set_style_text_font(weather_extra_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(weather_extra_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(weather_extra_label, LV_ALIGN_TOP_LEFT, 180, 36);
    lv_obj_set_width(weather_extra_label, 160);

    /* Right column: update time + city */
    weather_status_label = lv_label_create(card_w);
    lv_label_set_text(weather_status_label, "");
    lv_obj_set_style_text_font(weather_status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(weather_status_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(weather_status_label, LV_ALIGN_TOP_LEFT, 360, 36);

    /* ────────────────────────────────────────────
       CARD 4: RADIO ONLINE — grid de pulsadores
       ──────────────────────────────────────────── */

    /* Card height: title(20) + now(18) + grid rows + vol row(40) + padding */
    int cols = 4;
    int btn_w = 132;
    int btn_h = 38;
    int gap  = 6;
    int rows = (g_radio_preset_count + cols - 1) / cols;
    int grid_h = rows * (btn_h + gap) - gap;
    int card_h = 20 + 4 + 18 + 4 + grid_h + 12 + 40 + 8;
    lv_obj_t * card_r = ui_create_card(parent, 600, card_h);
    lv_obj_set_style_pad_all(card_r, 8, 0);

    /* Title + now playing on same line */
    lv_obj_t * rt = lv_label_create(card_r);
    lv_label_set_text(rt, "RADIO ONLINE");
    lv_obj_set_style_text_font(rt, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(rt, UI_COLOR_ACCENT, 0);
    lv_obj_align(rt, LV_ALIGN_TOP_LEFT, 8, 4);

    radio_now_label = lv_label_create(card_r);
    lv_label_set_text(radio_now_label, "Detenido");
    lv_obj_set_style_text_font(radio_now_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(radio_now_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(radio_now_label, LV_ALIGN_TOP_RIGHT, -8, 6);

    /* Grid container */
    int grid_top = 48;
    radio_grid = lv_obj_create(card_r);
    lv_obj_set_size(radio_grid, 580, grid_h);
    lv_obj_set_style_bg_opa(radio_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(radio_grid, 0, 0);
    lv_obj_set_style_pad_all(radio_grid, 0, 0);
    lv_obj_set_style_pad_column(radio_grid, gap, 0);
    lv_obj_set_style_pad_row(radio_grid, gap, 0);
    lv_obj_align(radio_grid, LV_ALIGN_TOP_LEFT, 8, grid_top);
    lv_obj_set_layout(radio_grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(radio_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_clear_flag(radio_grid, LV_OBJ_FLAG_SCROLLABLE);

    for(int i = 0; i < g_radio_preset_count; i++) {
        radio_btns[i] = lv_button_create(radio_grid);
        lv_obj_set_size(radio_btns[i], btn_w, btn_h);
        lv_obj_set_style_bg_color(radio_btns[i], UI_COLOR_CARD_BORDER, 0);
        lv_obj_set_style_radius(radio_btns[i], 6, 0);
        lv_obj_set_style_pad_all(radio_btns[i], 0, 0);

        lv_obj_t * lbl = lv_label_create(radio_btns[i]);
        lv_label_set_text(lbl, g_radio_presets[i].name);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_MAIN, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        lv_obj_add_event_cb(radio_btns[i], radio_btn_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }
    /* Clear remaining slots */
    for(int i = g_radio_preset_count; i < RADIO_MAX_STATIONS; i++)
        radio_btns[i] = NULL;

    /* Volume + Stop row */
    int vol_y = grid_top + grid_h + 8;
    radio_vol_label = lv_label_create(card_r);
    lv_label_set_text(radio_vol_label, "Vol: 70%");
    lv_obj_set_style_text_font(radio_vol_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(radio_vol_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(radio_vol_label, LV_ALIGN_TOP_LEFT, 8, vol_y);

    radio_vol_down_btn = lv_button_create(card_r);
    lv_obj_set_size(radio_vol_down_btn, 48, 38);
    lv_obj_set_style_bg_color(radio_vol_down_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_radius(radio_vol_down_btn, 6, 0);
    lv_obj_align_to(radio_vol_down_btn, radio_vol_label, LV_ALIGN_OUT_RIGHT_MID, 12, 0);
    lv_obj_t * down_lbl = lv_label_create(radio_vol_down_btn);
    lv_label_set_text(down_lbl, LV_SYMBOL_MINUS);
    lv_obj_set_style_text_font(down_lbl, font_for_size(20), 0);
    lv_obj_align(down_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(radio_vol_down_btn, radio_vol_down_cb, LV_EVENT_CLICKED, NULL);

    radio_vol_up_btn = lv_button_create(card_r);
    lv_obj_set_size(radio_vol_up_btn, 48, 38);
    lv_obj_set_style_bg_color(radio_vol_up_btn, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_radius(radio_vol_up_btn, 6, 0);
    lv_obj_align_to(radio_vol_up_btn, radio_vol_down_btn, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    lv_obj_t * up_lbl = lv_label_create(radio_vol_up_btn);
    lv_label_set_text(up_lbl, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_font(up_lbl, font_for_size(20), 0);
    lv_obj_align(up_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(radio_vol_up_btn, radio_vol_up_cb, LV_EVENT_CLICKED, NULL);

    radio_stop_btn = lv_button_create(card_r);
    lv_obj_set_size(radio_stop_btn, 90, 34);
    lv_obj_set_style_bg_color(radio_stop_btn, lv_color_hex(0xDC2626), 0);
    lv_obj_set_style_radius(radio_stop_btn, 6, 0);
    lv_obj_align(radio_stop_btn, LV_ALIGN_TOP_RIGHT, -8, vol_y);
    lv_obj_t * stop_lbl = lv_label_create(radio_stop_btn);
    lv_label_set_text(stop_lbl, LV_SYMBOL_STOP " STOP");
    lv_obj_set_style_text_font(stop_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(stop_lbl, lv_color_white(), 0);
    lv_obj_align(stop_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(radio_stop_btn, radio_stop_cb, LV_EVENT_CLICKED, NULL);

    update_radio_display();

    /* ── Show clock immediately ── */
    update_clock_display();
    lv_timer_create(clock_timer_cb, 1000, NULL);

    /* ── Show weather and start 15-min refresh timer ── */
    update_weather_display();
    lv_timer_create(weather_timer_cb, WEATHER_UPDATE_INTERVAL * 1000, NULL);
}
