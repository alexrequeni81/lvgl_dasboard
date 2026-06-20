#include <stdio.h>
#include "lvgl/lvgl.h"

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

#ifdef __linux__
#include <time.h>
#include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#include "lvgl/src/drivers/evdev/lv_evdev.h"
#endif

#include "src/ui/ui.h"
#include "src/ui/calendar_tab.h"
#include "src/time_manager.h"
#include "src/radio_manager.h"
#include "src/agenda_manager.h"
#include "src/weather.h"

static void test_edit_timer_cb(lv_timer_t * t)
{
    (void)t;
    printf("[test] Starting stress test (3 iterations)...\n");
    fflush(stdout);
    calendar_tab_test_stress(3);
    lv_timer_delete(t);
}

int main(int argc, char *argv[])
{
    /* Initialize LVGL */
    lv_init();

#ifdef _WIN32
    /* Windows: SDL2 simulator */
    lv_display_t * disp = lv_sdl_window_create(800, 600);
    lv_sdl_window_set_title(disp, "LVGL Simulator - Atom N270 Dashboard");
    lv_sdl_mouse_create();
    // lv_sdl_keyboard_create(); // temp: disabled to test crash interaction
#elif __linux__
    /* Linux: framebuffer + evdev touch */
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    /* Sync system clock via NTP (non-blocking) */
    system("ntpclient -h pool.ntp.org -s >/dev/null 2>&1 &");

    /* Timezone default, will be overridden by geo-IP if available */
    setenv("TZ", "Europe/Madrid", 0);
    tzset();

    /* Touchscreen input */
    lv_indev_t * touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");
    lv_evdev_set_calibration(touch, 88, 978, 970, 70);
#endif

    /* Initialize Time Manager (load settings) and Agenda */
    time_manager_init();
    agenda_init();
    radio_init();

    /* Auto-detect location via geo-IP (sets lat/lon/city/timezone) */
    weather_auto_detect();

    /* Initialize UI modularly */
    ui_init();

    /* Programmatic test: click Editar after UI is ready */
    //lv_timer_create(test_edit_timer_cb, 500, NULL); // temp: disabled for manual testing

    /* Main loop */
    while(1) {
        lv_timer_handler();
        sleep_ms(5);
    }

    return 0;
}
