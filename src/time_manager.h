#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

/* Maximum number of simultaneous alarm slots */
#define ALARM_SLOTS 4

/* Per-alarm configuration */
typedef struct {
    int  hour;
    int  minute;
    bool enabled;
    bool triggered; /* Prevents re-triggering within the same minute */
} alarm_t;

/* Global state */
extern alarm_t  g_alarms[ALARM_SLOTS];
extern volatile bool g_alarm_ringing;   /* Sound thread reads this to know when to stop */
extern int      g_ringing_slot;         /* Which slot is currently ringing (-1 = none) */

/* ── Lifecycle ── */
void time_manager_init(void);
void time_manager_save(void);
void time_manager_load(void);

/* ── Per-tick check ──
   Call once per second. Returns index of triggered slot (0-3) or -1. */
int  time_manager_check_alarms(int cur_hour, int cur_minute);

/* ── Audio control ──
   Starts a background thread that beeps for up to 30 s.
   The thread exits as soon as g_alarm_ringing is set to false. */
void time_manager_start_sound(void);
void time_manager_stop_sound(void);   /* Sets g_alarm_ringing = false */

/* ── Snooze ──
   Stops sound and pushes the triggered alarm forward by 5 minutes. */
void time_manager_snooze(int slot);

#endif /* TIME_MANAGER_H */
