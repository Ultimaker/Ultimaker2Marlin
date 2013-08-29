#ifndef ULTI_LCD2_H
#define ULTI_LCD2_H

#include "Configuration.h"

#ifdef ENABLE_ULTILCD2

void lcd_init();
void lcd_update();
FORCE_INLINE void lcd_setstatus(const char* message) {}
void lcd_buttons_update();
FORCE_INLINE void lcd_reset_alert_level() {}
FORCE_INLINE void lcd_buzz(long duration,uint16_t freq) {}

#define LCD_MESSAGEPGM(x) 
#define LCD_ALERTMESSAGEPGM(x) 

void lcd_menu_main();

#endif

#endif//ULTI_LCD2_H
