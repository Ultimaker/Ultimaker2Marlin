#ifndef ULTI_LCD2_MENU_PRINT_H
#define ULTI_LCD2_MENU_PRINT_H

#include "Configuration.h"
#include "SdFatConfig.h"
#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_menu_material.h"

/*
#include "cardreader.h"

#define LCD_CACHE_COUNT 6
#define LCD_CACHE_TEXT_SIZE_SHORT  20
#define LCD_CACHE_TEXT_SIZE_FULL   (LONG_FILENAME_LENGTH-1)
#define LCD_CACHE_TEXT_SIZE_REMAIN (LCD_CACHE_TEXT_SIZE_FULL - LCD_CACHE_TEXT_SIZE_SHORT)
#define LCD_CACHE_FILE_SIZE ((2 + LCD_CACHE_TEXT_SIZE_SHORT) * LCD_CACHE_COUNT + 1)
#define LCD_DETAIL_CACHE_SIZE (5 + 8*EXTRUDERS + 8*EXTRUDERS)
//
#define LCD_CACHE_ID(n) lcd_cache[(n)]
#define LCD_CACHE_TYPE(n) lcd_cache[LCD_CACHE_COUNT + (n)]
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LCD_CACHE_TEXT_SIZE_SHORT])
#define LCD_CACHE_NR_OF_FILES() lcd_cache[(LCD_CACHE_COUNT*(LCD_CACHE_TEXT_SIZE_SHORT+2))]
//
#define LCD_CACHE_REMAIN_COUNT 1
#define LCD_CACHE_REMAIN_START (LCD_CACHE_FILE_SIZE + LCD_DETAIL_CACHE_SIZE)
#define LCD_CACHE_REMAIN_SIZE ((1 + LCD_CACHE_TEXT_SIZE_REMAIN) * LCD_CACHE_REMAIN_COUNT)
#define LCD_CACHE_REMAIN_ID(n) lcd_cache[LCD_CACHE_REMAIN_START + (n)]
#define LCD_CACHE_REMAIN_FILENAME(n) ((char*)&lcd_cache[LCD_CACHE_REMAIN_START + LCD_CACHE_REMAIN_COUNT + (n) * LCD_CACHE_TEXT_SIZE_REMAIN])
//
#define LCD_DETAIL_CACHE_START (LCD_CACHE_FILE_SIZE)
#define LCD_DETAIL_CACHE_ID() lcd_cache[LCD_DETAIL_CACHE_START]
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
#define LCD_DETAIL_CACHE_MATERIAL(n) (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*n])
#define LCD_DETAIL_CACHE_NOZZLE_DIAMETER(n) (*(float*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*EXTRUDERS+4*n])
#define LCD_DETAIL_CACHE_MATERIAL_TYPE(n) ((char*)&lcd_cache[LCD_DETAIL_CACHE_START+5+8*EXTRUDERS+8*n])
//
#define LCD_CACHE_SIZE (LCD_CACHE_FILE_SIZE + LCD_DETAIL_CACHE_SIZE + LCD_CACHE_REMAIN_SIZE)
extern uint8_t lcd_cache[LCD_CACHE_SIZE];
*/

#define LCD_CACHE_COUNT 6
#define LCD_CACHE_TEXT_SIZE_REMAIN (LONG_FILENAME_LENGTH - LINE_ENTRY_TEXT_LENGTH)

typedef struct
{
    uint8_t     id;
    uint8_t     type;
    char        name[LINE_ENTRY_TEXT_LENGTH+1];
} file_info_t;

typedef struct
{
    uint8_t     id;
    uint32_t    time;
    uint32_t    material[EXTRUDERS];
    float       nozzle_diameter[EXTRUDERS];
    char        material_type[EXTRUDERS][MATERIAL_NAME_SIZE + 1];
    char        remain_filename[LCD_CACHE_TEXT_SIZE_REMAIN+1];
} cache_detail_t;

typedef struct
{
    file_info_t     file[LCD_CACHE_COUNT];
    uint8_t         nr_of_files;
    cache_detail_t  detail;
} lcd_cache_t;

typedef union
{
    lcd_cache_t  lcd;
    uint16_t     _uint16[32];
    int16_t      _int16[32];
    float        _float[16];
    uint8_t      _byte[64];
    bool         _bool[64];
} lcd_cache_union;

extern lcd_cache_union cache;


extern unsigned long predictedTime;

void lcd_menu_print_select();
void lcd_clear_cache();

void abortPrint(bool bQuickstop);

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

#define LCD_CACHE_NR_OF_FILES               (cache.lcd.nr_of_files)
#define LCD_CACHE_ID(n)                     (cache.lcd.file[n].id)
#define LCD_CACHE_TYPE(n)                   (cache.lcd.file[n].type)
#define LCD_CACHE_FILENAME(n)               (cache.lcd.file[n].name)
#define LCD_DETAIL_CACHE_ID                 (cache.lcd.detail.id)
#define LCD_DETAIL_CACHE_TIME               (cache.lcd.detail.time)
#define LCD_DETAIL_CACHE_MATERIAL(n)        (cache.lcd.detail.material[n])
#define LCD_DETAIL_CACHE_NOZZLE_DIAMETER(n) (cache.lcd.detail.nozzle_diameter[n])
#define LCD_DETAIL_CACHE_MATERIAL_TYPE(n)   (cache.lcd.detail.material_type[n])
#define LCD_DETAIL_CACHE_REMAIN_FILENAME    (cache.lcd.detail.remain_filename)

#endif//ULTI_LCD2_MENU_PRINT_H
