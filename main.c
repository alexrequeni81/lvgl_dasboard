#include "lvgl/lvgl.h"

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
#endif

#ifdef __linux__
#include "lvgl/src/drivers/display/fb/lv_linux_fbdev.h"
#include "lvgl/src/drivers/evdev/lv_evdev.h"
#endif

#include "src/ui/ui.h"
#include "src/time_manager.h"
#include "src/radio_manager.h"
#include "src/agenda_manager.h"

int main(int argc, char *argv[])
{
    /* Initialize LVGL */
    lv_init();

#ifdef _WIN32
    /* Windows: SDL2 simulator */
    lv_display_t * disp = lv_sdl_window_create(800, 600);
    lv_sdl_window_set_title(disp, "LVGL Simulator - Atom N270 Dashboard");
    lv_sdl_mouse_create();
    lv_sdl_keyboard_create();
#elif __linux__
    /* Linux: framebuffer + evdev touch */
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    /* Touchscreen input */
    lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event1");
#endif

    /* Initialize Time Manager (load settings) and Agenda */
    time_manager_init();
    agenda_init();
    radio_init();

    /* Initialize UI modularly */
    ui_init();

    /* Main loop */
    while(1) {
        lv_timer_handler();
        sleep_ms(5);
    }

    return 0;
}
