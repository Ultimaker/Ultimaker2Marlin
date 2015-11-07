#ifndef ULTILCD2_MENU_PREFS_H
#define ULTILCD2_MENU_PREFS_H

#include "preferences.h"

void lcd_store_expertflags();

void lcd_menu_sleeptimer();
void lcd_menu_axeslimit();
void lcd_menu_steps();
void lcd_menu_retraction();
void lcd_menu_motorcurrent();
void lcd_menu_maxspeed();
void lcd_menu_acceleration();
void lcd_menu_heatercheck();

#if EXTRUDERS > 1
void init_swap_menu();
void lcd_menu_swap_extruder();
#endif

#endif //ULTILCD2_MENU_PREFS_H
