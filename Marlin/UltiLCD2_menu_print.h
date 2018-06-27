#ifndef ULTI_LCD2_MENU_PRINT_H
#define ULTI_LCD2_MENU_PRINT_H

#include "SdFatConfig.h"

#define LCD_CACHE_COUNT 6

typedef struct
{
    uint8_t     id;
    uint8_t     type;
    char        name[LONG_FILENAME_LENGTH];
} file_info_t;

typedef struct
{
    uint8_t     id;
    uint32_t    time;
    uint32_t    material[EXTRUDERS];
} cache_detail_t;

typedef struct
{
    file_info_t     file[LCD_CACHE_COUNT];
    uint8_t         nr_of_files;
    cache_detail_t  detail;
} lcd_cache_t;

extern lcd_cache_t lcd_cache;

void lcd_menu_print_select();
void lcd_clear_cache();
void doCancelPrint();

extern bool primed;

#endif//ULTI_LCD2_MENU_PRINT_H
