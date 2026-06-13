#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* ── Platform-specific settings ── */
#ifdef __linux__
  /* Linux: framebuffer + evdev */
  #define LV_USE_SDL 0
  #define LV_USE_LINUX_FBDEV 1
  #define LV_USE_EVDEV 1
  #define LV_COLOR_DEPTH 32
#else
  /* Windows: SDL2 simulator */
  #define LV_USE_SDL 1
  #define LV_SDL_INCLUDE_PATH <SDL2/SDL.h>
  #define LV_SDL_RENDER_MODE LV_DISPLAY_RENDER_MODE_DIRECT
  #define LV_SDL_SINGLE_THREAD 1
  #define LV_COLOR_DEPTH 16
#endif
#define LV_COLOR_16_SWAP 0

/* ── Font settings ── */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 1

/* ── Features ── */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

#endif /*LV_CONF_H*/
