#ifndef ULTI_LCD2_HI_LIB_H
#define ULTI_LCD2_HI_LIB_H

#include "preferences.h"
#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_utils.h"

//Arduino IDE compatibility, lacks the eeprom_read_float function
#pragma weak eeprom_read_float
float eeprom_read_float(const float* addr);
#pragma weak eeprom_write_float
void eeprom_write_float(const float* addr, float f);

void lcd_tripple_menu(const char* left, const char* right, const char* bottom);
void lcd_basic_screen();
void lcd_info_screen(menuFunc_t cancelMenu, menuFunc_t callbackOnCancel = NULL, const char* cancelButtonText = NULL);
void lcd_question_screen(menuFunc_t optionAMenu, menuFunc_t callbackOnA, const char* AButtonText, menuFunc_t optionBMenu, menuFunc_t callbackOnB, const char* BButtonText);
// void lcd_scroll_menu(const char* menuNameP, int8_t entryCount, entryNameCallback_t entryNameCallback, entryDetailsCallback_t entryDetailsCallback);
void lcd_scroll_menu(const char* menuNameP, int8_t entryCount, scrollDrawCallback_t entryDrawCallback, entryDetailsCallback_t entryDetailsCallback);

void lcd_progressbar(uint8_t progress);
void lcd_draw_scroll_entry(uint8_t offsetY, char * buffer, uint8_t flags);

void lcd_menu_edit_setting();

bool check_heater_timeout();
bool check_preheat();

#if EXTRUDERS > 1
void lcd_select_nozzle(menuFunc_t callbackOnSelect = 0, menuFunc_t callbackOnAbort = 0);
#endif // EXTRUDERS

extern uint8_t heater_timeout;
extern uint16_t backup_temperature[EXTRUDERS];

extern const char* lcd_setting_name;
extern const char* lcd_setting_postfix;
extern void* lcd_setting_ptr;
extern uint8_t lcd_setting_type;
extern int16_t lcd_setting_min;
extern int16_t lcd_setting_max;
extern int16_t lcd_setting_start_value;

extern menuFunc_t postMenuCheck;
extern uint8_t minProgress;

extern uint16_t lineEntryPos;
extern int8_t   lineEntryWait;

#define LCD_EDIT_SETTING(_setting, _name, _postfix, _min, _max) do { \
    menu.add_menu(menu_t(lcd_menu_edit_setting)); \
    lcd_setting_name = PSTR(_name); \
    lcd_setting_postfix = PSTR(_postfix); \
    lcd_setting_ptr = &_setting; \
    lcd_setting_type = sizeof(_setting); \
    lcd_setting_start_value = lcd_lib_encoder_pos = _setting; \
    lcd_setting_min = _min; \
    lcd_setting_max = _max; \
  } while(0)
#define LCD_EDIT_SETTING_BYTE_PERCENT(_setting, _name, _postfix, _min, _max) do { \
    menu.add_menu(menu_t(lcd_menu_edit_setting)); \
    lcd_setting_name = PSTR(_name); \
    lcd_setting_postfix = PSTR(_postfix); \
    lcd_setting_ptr = &_setting; \
    lcd_setting_type = 5; \
    lcd_setting_start_value = lcd_lib_encoder_pos = int(_setting) * 100 / 255; \
    lcd_setting_min = _min; \
    lcd_setting_max = _max; \
  } while(0)
#define LCD_EDIT_SETTING_FLOAT001(_setting, _name, _postfix, _min, _max) do { \
    menu.add_menu(menu_t(lcd_menu_edit_setting)); \
    lcd_setting_name = PSTR(_name); \
    lcd_setting_postfix = PSTR(_postfix); \
    lcd_setting_ptr = &_setting; \
    lcd_setting_type = 3; \
    lcd_setting_start_value = lcd_lib_encoder_pos = (_setting) * 100.0 + 0.5; \
    lcd_setting_min = (_min) * 100; \
    lcd_setting_max = (_max) * 100; \
  } while(0)
#define LCD_EDIT_SETTING_FLOAT100(_setting, _name, _postfix, _min, _max) do { \
    menu.add_menu(menu_t(lcd_menu_edit_setting)); \
    lcd_setting_name = PSTR(_name); \
    lcd_setting_postfix = PSTR("00" _postfix); \
    lcd_setting_ptr = &(_setting); \
    lcd_setting_type = 7; \
    lcd_setting_start_value = lcd_lib_encoder_pos = (_setting) / 100 + 0.5; \
    lcd_setting_min = (_min) / 100 + 0.5; \
    lcd_setting_max = (_max) / 100 + 0.5; \
  } while(0)
#define LCD_EDIT_SETTING_FLOAT1(_setting, _name, _postfix, _min, _max) do { \
    menu.add_menu(menu_t(lcd_menu_edit_setting)); \
    lcd_setting_name = PSTR(_name); \
    lcd_setting_postfix = PSTR(_postfix); \
    lcd_setting_ptr = &(_setting); \
    lcd_setting_type = 8; \
    lcd_setting_start_value = lcd_lib_encoder_pos = (_setting) + 0.5; \
    lcd_setting_min = (_min) + 0.5; \
    lcd_setting_max = (_max) + 0.5; \
  } while(0)
#define LCD_EDIT_SETTING_SPEED(_setting, _name, _postfix, _min, _max) do { \
    menu.add_menu(menu_t(lcd_menu_edit_setting)); \
    lcd_setting_name = PSTR(_name); \
    lcd_setting_postfix = PSTR(_postfix); \
    lcd_setting_ptr = &(_setting); \
    lcd_setting_type = 6; \
    lcd_setting_start_value = lcd_lib_encoder_pos = (_setting) / 60 + 0.5; \
    lcd_setting_min = (_min) / 60 + 0.5; \
    lcd_setting_max = (_max) / 60 + 0.5; \
  } while(0)

//If we have a heated bed, then the heated bed menu entries have a size of 1, else they have a size of 0.
#if TEMP_SENSOR_BED != 0
#define BED_MENU_OFFSET 1
#else
#define BED_MENU_OFFSET 0
#endif

#define BOTTOM_MENU_YPOS 54

#define EQUALF(f1, f2) (fabs(f2-f1)<=0.01f)
#define NEQUALF(f1, f2) (fabs(f2-f1)>0.01f)

#endif//ULTI_LCD2_HI_LIB_H
