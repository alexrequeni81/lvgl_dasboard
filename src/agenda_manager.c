#include "agenda_manager.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define AGENDA_PATH "agenda.json"
#else
  #define AGENDA_PATH "/home/tc/agenda.json"
#endif

agenda_event_t g_events[MAX_EVENTS];
int g_event_count = 0;

void agenda_init(void)
{
    g_event_count = 0;
    memset(g_events, 0, sizeof(g_events));

    FILE * f = fopen(AGENDA_PATH, "r");
    if(!f) {
        printf("[agenda] %s not found. Starting with empty agenda.\n", AGENDA_PATH);
        return;
    }

    /* Seek to end to find file size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0) {
        fclose(f);
        return;
    }

    char * buf = malloc(size + 1);
    if(!buf) {
        fclose(f);
        printf("[agenda] Failed to allocate memory for reading agenda.\n");
        return;
    }

    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    cJSON * root = cJSON_Parse(buf);
    free(buf);

    if(!root) {
        printf("[agenda] Failed to parse JSON in %s\n", AGENDA_PATH);
        return;
    }

    cJSON * child = root->child;
    while(child && g_event_count < MAX_EVENTS) {
        if(cJSON_IsString(child)) {
            int y, m, d;
            if(sscanf(child->string, "%d-%d-%d", &y, &m, &d) == 3) {
                g_events[g_event_count].year = y;
                g_events[g_event_count].month = m;
                g_events[g_event_count].day = d;
                strncpy(g_events[g_event_count].description, child->valuestring, EVENT_DESC_MAX - 1);
                g_events[g_event_count].description[EVENT_DESC_MAX - 1] = '\0';
                g_event_count++;
            }
        }
        child = child->next;
    }

    cJSON_Delete(root);
    printf("[agenda] Loaded %d events from %s\n", g_event_count, AGENDA_PATH);
}

void agenda_save(void)
{
    cJSON * root = cJSON_CreateObject();
    if(!root) return;

    for(int i = 0; i < g_event_count; i++) {
        char date_str[16];
        snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d", g_events[i].year, g_events[i].month, g_events[i].day);
        cJSON_AddStringToObject(root, date_str, g_events[i].description);
    }

    char * json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if(!json_str) {
        printf("[agenda] Failed to serialize events.\n");
        return;
    }

    FILE * f = fopen(AGENDA_PATH, "w");
    if(!f) {
        printf("[agenda] Failed to open %s for writing.\n", AGENDA_PATH);
        free(json_str);
        return;
    }

    fputs(json_str, f);
    fclose(f);
    free(json_str);

    printf("[agenda] Saved %d events to %s\n", g_event_count, AGENDA_PATH);
}

const char * agenda_get_event(uint16_t year, uint8_t month, uint8_t day)
{
    for(int i = 0; i < g_event_count; i++) {
        if(g_events[i].year == year && g_events[i].month == month && g_events[i].day == day) {
            return g_events[i].description;
        }
    }
    return NULL;
}

bool agenda_set_event(uint16_t year, uint8_t month, uint8_t day, const char * desc)
{
    if(!desc || strlen(desc) == 0) {
        agenda_delete_event(year, month, day);
        return true;
    }

    /* Search for existing event to update */
    for(int i = 0; i < g_event_count; i++) {
        if(g_events[i].year == year && g_events[i].month == month && g_events[i].day == day) {
            strncpy(g_events[i].description, desc, EVENT_DESC_MAX - 1);
            g_events[i].description[EVENT_DESC_MAX - 1] = '\0';
            agenda_save();
            return true;
        }
    }

    /* Check limit */
    if(g_event_count >= MAX_EVENTS) {
        printf("[agenda] Event limit reached! Cannot add event on %04d-%02d-%02d\n", year, month, day);
        return false;
    }

    /* Add new event */
    g_events[g_event_count].year = year;
    g_events[g_event_count].month = month;
    g_events[g_event_count].day = day;
    strncpy(g_events[g_event_count].description, desc, EVENT_DESC_MAX - 1);
    g_events[g_event_count].description[EVENT_DESC_MAX - 1] = '\0';
    g_event_count++;

    agenda_save();
    return true;
}

void agenda_delete_event(uint16_t year, uint8_t month, uint8_t day)
{
    for(int i = 0; i < g_event_count; i++) {
        if(g_events[i].year == year && g_events[i].month == month && g_events[i].day == day) {
            /* Shift remaining events left */
            for(int j = i; j < g_event_count - 1; j++) {
                g_events[j] = g_events[j + 1];
            }
            g_event_count--;
            agenda_save();
            return;
        }
    }
}

int agenda_get_highlighted_dates(lv_calendar_date_t * dates, int max_dates)
{
    int count = 0;
    for(int i = 0; i < g_event_count && count < max_dates; i++) {
        dates[count].year = g_events[i].year;
        dates[count].month = g_events[i].month;
        dates[count].day = g_events[i].day;
        count++;
    }
    return count;
}
