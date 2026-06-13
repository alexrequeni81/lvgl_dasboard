#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "lvgl/lvgl.h"

#define WIFI_SSID_MAX 64
#define WIFI_PASS_MAX 64

typedef struct {
    char ssid[WIFI_SSID_MAX];
    char password[WIFI_PASS_MAX];
} wifi_config_t;

extern wifi_config_t g_wifi;

void wifi_manager_init(void);
void wifi_manager_save(void);
void wifi_manager_load(void);
const char * wifi_status_str(void);
void wifi_show_config_modal(lv_obj_t * parent);

#endif
