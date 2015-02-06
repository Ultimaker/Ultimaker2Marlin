#ifndef ULTI_LCD2_MENU_PRINT_H
#define ULTI_LCD2_MENU_PRINT_H

#include "cardreader.h"

#define LCD_CACHE_COUNT 6
#define LCD_DETAIL_CACHE_SIZE (5+4*EXTRUDERS)
#define LCD_CACHE_SIZE (1 + (2 + LONG_FILENAME_LENGTH) * LCD_CACHE_COUNT + LCD_DETAIL_CACHE_SIZE)
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])
#define LCD_DETAIL_CACHE_START ((LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))+1)
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
extern uint8_t lcd_cache[LCD_CACHE_SIZE];
#define LCD_CACHE_ID(n) lcd_cache[(n)]

void lcd_menu_print_select();
void lcd_clear_cache();

void abortPrint();

bool isPauseRequested();
void lcd_print_pause();
void lcd_print_tune();
void lcd_print_abort();

void lcd_menu_print_abort();
void lcd_menu_print_tune();
void lcd_menu_print_ready();
void lcd_menu_print_tune_heatup_nozzle0();
#if EXTRUDERS > 1
void lcd_menu_print_tune_heatup_nozzle1();
#endif
void doStartPrint();
void lcd_change_to_menu_change_material_return();
void lcd_menu_print_pause();

extern bool primed;

#endif//ULTI_LCD2_MENU_PRINT_H
