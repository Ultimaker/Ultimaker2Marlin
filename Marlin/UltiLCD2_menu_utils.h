#ifndef ULTI_LCD2_MENU_UTILS_H
#define ULTI_LCD2_MENU_UTILS_H

#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_print.h"

// --------------------------------------------------------------------------
// menu stack handling
// --------------------------------------------------------------------------

struct menu_t {
    menuFunc_t  menuFunc;
    int16_t     encoderPos;
    uint8_t     max_encoder_acceleration;

    // menu_t() : menuFunc(NULL), encoderPos(0) {}
    menu_t(menuFunc_t func, int16_t pos, uint8_t accel) : menuFunc(func), encoderPos(pos), max_encoder_acceleration(accel) {}
};

void lcd_add_menu(menuFunc_t nextMenu, int16_t newEncoderPos);
void lcd_replace_menu(menuFunc_t nextMenu);
void lcd_replace_menu(menuFunc_t nextMenu, int16_t newEncoderPos);
void lcd_change_to_menu(menuFunc_t nextMenu, int16_t newEncoderPos = ENCODER_NO_SELECTION, int16_t oldEncoderPos = ENCODER_NO_SELECTION);
void lcd_change_to_previous_menu();
void lcd_remove_menu();

menu_t & currentMenu();

// --------------------------------------------------------------------------
// menu item processing
// --------------------------------------------------------------------------

// menu item flags
#define MENU_NORMAL 0
#define MENU_INPLACE_EDIT 1

// text alignment
#define ALIGN_TOP 1
#define ALIGN_BOTTOM 2
#define ALIGN_VCENTER 4
#define ALIGN_LEFT 8
#define ALIGN_RIGHT 16
#define ALIGN_HCENTER 32
#define ALIGN_CENTER 36

typedef void (*menuActionCallback_t) ();

struct menuitem_t {
    char                 *title;
    char                 *details;
    uint8_t               left;
    uint8_t               top;
    uint8_t               width;
    uint8_t               height;
    menuActionCallback_t  initFunc;
    menuActionCallback_t  callbackFunc;
    uint8_t               textalign;
    uint8_t               flags;
    uint8_t               max_encoder_acceleration;

    menuitem_t() : title(LCD_CACHE_FILENAME(0))
                   , details(LCD_CACHE_FILENAME(1))
                   , left(0)
                   , top(0)
                   , width(30)
                   , height(LCD_CHAR_HEIGHT)
                   , initFunc(0)
                   , callbackFunc(0)
                   , textalign(0)
                   , flags(MENU_NORMAL)
                   , max_encoder_acceleration(1)
                     {}
};

typedef const menuitem_t & (*menuItemCallback_t) (uint8_t nr, menuitem_t &opt);

extern int8_t selectedMenuItem;
extern menuActionCallback_t currentMenuFunc;

FORCE_INLINE bool isActive(uint8_t nr) { return ((selectedMenuItem == nr) && currentMenuFunc); }
FORCE_INLINE bool isSelected(uint8_t nr) { return (selectedMenuItem == nr); }

void lcd_reset_menu();
const menuitem_t & lcd_draw_menu_item(menuItemCallback_t getMenuItem, uint8_t nr);
void lcd_process_menu(menuItemCallback_t getMenuItem, uint8_t len);

#endif
