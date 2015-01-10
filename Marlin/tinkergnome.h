#ifndef ULTI_LCD2_MENU_TINKERGNOME_H
#define ULTI_LCD2_MENU_TINKERGNOME_H

// #include "UltiLCD2_hi_lib.h"

#define EEPROM_UI_MODE_OFFSET 0x401
#define GET_UI_MODE() (eeprom_read_byte((const uint8_t*)EEPROM_UI_MODE_OFFSET))
#define SET_UI_MODE(n) do { eeprom_write_byte((uint8_t*)EEPROM_UI_MODE_OFFSET, n); } while(0)

// display constants
#define LCD_GFX_HEIGHT 64
#define LCD_CHAR_HEIGHT 7
#define LCD_LINE_HEIGHT 9

// menu option states
#define STATE_NONE 0
#define STATE_SELECTED 4
#define STATE_ACTIVE 8
#define STATE_DISABLED 16

// text alignment
#define ALIGN_TOP 1
#define ALIGN_BOTTOM 2
#define ALIGN_VCENTER 4
#define ALIGN_LEFT 8
#define ALIGN_RIGHT 16
#define ALIGN_HCENTER 32
#define ALIGN_CENTER 36

#define MILLISECONDS_PER_SECOND 1000UL
#define MILLISECONDS_PER_MINUTE (MILLISECONDS_PER_SECOND*60UL)

#define LED_DIM_TIME (MILLISECONDS_PER_MINUTE*30UL)		// 30 min
#define DIM_LEVEL 5

// customizations and enhancements (added by Lars June 3,2014)
#define EXTENDED_BEEP 1					// enables extened audio feedback
#define MAX_ENCODER_ACCELERATION 4		// set this to 1 for no acceleration

// UI Mode
#define UI_MODE_STANDARD 0
#define UI_MODE_TINKERGNOME 1

extern uint8_t ui_mode;

struct menuoption_t;

typedef void (*optionCallback_t) (menuoption_t &option, char *detail, uint8_t n);

struct menuoption_t {
    char             *title;
    uint8_t           left;
    uint8_t           top;
    uint8_t           width;
    uint8_t           height;
    optionCallback_t  callbackFunc;
    uint8_t           state;
    uint8_t           textalign;

    bool isActive()  { return (state & STATE_ACTIVE) == STATE_ACTIVE; }
    bool isEnabled() { return !((state & STATE_DISABLED) == STATE_DISABLED); }
};

void tinkergnome_init();
void lcd_menu_maintenance_uimode();
void lcd_menu_print_heatup_tg();
void lcd_menu_printing_tg();
void lcd_lib_draw_heater(uint8_t x, uint8_t y, uint8_t heaterPower);

#endif//ULTI_LCD2_MENU_TINKERGNOME_H

