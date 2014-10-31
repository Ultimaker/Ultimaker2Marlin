#ifndef ULTI_LCD2_MENU_PRINT_H
#define ULTI_LCD2_MENU_PRINT_H

#include "cardreader.h"

#define LCD_CACHE_COUNT 6
#define LCD_DETAIL_CACHE_SIZE (5+4*EXTRUDERS)
#define LCD_CACHE_SIZE (1 + (2 + LONG_FILENAME_LENGTH) * LCD_CACHE_COUNT + LCD_DETAIL_CACHE_SIZE)
extern uint8_t lcd_cache[LCD_CACHE_SIZE];

void lcd_menu_print_select();
void lcd_clear_cache();
void doCancelPrint();

extern bool primed;

#endif//ULTI_LCD2_MENU_PRINT_H
