#ifndef ULTI_LCD2_HI_LIB_H
#define ULTI_LCD2_HI_LIB_H

#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_gfx.h"

typedef void (*menuFunc_t)();
typedef char* (*entryNameCallback_t)(uint8_t nr);
typedef void (*entryDetailsCallback_t)(uint8_t nr);

#define ENCODER_TICKS_PER_MENU_ITEM 4
#define ENCODER_NO_SELECTION (ENCODER_TICKS_PER_MENU_ITEM * -11)
#define MENU_ITEM_POS(n)  (ENCODER_TICKS_PER_MENU_ITEM * (n) + ENCODER_TICKS_PER_MENU_ITEM / 2)
#define SELECT_MENU_ITEM(n)  do { lcd_lib_encoder_pos = MENU_ITEM_POS(n); } while(0)
#define SELECTED_MENU_ITEM() (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM)
#define IS_SELECTED(n) ((n) == SELECTED_MENU_ITEM())

void lcd_change_to_menu(menuFunc_t nextMenu, int16_t newEncoderPos = ENCODER_NO_SELECTION);

void lcd_tripple_menu(const char* left, const char* right, const char* bottom);
void lcd_basic_screen();
void lcd_info_screen(menuFunc_t cancelMenu, menuFunc_t callbackOnCancel = NULL, const char* cancelButtonText = NULL);
void lcd_question_screen(menuFunc_t optionAMenu, menuFunc_t callbackOnA, const char* AButtonText, menuFunc_t optionBMenu, menuFunc_t callbackOnB, const char* BButtonText);
void lcd_scroll_menu(const char* menuNameP, int8_t entryCount, entryNameCallback_t entryNameCallback, entryDetailsCallback_t entryDetailsCallback);

void lcd_progressbar(uint8_t progress);

void lcd_menu_edit_setting();

extern const char* lcd_setting_name;
extern const char* lcd_setting_postfix;
extern void* lcd_setting_ptr;
extern uint8_t lcd_setting_type;
extern int16_t lcd_setting_min;
extern int16_t lcd_setting_max;

extern menuFunc_t currentMenu;
extern menuFunc_t previousMenu;
extern int16_t previousEncoderPos;
extern uint8_t minProgress;

#define LCD_EDIT_SETTING(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &_setting; \
            lcd_setting_type = sizeof(_setting); \
            lcd_lib_encoder_pos = _setting; \
            lcd_setting_min = _min; \
            lcd_setting_max = _max; \
        } while(0)
#define LCD_EDIT_SETTING_BYTE_PERCENT(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &_setting; \
            lcd_setting_type = 5; \
            lcd_lib_encoder_pos = int(_setting) * 100 / 255; \
            lcd_setting_min = _min; \
            lcd_setting_max = _max; \
        } while(0)
#define LCD_EDIT_SETTING_FLOAT001(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &_setting; \
            lcd_setting_type = 3; \
            lcd_lib_encoder_pos = (_setting) * 100.0 + 0.5; \
            lcd_setting_min = (_min) * 100; \
            lcd_setting_max = (_max) * 100; \
        } while(0)
#define LCD_EDIT_SETTING_FLOAT100(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR("00" _postfix); \
            lcd_setting_ptr = &(_setting); \
            lcd_setting_type = 7; \
            lcd_lib_encoder_pos = (_setting) / 100 + 0.5; \
            lcd_setting_min = (_min) / 100 + 0.5; \
            lcd_setting_max = (_max) / 100 + 0.5; \
        } while(0)
#define LCD_EDIT_SETTING_FLOAT1(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &(_setting); \
            lcd_setting_type = 8; \
            lcd_lib_encoder_pos = (_setting) + 0.5; \
            lcd_setting_min = (_min) + 0.5; \
            lcd_setting_max = (_max) + 0.5; \
        } while(0)
#define LCD_EDIT_SETTING_SPEED(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &(_setting); \
            lcd_setting_type = 6; \
            lcd_lib_encoder_pos = (_setting) / 60 + 0.5; \
            lcd_setting_min = (_min) / 60 + 0.5; \
            lcd_setting_max = (_max) / 60 + 0.5; \
        } while(0)

extern uint8_t led_glow;
extern uint8_t led_glow_dir;
#define LED_GLOW() lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4)

#endif//ULTI_LCD2_HI_LIB_H
