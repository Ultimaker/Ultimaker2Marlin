#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
//#include "Marlin.h"
//#include "cardreader.h"//This code uses the card.longFilename as buffer to store data, to save memory.
#include "temperature.h"
//#include "ConfigurationStore.h"
#include "UltiLCD2.h"
#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_maintenance.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_main.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_utils.h"
#include "tinkergnome.h"

#define BREAKOUT_PADDLE_WIDTH 21
//Use the lcd_cache memory to store breakout data, so we do not waste memory.
#define ball_x (*(int16_t*)&lcd_cache[3*5])
#define ball_y (*(int16_t*)&lcd_cache[3*5+2])
#define ball_dx (*(int16_t*)&lcd_cache[3*5+4])
#define ball_dy (*(int16_t*)&lcd_cache[3*5+6])
static void lcd_menu_breakout()
{
    if (lcd_lib_encoder_pos == ENCODER_NO_SELECTION)
    {
        lcd_lib_encoder_pos = (128 - BREAKOUT_PADDLE_WIDTH) / 2 / 2;
        for(uint8_t y=0; y<3;y++)
            for(uint8_t x=0; x<5;x++)
                lcd_cache[x+y*5] = 3;
        ball_x = 0;
        ball_y = 57 << 8;
        ball_dx = 0;
        ball_dy = 0;
    }

    if (lcd_lib_encoder_pos < 0) lcd_lib_encoder_pos = 0;
    if (lcd_lib_encoder_pos * 2 > 128 - BREAKOUT_PADDLE_WIDTH - 1) lcd_lib_encoder_pos = (128 - BREAKOUT_PADDLE_WIDTH - 1) / 2;
    ball_x += ball_dx;
    ball_y += ball_dy;
    if (ball_x < 1 << 8) ball_dx = abs(ball_dx);
    if (ball_x > 124 << 8) ball_dx = -abs(ball_dx);
    if (ball_y < (1 << 8)) ball_dy = abs(ball_dy);
    if (ball_y < (3 * 10) << 8)
    {
        uint8_t x = (ball_x >> 8) / 25;
        uint8_t y = (ball_y >> 8) / 10;
        if (lcd_cache[x+y*5])
        {
            lcd_cache[x+y*5]--;
            ball_dy = abs(ball_dy);
            for(y=0; y<3;y++)
            {
                for(x=0; x<5;x++)
                    if (lcd_cache[x+y*5])
                        break;
                if (x != 5)
                    break;
            }
            if (x==5 && y==3)
            {
                for(y=0; y<3;y++)
                    for(x=0; x<5;x++)
                        lcd_cache[x+y*5] = 3;
            }
        }
    }
    if (ball_y > (58 << 8))
    {
        if (ball_x < (lcd_lib_encoder_pos * 2 - 2) << 8 || ball_x > (lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH) << 8)
            menu.return_to_previous();
        ball_dx += (ball_x - ((lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH / 2) * 256)) / 64;
        ball_dy = -512 + abs(ball_dx);
    }
    if (ball_dy == 0)
    {
        ball_y = 57 << 8;
        ball_x = (lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH / 2) << 8;
        if (lcd_lib_button_pressed)
        {
            ball_dx = -256 + lcd_lib_encoder_pos * 8;
            ball_dy = -512 + abs(ball_dx);
        }
    }

    lcd_lib_clear();

    for(uint8_t y=0; y<3;y++)
        for(uint8_t x=0; x<5;x++)
        {
            if (lcd_cache[x+y*5])
                lcd_lib_draw_box(3 + x*25, 2 + y * 10, 23 + x*25, 10 + y * 10);
            if (lcd_cache[x+y*5] == 2)
                lcd_lib_draw_shade(4 + x*25, 3 + y * 10, 22 + x*25, 9 + y * 10);
            if (lcd_cache[x+y*5] == 3)
                lcd_lib_set(4 + x*25, 3 + y * 10, 22 + x*25, 9 + y * 10);
        }

    lcd_lib_draw_box(ball_x >> 8, ball_y >> 8, (ball_x >> 8) + 2, (ball_y >> 8) + 2);
    lcd_lib_draw_box(lcd_lib_encoder_pos * 2, 60, lcd_lib_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH, 63);
    lcd_lib_update_screen();
}

static void lcd_cooldown()
{
    for(uint8_t n=0; n<EXTRUDERS; n++)
        setTargetHotend(0, n);

#if TEMP_SENSOR_BED != 0
    setTargetBed(0);
#endif
}

// return heatup menu option
static const menu_t & get_preheat_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_tune);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_abort);
    }
    else if (nr == menu_index++)
    {
        // temp nozzle 1
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == menu_index++)
    {
        // temp nozzle 2
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle1);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == menu_index++)
    {
        // temp buildplate
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_bed);
    }
#endif
    return opt;
}

static void drawPreheatSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32];

    if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT*2
                           , BOTTOM_MENU_YPOS
                           , 48
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Tune menu"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_clear_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_draw_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
    }
    else if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, standbyGfx);
            lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 5, PSTR("Abort print"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT*2
                                , BOTTOM_MENU_YPOS
                                , 48
                                , LCD_CHAR_HEIGHT
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
    }
    else if (nr == index++)
    {
        // temp nozzle 1
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
#if EXTRUDERS < 2
            strcpy_P(buffer, PSTR("Nozzle "));
#else
            strcpy_P(buffer, PSTR("Nozzle(1) "));
#endif
            int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer+strlen(buffer), PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        int_to_string(target_temperature[0], buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                          , 51-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                          , 24
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // temp nozzle 2
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            strcpy_P(buffer, PSTR("Nozzle(2) "));
            int_to_string(target_temperature[1], int_to_string(dsp_temperature[1], buffer+strlen(buffer), PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        int_to_string(target_temperature[1], buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                              , 24
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // temp buildplate
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            strcpy_P(buffer, PSTR("Buildplate "));
            int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, buffer+strlen(buffer), PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        int_to_string(target_temperature_bed, buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42
                              , 24
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
}

static void lcd_main_print()
{
    lcd_clear_cache();
    card.release();
    menu.add_menu(menu_t(lcd_menu_print_select, SCROLL_MENU_ITEM_POS(0)));
}

static void lcd_main_material()
{
    menu.add_menu(menu_t(lcd_menu_material));
}

static void lcd_main_maintenance()
{
    if (ui_mode & UI_MODE_EXPERT)
    {
        menu.add_menu(menu_t(lcd_menu_maintenance_advanced));
    }else{
        menu.add_menu(menu_t(lcd_menu_maintenance));
    }
}

static const menu_t & get_main_standard(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_print);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_material);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_maintenance);
    }
    return opt;
}

void drawMainStandard(uint8_t nr, uint8_t &flags)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
                                , LCD_LINE_HEIGHT
                                , 52
                                , LCD_LINE_HEIGHT*4
                                , PSTR("PRINT")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                                , LCD_LINE_HEIGHT
                                , 52
                                , LCD_LINE_HEIGHT*4
                                , PSTR("MATERIAL")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 - 7*LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 14*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("MAINTENANCE")
                                , ALIGN_CENTER
                                , flags);
    }
}


static const menu_t & get_main_expert(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_print);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_material);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_maintenance);
    }
    return opt;
}

void drawMainExpert(uint8_t nr, uint8_t &flags)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
                                , LCD_LINE_HEIGHT
                                , 52
                                , LCD_LINE_HEIGHT*4
                                , PSTR("PRINT")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                                , LCD_LINE_HEIGHT
                                , 52
                                , LCD_LINE_HEIGHT*4
                                , PSTR("MATERIAL")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 - 7*LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 14*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("ADVANCED")
                                , ALIGN_CENTER
                                , flags);
    }
}

void lcd_menu_main()
{
    lcd_lib_clear();
    lcd_lib_draw_vline(64, 5, 46);
    lcd_lib_draw_hline(3, 124, 50);

    if (lcd_lib_button_down && !menu.isSubmenuSelected())
    {
        led_glow_dir = 0;
        if (led_glow > 200)
            menu.add_menu(menu_t(lcd_menu_breakout));
    }
    else
    {
        if (ui_mode & UI_MODE_EXPERT)
        {
            menu.process_submenu(get_main_expert, 3);
        }
        else
        {
            menu.process_submenu(get_main_standard, 3);
        }
    }
    if (ui_mode & UI_MODE_EXPERT)
    {
        for (uint8_t index=0; index<3; ++index)
        {
            menu.drawSubMenu(drawMainExpert, index);
        }
    }
    else
    {
        for (uint8_t index=0; index<3; ++index)
        {
            menu.drawSubMenu(drawMainStandard, index);
        }
    }
    lcd_lib_update_screen();
}


#endif//ENABLE_ULTILCD2
