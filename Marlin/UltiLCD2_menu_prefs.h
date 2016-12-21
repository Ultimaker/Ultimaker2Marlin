#ifndef ULTILCD2_MENU_PREFS_H
#define ULTILCD2_MENU_PREFS_H

#include "preferences.h"

void lcd_menu_sleeptimer();
void lcd_menu_axeslimit();
void lcd_menu_steps();
void lcd_menu_axisdirection();
void lcd_menu_retraction();
void lcd_menu_motorcurrent();
void lcd_menu_maxspeed();
void lcd_menu_acceleration();
void lcd_menu_heatercheck();

#if EXTRUDERS > 1
void init_swap_menu();
void lcd_menu_swap_extruder();
#endif

#if (TEMP_SENSOR_BED != 0) || (EXTRUDERS > 1)
void lcd_menu_tempcontrol();
#else
void init_tempcontrol_e1();
void lcd_menu_tempcontrol_e1();
#endif

#endif //ULTILCD2_MENU_PREFS_H
