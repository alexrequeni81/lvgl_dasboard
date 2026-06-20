#ifndef WEATHER_H
#define WEATHER_H

#include <stdbool.h>
#include <time.h>

#define WEATHER_UPDATE_INTERVAL 900 /* 15 min */

typedef struct {
    float temperature;
    int   humidity;
    float wind_speed;
    int   weather_code;
    char  description[48];
    char  city_name[32];
    time_t fetch_time;   /* when data was fetched */
    bool   valid;
} weather_data_t;

extern weather_data_t g_weather;
extern float g_weather_lat;
extern float g_weather_lon;
extern char  g_weather_zip[16];
extern char  g_weather_country[4];
extern char  g_weather_last_error[128];
extern bool  g_weather_last_ok;

void weather_init(void);
bool weather_auto_detect(void);
bool weather_update(void);
void weather_load_settings(void);
void weather_save_settings(void);
void weather_set_coords(float lat, float lon);
void weather_set_zip(const char * zip);
bool weather_geocode_zip(void);
const char * weather_code_to_str(int code);

#endif
