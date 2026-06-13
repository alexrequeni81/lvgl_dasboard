#include "radio_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <windows.h>
  static HANDLE g_proc = NULL;
#else
  #include <unistd.h>
  #include <signal.h>
  #include <sys/types.h>
  #include <sys/wait.h>
  static pid_t g_player_pid = 0;
#endif

/* ── Globals ── */
radio_preset_t g_radio_presets[RADIO_MAX_STATIONS];
int   g_radio_preset_count = 0;
int   g_radio_current      = -1;
bool  g_radio_playing      = false;
int   g_radio_volume       = 70;

/* ── Default presets ── */
static const char * default_names[] = {
    "Radio Ribarroja",
    "Ona Digital",
    "Rock Star",
    "Rock Satelite",
    "4U Hard FM",
    "Rock Radio SI",
    "Cadena 100",
    "BBKiss FM",
};
static const char * default_urls[] = {
    "https://streaming.enacast.com:8000/radioribarroja128.mp3",
    "https://streaming.enacast.com/onadigital56.mp3",
    "https://stream2.mediasector.es/rockstar.mp3",
    "https://stream.tunerplay.com/listen/rocksatelite/rocksatelite.mp3",
    "https://superradio.streamakaci.com/4uhardfm.mp3",
    "http://stream.rockradio.si:9040/;",
    "https://cadena100-cope-rrcast.flumotion.com/cope/cadena100-low.mp3",
    "https://bbkissfm.kissfmradio.cires21.com:8443/bbkissfm/mp3/icecast.audio?wmsAuthSign=c2VydmVyX3RpbWU9MDYvMDYvMjAyNiAwNjo0Nzo1NSBBTSZoYXNoX3ZhbHVlPXJDQ0daaHhTZzhNMVBjYXlaa0xkZXc9PSZ2YWxpZG1pbnV0ZXM9MTQ0MCZpZD0yNjE1MzQyNg==",
};

void radio_init(void)
{
    int n = sizeof(default_names) / sizeof(default_names[0]);
    for(int i = 0; i < n && i < RADIO_MAX_STATIONS; i++) {
        strncpy(g_radio_presets[i].name, default_names[i], RADIO_NAME_MAX - 1);
        strncpy(g_radio_presets[i].url,  default_urls[i],  RADIO_URL_MAX - 1);
    }
    g_radio_preset_count = n;
    printf("[radio] %d emisoras cargadas\n", n);
}

void radio_stop(void)
{
#ifdef _WIN32
    if(g_proc) {
        TerminateProcess(g_proc, 0);
        CloseHandle(g_proc);
        g_proc = NULL;
    }
    /* Kill any stray ffplay instances we spawned */
    system("taskkill /F /IM ffplay.exe >nul 2>&1");
    g_radio_playing = false;
    g_radio_current = -1;
    printf("[radio] Stop\n");
#else
    if(g_player_pid > 0) {
        kill(g_player_pid, SIGTERM);
        usleep(100000);
        kill(g_player_pid, SIGKILL);
        waitpid(g_player_pid, NULL, WNOHANG);
        g_player_pid = 0;
    }
    system("killall -q mpg123 2>/dev/null");
    g_radio_playing = false;
    g_radio_current = -1;
#endif
}

void radio_play(int index)
{
    if(index < 0 || index >= g_radio_preset_count) return;

    radio_stop();

#ifdef _WIN32
    /* Build ffplay command */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "ffplay.exe -nodisp -autoexit -volume %d \"%s\"",
             g_radio_volume, g_radio_presets[index].url);

    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    BOOL ok = CreateProcess(NULL, cmd, NULL, NULL, FALSE,
                            CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    if(ok) {
        CloseHandle(pi.hThread);
        g_proc = pi.hProcess;
        printf("[radio] Play: %s (pid %lu)\n",
               g_radio_presets[index].name, pi.dwProcessId);
    } else {
        /* Fallback: open URL in default handler */
        printf("[radio] ffplay no encontrado, abriendo en navegador...\n");
        ShellExecute(0, "open", g_radio_presets[index].url, NULL, NULL, SW_SHOW);
    }
    g_radio_current = index;
    g_radio_playing = true;
#else
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "mpg123 -q \"%s\" >/dev/null 2>&1",
             g_radio_presets[index].url);
    g_player_pid = fork();
    if(g_player_pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(1);
    } else if(g_player_pid > 0) {
        printf("[radio] Play: %s (pid %d)\n",
               g_radio_presets[index].name, (int)g_player_pid);
    } else {
        printf("[radio] fork() failed\n");
    }
    g_radio_current = index;
    g_radio_playing = true;
#endif
}

void radio_toggle(int index)
{
    if(index < 0 || index >= g_radio_preset_count) return;

    if(g_radio_playing && g_radio_current == index) {
        radio_stop();
    } else {
        radio_play(index);
    }
}

void radio_set_volume(int vol)
{
    if(vol < 0) vol = 0;
    if(vol > 100) vol = 100;
    g_radio_volume = vol;

#ifdef _WIN32
    /* ffplay volume is set at launch; restart if currently playing */
    if(g_radio_playing && g_radio_current >= 0) {
        int idx = g_radio_current;
        radio_play(idx);
    }
#else
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "amixer set PCM %d%% >/dev/null 2>&1", vol);
    system(cmd);
#endif
    printf("[radio] Volumen: %d%%\n", vol);
}