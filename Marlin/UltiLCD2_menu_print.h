#ifndef ULTI_LCD2_MENU_PRINT_H
#define ULTI_LCD2_MENU_PRINT_H

#include "cardreader.h"

#define LCD_CACHE_COUNT 6
#define LCD_DETAIL_CACHE_SIZE (5+8*EXTRUDERS+8*EXTRUDERS)
#define LCD_CACHE_SIZE (1 + (2 + LONG_FILENAME_LENGTH) * LCD_CACHE_COUNT + LCD_DETAIL_CACHE_SIZE)
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])
#define LCD_DETAIL_CACHE_START ((LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))+1)
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
#define LCD_DETAIL_CACHE_MATERIAL(n) (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*n])
#define LCD_CACHE_ID(n) lcd_cache[(n)]
#define LCD_DETAIL_CACHE_NOZZLE_DIAMETER(n) (*(float*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*EXTRUDERS+4*n])
#define LCD_DETAIL_CACHE_MATERIAL_TYPE(n) ((char*)&lcd_cache[LCD_DETAIL_CACHE_START+5+8*EXTRUDERS+8*n])
extern uint8_t lcd_cache[LCD_CACHE_SIZE];

extern unsigned long predictedTime;

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
void lcd_menu_print_resume();
void lcd_menu_print_heatup();

#endif//ULTI_LCD2_MENU_PRINT_H
