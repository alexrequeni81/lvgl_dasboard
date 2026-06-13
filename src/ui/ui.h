#ifndef UI_H
#define UI_H

#include "lvgl/lvgl.h"

/* Visual Design Tokens (Colors) */
#define UI_COLOR_BG            lv_color_hex(0x0F172A)  /* slate-900 */
#define UI_COLOR_CARD          lv_color_hex(0x1E293B)  /* slate-800 */
#define UI_COLOR_CARD_BORDER   lv_color_hex(0x334155)  /* slate-700 */
#define UI_COLOR_ACCENT        lv_color_hex(0x06B6D4)  /* cyan-500 */
#define UI_COLOR_ALERT         lv_color_hex(0xF59E0B)  /* amber-500 */
#define UI_COLOR_TEXT_MAIN     lv_color_hex(0xF8FAFC)  /* slate-50 */
#define UI_COLOR_TEXT_MUTED    lv_color_hex(0x94A3B8)  /* slate-400 */

/* Global UI Functions */
void ui_init(void);

/* Helper UI Components */
lv_obj_t * ui_create_card(lv_obj_t * parent, int32_t w, int32_t h);
void ui_attach_keyboard(lv_obj_t * ta, lv_keyboard_mode_t mode);

#endif /* UI_H */
