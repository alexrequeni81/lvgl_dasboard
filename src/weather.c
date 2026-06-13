#include "weather.h"
#include "net_manager.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
  #define SETTINGS_PATH "settings.conf"
  #define WEATHER_CACHE_FILE "weather_cache.json"
#else
  #define SETTINGS_PATH "/home/tc/settings.conf"
  #define WEATHER_CACHE_FILE "/tmp/weather_cache.json"
#endif

/* ── Globals ── */
weather_data_t g_weather = { .valid = false };
float g_weather_lat = 40.42f;
float g_weather_lon = -3.70f;
char  g_weather_zip[16] = "28001";
char  g_weather_country[4] = "ES";
char  g_weather_last_error[128] = "";
bool  g_weather_last_ok = false;

/* ── WMO Weather Code → Spanish description ── */
const char * weather_code_to_str(int code)
{
    if(code == 0)              return "Despejado";
    if(code == 1)              return "Mayormente despejado";
    if(code == 2)              return "Parcialmente nublado";
    if(code == 3)              return "Nublado";
    if(code >= 45 && code <= 48) return "Niebla";
    if(code >= 51 && code <= 55) return "Llovizna";
    if(code >= 56 && code <= 57) return "Llovizna helada";
    if(code >= 61 && code <= 65) return "Lluvia";
    if(code >= 66 && code <= 67) return "Lluvia helada";
    if(code >= 71 && code <= 75) return "Nieve";
    if(code >= 77 && code <= 77) return "Granos de nieve";
    if(code >= 80 && code <= 82) return "Chubascos";
    if(code >= 85 && code <= 86) return "Chubascos de nieve";
    if(code >= 95 && code <= 99) return "Tormenta";
    return "Desconocido";
}

/* ── Settings from settings.conf (zip + cached lat/lon) ── */
void weather_load_settings(void)
{
    FILE * f = fopen(SETTINGS_PATH, "r");
    if(!f) return;
    char line[128];
    while(fgets(line, sizeof(line), f)) {
        float val;
        char buf[32];
        if(sscanf(line, "weather_lat=%f", &val) == 1) g_weather_lat = val;
        if(sscanf(line, "weather_lon=%f", &val) == 1) g_weather_lon = val;
        if(sscanf(line, "weather_zip=%16s", buf) == 1) strncpy(g_weather_zip, buf, sizeof(g_weather_zip) - 1);
        if(sscanf(line, "weather_country=%4s", buf) == 1) strncpy(g_weather_country, buf, sizeof(g_weather_country) - 1);
    }
    fclose(f);
    printf("[weather] Zip: %s, Coords: %.2f, %.2f\n", g_weather_zip, g_weather_lat, g_weather_lon);
}

void weather_save_settings(void)
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

    int zip_found = 0, country_found = 0, lat_found = 0, lon_found = 0;
    if(buf) {
        char line[256];
        char * pos = buf;
        while(pos && *pos) {
            char * nl = strchr(pos, '\n');
            if(nl) { size_t len = nl - pos; if(len > 255) len = 255; strncpy(line, pos, len); line[len] = '\0'; pos = nl + 1; }
            else { strncpy(line, pos, 255); line[255] = '\0'; pos = NULL; }

            if(strncmp(line, "weather_zip=", 12) == 0) { fprintf(f, "weather_zip=%s\n", g_weather_zip); zip_found = 1; }
            else if(strncmp(line, "weather_country=", 16) == 0) { fprintf(f, "weather_country=%s\n", g_weather_country); country_found = 1; }
            else if(strncmp(line, "weather_lat=", 12) == 0) { fprintf(f, "weather_lat=%.4f\n", g_weather_lat); lat_found = 1; }
            else if(strncmp(line, "weather_lon=", 12) == 0) { fprintf(f, "weather_lon=%.4f\n", g_weather_lon); lon_found = 1; }
            else if(strlen(line) > 0) fprintf(f, "%s\n", line);
        }
        free(buf);
    }
    if(!zip_found) fprintf(f, "weather_zip=%s\n", g_weather_zip);
    if(!country_found) fprintf(f, "weather_country=%s\n", g_weather_country);
    if(!lat_found) fprintf(f, "weather_lat=%.4f\n", g_weather_lat);
    if(!lon_found) fprintf(f, "weather_lon=%.4f\n", g_weather_lon);
    fclose(f);
}

/* ── Cache to file (so we have data quickly on next boot) ── */
void weather_cache_save(void)
{
    FILE * f = fopen(WEATHER_CACHE_FILE, "w");
    if(!f) return;
    fprintf(f, "{\"temp\":%.1f,\"humidity\":%d,\"wind\":%.1f,\"code\":%d,\"time\":%lld,\"desc\":\"%s\"}\n",
            g_weather.temperature, g_weather.humidity, g_weather.wind_speed,
            g_weather.weather_code, (long long)g_weather.fetch_time,
            g_weather.description);
    fclose(f);
}

void weather_cache_load(void)
{
    FILE * f = fopen(WEATHER_CACHE_FILE, "r");
    if(!f) return;
    fseek(f, 0, SEEK_END); long flen = ftell(f); rewind(f);
    if(flen < 10) { fclose(f); return; }
    char * json = (char *)malloc(flen + 1);
    fread(json, 1, flen, f); json[flen] = '\0';
    fclose(f);

    cJSON * root = cJSON_Parse(json);
    if(root) {
        cJSON * t = cJSON_GetObjectItem(root, "temp");
        cJSON * h = cJSON_GetObjectItem(root, "humidity");
        cJSON * w = cJSON_GetObjectItem(root, "wind");
        cJSON * c = cJSON_GetObjectItem(root, "code");
        cJSON * ts = cJSON_GetObjectItem(root, "time");
        cJSON * d = cJSON_GetObjectItem(root, "desc");
        if(cJSON_IsNumber(t)) g_weather.temperature = (float)t->valuedouble;
        if(cJSON_IsNumber(h)) g_weather.humidity = h->valueint;
        if(cJSON_IsNumber(w)) g_weather.wind_speed = (float)w->valuedouble;
        if(cJSON_IsNumber(c)) g_weather.weather_code = c->valueint;
        if(cJSON_IsNumber(ts)) g_weather.fetch_time = (time_t)ts->valueint;
        if(cJSON_IsString(d)) strncpy(g_weather.description, d->valuestring, sizeof(g_weather.description) - 1);
        time_t now = time(NULL);
        if(now - g_weather.fetch_time < WEATHER_UPDATE_INTERVAL + 60) {
            g_weather.valid = true;
            printf("[weather] Cache loaded (age: %llds)\n", (long long)(now - g_weather.fetch_time));
        }
        cJSON_Delete(root);
    }
    free(json);
}

void weather_set_coords(float lat, float lon)
{
    if(lat < -90 || lat > 90) return;
    if(lon < -180 || lon > 180) return;
    g_weather_lat = lat;
    g_weather_lon = lon;
    weather_save_settings();
    printf("[weather] Coords set: %.4f, %.4f\n", lat, lon);
}

/* ── Geocode zip code via Open-Meteo ── */
bool weather_geocode_zip(void)
{
    char url[384];
    snprintf(url, sizeof(url),
             "https://geocoding-api.open-meteo.com/v1/search?name=%s&count=3&language=es&format=json",
             g_weather_zip);

    char resp[8192];
    if(!net_http_get(url, resp, sizeof(resp))) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error: no se pudo geocodificar %s (sin conexion)", g_weather_zip);
        return false;
    }

    cJSON * root = cJSON_Parse(resp);
    if(!root) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error: respuesta invalida del geocoder");
        return false;
    }

    cJSON * results = cJSON_GetObjectItem(root, "results");
    if(!results || !cJSON_IsArray(results) || cJSON_GetArraySize(results) == 0) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error: codigo postal %s no encontrado", g_weather_zip);
        cJSON_Delete(root);
        return false;
    }

    /* Take first result (best match) */
    cJSON * first = cJSON_GetArrayItem(results, 0);
    cJSON * lat = cJSON_GetObjectItem(first, "latitude");
    cJSON * lon = cJSON_GetObjectItem(first, "longitude");
    cJSON * country = cJSON_GetObjectItem(first, "country_code");
    cJSON * name = cJSON_GetObjectItem(first, "name");
    cJSON * admin1 = cJSON_GetObjectItem(first, "admin1");

    if(!cJSON_IsNumber(lat) || !cJSON_IsNumber(lon)) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error: geocoder no devolvio coordenadas");
        cJSON_Delete(root);
        return false;
    }

    g_weather_lat = (float)lat->valuedouble;
    g_weather_lon = (float)lon->valuedouble;

    if(cJSON_IsString(country))
        strncpy(g_weather_country, country->valuestring, sizeof(g_weather_country) - 1);

    /* Try to get city name from admin1 */
    if(cJSON_IsString(admin1)) {
        strncpy(g_weather.city_name, admin1->valuestring, sizeof(g_weather.city_name) - 1);
    } else if(cJSON_IsString(name)) {
        strncpy(g_weather.city_name, name->valuestring, sizeof(g_weather.city_name) - 1);
    }

    weather_save_settings();
    printf("[weather] Geocode %s -> %.4f, %.4f (%s)\n",
           g_weather_zip, g_weather_lat, g_weather_lon, g_weather.city_name);

    cJSON_Delete(root);
    return true;
}

void weather_set_zip(const char * zip)
{
    if(!zip || !zip[0]) return;
    strncpy(g_weather_zip, zip, sizeof(g_weather_zip) - 1);
    g_weather_zip[sizeof(g_weather_zip) - 1] = '\0';

    bool ok = weather_geocode_zip();
    if(!ok) {
        printf("[weather] Zip geocode failed, keeping previous coords\n");
        weather_save_settings();
    }
}

/* ── Fetch from Open-Meteo ── */
bool weather_update(void)
{
    char url[512];
    snprintf(url, sizeof(url),
             "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
             "&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code",
             g_weather_lat, g_weather_lon);

    char resp[8192];
    if(!net_http_get(url, resp, sizeof(resp))) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error HTTP: sin conexion o timeout");
        printf("[weather] HTTP request failed\n");
        g_weather_last_ok = false;
        return false;
    }

    cJSON * root = cJSON_Parse(resp);
    if(!root) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error: respuesta invalida del servidor");
        printf("[weather] JSON parse error\n");
        g_weather_last_ok = false;
        return false;
    }

    cJSON * current = cJSON_GetObjectItem(root, "current");
    if(!current) {
        snprintf(g_weather_last_error, sizeof(g_weather_last_error),
                 "Error: formato de respuesta inesperado");
        printf("[weather] No 'current' in response\n");
        cJSON_Delete(root);
        g_weather_last_ok = false;
        return false;
    }

    cJSON * temp = cJSON_GetObjectItem(current, "temperature_2m");
    cJSON * hum  = cJSON_GetObjectItem(current, "relative_humidity_2m");
    cJSON * wind = cJSON_GetObjectItem(current, "wind_speed_10m");
    cJSON * code = cJSON_GetObjectItem(current, "weather_code");

    if(cJSON_IsNumber(temp)) g_weather.temperature = (float)temp->valuedouble;
    if(cJSON_IsNumber(hum))  g_weather.humidity = hum->valueint;
    if(cJSON_IsNumber(wind)) g_weather.wind_speed = (float)wind->valuedouble;
    if(cJSON_IsNumber(code)) g_weather.weather_code = code->valueint;

    strncpy(g_weather.description, weather_code_to_str(g_weather.weather_code),
            sizeof(g_weather.description) - 1);
    g_weather.fetch_time = time(NULL);
    g_weather.valid = true;
    g_weather_last_ok = true;
    g_weather_last_error[0] = '\0';

    printf("[weather] %.1f°C, %d%% hum, %.1f km/h, %s\n",
           g_weather.temperature, g_weather.humidity,
           g_weather.wind_speed, g_weather.description);

    cJSON_Delete(root);
    weather_cache_save();
    return true;
}

void weather_init(void)
{
    weather_load_settings();
    weather_cache_load();
}
