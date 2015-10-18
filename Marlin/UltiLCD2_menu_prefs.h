#ifndef ULTILCD2_MENU_PREFS_H
#define ULTILCD2_MENU_PREFS_H

#include "preferences.h"


extern uint8_t ui_mode;
extern uint16_t lcd_timeout;
extern uint8_t lcd_contrast;
extern uint8_t led_sleep_glow;
extern uint8_t lcd_sleep_contrast;
extern uint8_t expert_flags;
extern float end_of_print_retraction;

void lcd_store_expertflags();

void lcd_menu_sleeptimer();
void lcd_menu_axeslimit();
void lcd_menu_steps();
void lcd_menu_retraction();
void lcd_menu_motorcurrent();
void lcd_menu_maxspeed();
void lcd_menu_acceleration();

#if EXTRUDERS > 1
void init_swap_menu();
void lcd_menu_swap_extruder();
#endif

#endif //ULTILCD2_MENU_PREFS_H
