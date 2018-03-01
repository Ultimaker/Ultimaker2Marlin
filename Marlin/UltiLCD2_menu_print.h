#ifndef ULTI_LCD2_MENU_PRINT_H
#define ULTI_LCD2_MENU_PRINT_H

#include "cardreader.h"

extern uint8_t lcd_cache[];

void lcd_menu_print_select();
void lcd_clear_cache();
void doCancelPrint();

extern bool primed;

#endif//ULTI_LCD2_MENU_PRINT_H
