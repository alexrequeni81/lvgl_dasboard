#ifndef AGENDA_MANAGER_H
#define AGENDA_MANAGER_H

#include "lvgl/lvgl.h"
#include <stdbool.h>

#define MAX_EVENTS 128
#define EVENT_DESC_MAX 128

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    char description[EVENT_DESC_MAX];
} agenda_event_t;

extern agenda_event_t g_events[MAX_EVENTS];
extern int g_event_count;

/* Initialize the agenda manager (loads events from agenda.json) */
void agenda_init(void);

/* Save the current list of events to agenda.json */
void agenda_save(void);

/* Retrieve event description for a given date. Returns NULL if no event exists. */
const char * agenda_get_event(uint16_t year, uint8_t month, uint8_t day);

/* Add or update an event for a given date. Returns true on success. */
bool agenda_set_event(uint16_t year, uint8_t month, uint8_t day, const char * desc);

/* Delete the event on the given date if it exists */
void agenda_delete_event(uint16_t year, uint8_t month, uint8_t day);

/* Fill an array of lv_calendar_date_t for LVGL. Returns the count. */
int agenda_get_highlighted_dates(lv_calendar_date_t * dates, int max_dates);

#endif /* AGENDA_MANAGER_H */
