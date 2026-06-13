#ifndef RADIO_MANAGER_H
#define RADIO_MANAGER_H

#include <stdbool.h>

#define RADIO_MAX_STATIONS 16
#define RADIO_NAME_MAX     48
#define RADIO_URL_MAX      256

typedef struct {
    char name[RADIO_NAME_MAX];
    char url[RADIO_URL_MAX];
} radio_preset_t;

extern radio_preset_t g_radio_presets[RADIO_MAX_STATIONS];
extern int            g_radio_preset_count;
extern int            g_radio_current;   /* -1 = none */
extern bool           g_radio_playing;
extern int            g_radio_volume;

void radio_init(void);
void radio_play(int index);
void radio_stop(void);
void radio_toggle(int index);
void radio_set_volume(int vol);

#endif