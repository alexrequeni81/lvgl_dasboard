#include "wifi_manager.h"
#include "ui/ui.h"
#include "ui/dashboard_tab.h"
#include "weather.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define SETTINGS_PATH "settings.conf"
#else
  #define SETTINGS_PATH "/home/tc/settings.conf"
  #include <unistd.h>
#endif

wifi_config_t g_wifi = { .ssid = "", .password = "" };
static lv_obj_t * wf_modal_bg = NULL;

void wifi_manager_save(void)
{
    FILE * f = fopen(SETTINGS_PATH, "r+");
    if(!f) f = fopen(SETTINGS_PATH, "a");
    if(!f) return;

    fseek(f, 0, SEEK_END); long flen = ftell(f); rewind(f);
    char * buf = NULL;
    if(flen > 0) { buf = (char *)malloc(flen + 1); size_t r = fread(buf, 1, flen, f); buf[r] = '\0'; }
    fclose(f);

    f = fopen(SETTINGS_PATH, "w");
    if(!f) { free(buf); return; }

    int ssid_found = 0, pass_found = 0;
    if(buf) {
        char line[256];
        char * pos = buf;
        while(pos && *pos) {
            char * nl = strchr(pos, '\n');
            if(nl) { size_t len = nl - pos; if(len > 255) len = 255; strncpy(line, pos, len); line[len] = '\0'; pos = nl + 1; }
            else { strncpy(line, pos, 255); line[255] = '\0'; pos = NULL; }
            if(strncmp(line, "wifi_ssid=", 10) == 0) { fprintf(f, "wifi_ssid=%s\n", g_wifi.ssid); ssid_found = 1; }
            else if(strncmp(line, "wifi_password=", 14) == 0) { fprintf(f, "wifi_password=%s\n", g_wifi.password); pass_found = 1; }
            else if(strlen(line) > 0) fprintf(f, "%s\n", line);
        }
        free(buf);
    }
    if(!ssid_found) fprintf(f, "wifi_ssid=%s\n", g_wifi.ssid);
    if(!pass_found) fprintf(f, "wifi_password=%s\n", g_wifi.password);
    fclose(f);
}

void wifi_manager_load(void)
{
    g_wifi.ssid[0] = '\0';
    g_wifi.password[0] = '\0';

    FILE * f = fopen(SETTINGS_PATH, "r");
    if(!f) return;

    char line[256];
    while(fgets(line, sizeof(line), f)) {
        char * nl = strchr(line, '\n'); if(nl) *nl = '\0';
        if(sscanf(line, "wifi_ssid=%63s", g_wifi.ssid) == 1) {}
        else if(strncmp(line, "wifi_password=", 14) == 0) {
            strncpy(g_wifi.password, line + 14, WIFI_PASS_MAX - 1);
            g_wifi.password[WIFI_PASS_MAX - 1] = '\0';
            size_t len = strlen(g_wifi.password);
            while(len > 0 && (g_wifi.password[len-1] == '\n' || g_wifi.password[len-1] == '\r' || g_wifi.password[len-1] == ' '))
                g_wifi.password[--len] = '\0';
        }
    }
    fclose(f);
}

void wifi_manager_init(void)
{
    wifi_manager_load();
}

const char * wifi_status_str(void)
{
#ifdef _WIN32
    (void)g_wifi;
    return "Red: PC";
#else
    return g_wifi.ssid[0] ? g_wifi.ssid : "Red: sin WiFi";
#endif
}

/* ── Modal ── */

static void wf_close_modal(void)
{
    if(wf_modal_bg) {
        lv_obj_delete(wf_modal_bg);
        wf_modal_bg = NULL;
    }
}

static void wf_close_cb(lv_event_t * e)
{
    (void)e;
    wf_close_modal();
}

/* ── Modal: zip textarea handle ── */
static lv_obj_t * wf_ta_zip = NULL;
static lv_obj_t * wf_test_result = NULL;

static void wf_test_conn_cb(lv_event_t * e)
{
    (void)e;
    if(wf_test_result) lv_label_set_text(wf_test_result, "Geocodificando...");

    /* Read zip from textarea */
    if(wf_ta_zip) {
        const char * zip = lv_textarea_get_text(wf_ta_zip);
        if(zip && zip[0]) weather_set_zip(zip);
    }

    if(wf_test_result) {
        if(g_weather_last_error[0] && !g_weather_last_ok) {
            lv_label_set_text(wf_test_result, g_weather_last_error);
            update_weather_display();
            return;
        }
    }

    lv_label_set_text(wf_test_result, "Obteniendo clima...");
    bool ok = weather_update();
    if(wf_test_result) {
        if(ok) {
            char buf[96];
            snprintf(buf, sizeof(buf), "OK: %.1f C, %s (%s)",
                     g_weather.temperature, g_weather.description,
                     g_weather.city_name[0] ? g_weather.city_name : g_weather_zip);
            lv_label_set_text(wf_test_result, buf);
        } else {
            lv_label_set_text(wf_test_result, g_weather_last_error);
        }
    }
    update_weather_display();
}

void wifi_show_config_modal(lv_obj_t * parent)
{
    if(wf_modal_bg) return;

    wf_modal_bg = lv_obj_create(parent ? parent : lv_screen_active());
    lv_obj_set_size(wf_modal_bg, 800, 600);
    lv_obj_set_style_bg_color(wf_modal_bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(wf_modal_bg, LV_OPA_50, 0);
    lv_obj_set_style_border_width(wf_modal_bg, 0, 0);
    lv_obj_set_style_radius(wf_modal_bg, 0, 0);
    lv_obj_clear_flag(wf_modal_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(wf_modal_bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(wf_modal_bg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * dlg = ui_create_card(wf_modal_bg, 380, 320);
    lv_obj_align(dlg, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t * title = lv_label_create(dlg);
    lv_label_set_text(title, "RED / CONFIGURACION");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, UI_COLOR_ACCENT, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* ── Location section ── */
    lv_obj_t * loc_label = lv_label_create(dlg);
    lv_label_set_text(loc_label, "Codigo Postal:");
    lv_obj_set_style_text_color(loc_label, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(loc_label, &lv_font_montserrat_14, 0);
    lv_obj_align(loc_label, LV_ALIGN_TOP_LEFT, 20, 40);

    wf_ta_zip = lv_textarea_create(dlg);
    lv_obj_set_size(wf_ta_zip, 140, 32);
    lv_textarea_set_text(wf_ta_zip, g_weather_zip);
    lv_textarea_set_one_line(wf_ta_zip, true);
    lv_textarea_set_accepted_chars(wf_ta_zip, "0123456789");
    lv_obj_set_style_bg_color(wf_ta_zip, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_text_color(wf_ta_zip, UI_COLOR_TEXT_MAIN, 0);
    lv_obj_align(wf_ta_zip, LV_ALIGN_TOP_LEFT, 20, 62);
    ui_attach_keyboard(wf_ta_zip, LV_KEYBOARD_MODE_NUMBER);

    /* City label (shows geocoded location) */
    lv_obj_t * city_label = lv_label_create(dlg);
    lv_label_set_text(city_label, g_weather.city_name[0] ? g_weather.city_name : "");
    lv_obj_set_style_text_font(city_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(city_label, UI_COLOR_ACCENT, 0);
    lv_obj_align_to(city_label, wf_ta_zip, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    /* ── Network info ── */
    lv_obj_t * net_info = lv_label_create(dlg);
#ifdef _WIN32
    lv_label_set_text(net_info, "Red: conexion del PC (simulador)");
#else
    lv_label_set_text(net_info, "Red: WiFi del dispositivo");
#endif
    lv_obj_set_style_text_font(net_info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(net_info, UI_COLOR_TEXT_MUTED, 0);
    lv_obj_align(net_info, LV_ALIGN_TOP_LEFT, 20, 105);

    /* ── Test result ── */
    wf_test_result = lv_label_create(dlg);
    lv_label_set_text(wf_test_result, "");
    lv_obj_set_style_text_font(wf_test_result, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wf_test_result, UI_COLOR_ACCENT, 0);
    lv_obj_set_width(wf_test_result, 340);
    lv_obj_align(wf_test_result, LV_ALIGN_TOP_LEFT, 20, 130);

    /* ── Test connection button ── */
    lv_obj_t * test_btn = lv_button_create(dlg);
    lv_obj_set_size(test_btn, 200, 38);
    lv_obj_set_style_bg_color(test_btn, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_radius(test_btn, 6, 0);
    lv_obj_align(test_btn, LV_ALIGN_TOP_LEFT, 20, 160);
    lv_obj_t * test_lbl = lv_label_create(test_btn);
    lv_label_set_text(test_lbl, "Probar conexion");
    lv_obj_set_style_text_font(test_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(test_lbl, lv_color_white(), 0);
    lv_obj_align(test_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(test_btn, wf_test_conn_cb, LV_EVENT_CLICKED, NULL);

    /* ── Close button ── */
    lv_obj_t * bc = lv_button_create(dlg);
    lv_obj_set_size(bc, 120, 38);
    lv_obj_set_style_bg_color(bc, UI_COLOR_CARD_BORDER, 0);
    lv_obj_set_style_radius(bc, 6, 0);
    lv_obj_align(bc, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t * lc = lv_label_create(bc);
    lv_label_set_text(lc, "Cerrar");
    lv_obj_set_style_text_font(lc, &lv_font_montserrat_14, 0);
    lv_obj_align(lc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(bc, wf_close_cb, LV_EVENT_CLICKED, NULL);
}
