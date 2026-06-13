#include "time_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ── Platform specifics ── */
#ifdef _WIN32
  #include <windows.h>
  #define SETTINGS_PATH "settings.conf"
  typedef HANDLE thread_t;
#else
  #include <unistd.h>
  #include <pthread.h>
  #define SETTINGS_PATH "/home/tc/settings.conf"
  typedef pthread_t thread_t;
#endif

/* ── Globals ── */
alarm_t        g_alarms[ALARM_SLOTS];
volatile bool  g_alarm_ringing = false;
int            g_ringing_slot  = -1;

/* Forward declaration of sound thread entry point */
#ifdef _WIN32
static DWORD WINAPI sound_thread_fn(LPVOID arg);
#else
static void * sound_thread_fn(void * arg);
#endif

/* ─────────────────────────────────────────────────────── */
/* Persistence                                             */
/* ─────────────────────────────────────────────────────── */

void time_manager_save(void)
{
    FILE * f = fopen(SETTINGS_PATH, "w");
    if(!f) { printf("[alarm] Cannot write %s\n", SETTINGS_PATH); return; }

    for(int i = 0; i < ALARM_SLOTS; i++) {
        fprintf(f, "alarm%d_enabled=%d\n", i, g_alarms[i].enabled ? 1 : 0);
        fprintf(f, "alarm%d_hour=%d\n",    i, g_alarms[i].hour);
        fprintf(f, "alarm%d_minute=%d\n",  i, g_alarms[i].minute);
    }
    fclose(f);
    printf("[alarm] Settings saved to %s\n", SETTINGS_PATH);
}

void time_manager_load(void)
{
    /* Default values */
    for(int i = 0; i < ALARM_SLOTS; i++) {
        g_alarms[i].hour    = 8 + i * 2; /* 08, 10, 12, 14 */
        g_alarms[i].minute  = 0;
        g_alarms[i].enabled = false;
        g_alarms[i].triggered = false;
    }

    FILE * f = fopen(SETTINGS_PATH, "r");
    if(!f) {
        printf("[alarm] %s not found. Creating defaults.\n", SETTINGS_PATH);
        time_manager_save();
        return;
    }

    char line[128];
    while(fgets(line, sizeof(line), f)) {
        int idx, val;
        if(sscanf(line, "alarm%d_enabled=%d", &idx, &val) == 2 && idx < ALARM_SLOTS)
            g_alarms[idx].enabled = (val != 0);
        else if(sscanf(line, "alarm%d_hour=%d", &idx, &val) == 2 && idx < ALARM_SLOTS)
            g_alarms[idx].hour = val;
        else if(sscanf(line, "alarm%d_minute=%d", &idx, &val) == 2 && idx < ALARM_SLOTS)
            g_alarms[idx].minute = val;
    }
    fclose(f);

    printf("[alarm] Loaded %d alarm slots:\n", ALARM_SLOTS);
    for(int i = 0; i < ALARM_SLOTS; i++) {
        printf("  [%d] %02d:%02d  %s\n", i,
               g_alarms[i].hour, g_alarms[i].minute,
               g_alarms[i].enabled ? "ON" : "OFF");
    }
}

void time_manager_init(void)
{
    time_manager_load();
}

/* ─────────────────────────────────────────────────────── */
/* Per-tick check                                          */
/* ─────────────────────────────────────────────────────── */

int time_manager_check_alarms(int cur_hour, int cur_minute)
{
    for(int i = 0; i < ALARM_SLOTS; i++) {
        if(!g_alarms[i].enabled) {
            g_alarms[i].triggered = false;
            continue;
        }
        if(cur_hour == g_alarms[i].hour && cur_minute == g_alarms[i].minute) {
            if(!g_alarms[i].triggered && !g_alarm_ringing) {
                g_alarms[i].triggered = true;
                return i;  /* This slot just fired */
            }
        } else {
            g_alarms[i].triggered = false; /* Reset once time has passed */
        }
    }
    return -1; /* Nothing triggered */
}

/* ─────────────────────────────────────────────────────── */
/* Sound thread                                            */
/* ─────────────────────────────────────────────────────── */

#ifdef _WIN32
static DWORD WINAPI sound_thread_fn(LPVOID arg)
{
    (void)arg;
    DWORD start = GetTickCount();
    while(g_alarm_ringing) {
        if((GetTickCount() - start) >= 30000) break; /* 30-second timeout */
        Beep(880, 300);  /* A5 — sharp and audible */
        Sleep(150);
        Beep(660, 200);  /* E5 */
        Sleep(300);
    }
    g_alarm_ringing = false;
    printf("[alarm] Sound thread finished.\n");
    return 0;
}
#else
static void * sound_thread_fn(void * arg)
{
    (void)arg;
    time_t start = time(NULL);
    while(g_alarm_ringing) {
        if(difftime(time(NULL), start) >= 30.0) break;
        system("aplay -q /home/tc/sounds/alarm.wav 2>/dev/null");
    }
    g_alarm_ringing = false;
    return NULL;
}
#endif

void time_manager_start_sound(void)
{
    if(g_alarm_ringing) return; /* Already playing */
    g_alarm_ringing = true;
    printf("[alarm] Starting sound (30s max)...\n");

#ifdef _WIN32
    HANDLE t = CreateThread(NULL, 0, sound_thread_fn, NULL, 0, NULL);
    if(t) CloseHandle(t); /* Detach: we control via g_alarm_ringing */
#else
    pthread_t t;
    pthread_create(&t, NULL, sound_thread_fn, NULL);
    pthread_detach(t);
#endif
}

void time_manager_stop_sound(void)
{
    g_alarm_ringing = false;
    g_ringing_slot = -1;
    printf("[alarm] Sound stopped.\n");
}

/* ─────────────────────────────────────────────────────── */
/* Snooze                                                  */
/* ─────────────────────────────────────────────────────── */

void time_manager_snooze(int slot)
{
    if(slot < 0 || slot >= ALARM_SLOTS) return;
    time_manager_stop_sound();

    /* Advance alarm by 5 minutes */
    g_alarms[slot].minute += 5;
    if(g_alarms[slot].minute >= 60) {
        g_alarms[slot].minute -= 60;
        g_alarms[slot].hour = (g_alarms[slot].hour + 1) % 24;
    }
    g_alarms[slot].triggered = false; /* Allow re-trigger */

    time_manager_save();
    printf("[alarm] Slot %d snoozed → %02d:%02d\n",
           slot, g_alarms[slot].hour, g_alarms[slot].minute);
}
