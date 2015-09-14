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
#include "machinesettings.h"

#define PREHEAT_FLAG(n) (lcd_cache[2*LCD_CACHE_COUNT+n])

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
    for(uint8_t n=0; n<EXTRUDERS; ++n)
    {
        PREHEAT_FLAG(n+1) = 0;
        setTargetHotend(0, n);
    }

#if TEMP_SENSOR_BED != 0
    setTargetBed(0);
    PREHEAT_FLAG(0) = 0;
#endif
    printing_state = PRINT_STATE_NORMAL;
    // menu.return_to_previous();
}

#if EXTRUDERS > 1
static void lcd_select_nozzle()
{
    lcd_tripple_menu(PSTR("PRIMARY|NOZZLE"), PSTR("SECONDARY|NOZZLE"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        uint8_t index(SELECTED_MAIN_MENU_ITEM());
        if (index < 2)
        {
            active_extruder = index;
            menu.return_to_previous();
        }
        else
        {
            menu.return_to_previous();
            menu.return_to_previous();
        }
    }

    lcd_lib_update_screen();
}
#endif

static void start_material_change()
{
    minProgress = 0;
    char buffer[32] = {0};
    enquecommand_P(PSTR("G28 X0 Y0"));
    sprintf_P(buffer, PSTR("G1 F%i X%i Y%i"), int(homing_feedrate[0]), int(AXIS_CENTER_POS(X_AXIS)), int(min_pos[Y_AXIS])+5);
    enquecommand(buffer);
    menu.replace_menu(menu_t(lcd_menu_material_main_return));
    menu.add_menu(menu_t(lcd_menu_change_material_preheat));
}

static void init_material_change()
{
#if EXTRUDERS > 1
    menu.add_menu(menu_t(start_material_change));
    menu.add_menu(menu_t(lcd_select_nozzle, MAIN_MENU_ITEM_POS(0)));
#else
    menu.add_menu(menu_t(start_material_change));
#endif
}

static void start_material_move()
{
    set_extrude_min_temp(0);
    enquecommand_P(PSTR("G92 E0"));
    if (current_temperature[active_extruder] < (material[active_extruder].temperature / 2))
    {
        target_temperature[active_extruder] = material[active_extruder].temperature;
    }
    menu.replace_menu(menu_t(lcd_menu_expert_extrude, MAIN_MENU_ITEM_POS(5)));
    menu.set_selection(5);
}

static void init_material_move()
{
#if EXTRUDERS > 1
    menu.add_menu(menu_t(start_material_move));
    menu.add_menu(menu_t(lcd_select_nozzle, MAIN_MENU_ITEM_POS(0)));
#else
    menu.add_menu(menu_t(start_material_move));
#endif
}

static void init_material_settings()
{
#if EXTRUDERS > 1
    menu.add_menu(menu_t(lcd_menu_material_select, SCROLL_MENU_ITEM_POS(0)));
    menu.add_menu(menu_t(lcd_select_nozzle, MAIN_MENU_ITEM_POS(0)));
#else
    menu.add_menu(menu_t(lcd_menu_material_select, SCROLL_MENU_ITEM_POS(0)));
#endif
}

static void lcd_main_print()
{
    lcd_clear_cache();
    card.release();
    menu.add_menu(menu_t(lcd_menu_print_select, SCROLL_MENU_ITEM_POS(0)));
}

static void lcd_toggle_preheat_nozzle0()
{
    PREHEAT_FLAG(1) = !PREHEAT_FLAG(1);
    if (!PREHEAT_FLAG(1))
    {
        setTargetHotend(0, 0);
    }
}

static void lcd_preheat_tune_nozzle0() { lcd_tune_value(target_temperature[0], 0, get_maxtemp(0) - 15); PREHEAT_FLAG(1) = (target_temperature[0]>0); }

#if EXTRUDERS > 1
static void lcd_toggle_preheat_nozzle1()
{
    PREHEAT_FLAG(2) = !PREHEAT_FLAG(2);
    if (PREHEAT_FLAG(2))
    {

    }
    else
    {
        setTargetHotend(0, 1);
    }
}

static void lcd_preheat_tune_nozzle1() { lcd_tune_value(target_temperature[1], 0, get_maxtemp(1) - 15); PREHEAT_FLAG(2) = (target_temperature[0]>0); }
#endif

#if TEMP_SENSOR_BED != 0
static void lcd_toggle_preheat_bed()
{
    PREHEAT_FLAG(0) = !PREHEAT_FLAG(0);
    if (PREHEAT_FLAG(0))
    {
        target_temperature_bed = 0;
        for(uint8_t e=0; e<EXTRUDERS; e++)
        {
            target_temperature_bed = max(target_temperature_bed, material[e].bed_temperature);
        }
    }
    else
    {
        target_temperature_bed = 0;
    }
}

static void lcd_preheat_tune_bed() { lcd_tune_value(target_temperature_bed, 0, BED_MAXTEMP - 15); PREHEAT_FLAG(0) = (target_temperature_bed>0); }
#endif

// return preheat menu option
static const menu_t & get_preheat_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_print);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_cooldown);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == menu_index++)
    {
        // toggle heatup nozzle 1
        opt.setData(MENU_NORMAL, lcd_toggle_preheat_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == menu_index++)
    {
        // toggle heatup nozzle 2
        opt.setData(MENU_NORMAL, lcd_toggle_preheat_nozzle1);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == menu_index++)
    {
        // toggle heatup buildplate
        opt.setData(MENU_NORMAL, lcd_toggle_preheat_bed);
    }
#endif
    else if (nr == menu_index++)
    {
        // temp nozzle 1
        opt.setData(MENU_INPLACE_EDIT, lcd_preheat_tune_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == menu_index++)
    {
        // temp nozzle 2
        opt.setData(MENU_INPLACE_EDIT, lcd_preheat_tune_nozzle1);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == menu_index++)
    {
        // temp buildplate
        opt.setData(MENU_INPLACE_EDIT, lcd_preheat_tune_bed);
    }
#endif
    return opt;
}

static void drawPreheatSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};

    if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT*2
                           , BOTTOM_MENU_YPOS
                           , 35
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Start print"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("PRINT"));
            // lcd_lib_clear_gfx(1*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, startGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("PRINT"));
            // lcd_lib_draw_gfx(1*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, startGfx);
        }
    }
    else if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            // lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, backGfx);
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, 5, PSTR("Cool down all"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(49
                           , BOTTOM_MENU_YPOS
                           , 35
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(49+LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("COOL"));
        }
        else
        {
            lcd_lib_draw_stringP(49+LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("COOL"));
        }
    }
    else if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            // lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, backGfx);
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, 5, PSTR("Return to main menu"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(89
                           , BOTTOM_MENU_YPOS
                           , 35
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(89 + LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            // lcd_lib_clear_gfx(86, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(89 + LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            // lcd_lib_draw_gfx(86, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // flag nozzle 1
        if (flags & MENU_SELECTED)
        {
#if EXTRUDERS < 2
            strcpy_P(buffer, PSTR("Nozzle: "));
            strcpy_P(buffer+8, PREHEAT_FLAG(1) ? PSTR("on") : PSTR("off"));
#else
            strcpy_P(buffer, PSTR("Nozzle(1): "));
            strcpy_P(buffer+11, PREHEAT_FLAG(1) ? PSTR("on") : PSTR("off"));
#endif
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT
                           , 47-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 47-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT), PREHEAT_FLAG(1)?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 47-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT), PREHEAT_FLAG(1)?checkbox_on:checkbox_off);
        }
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // flag nozzle 1
        if (flags & MENU_SELECTED)
        {
            strcpy_P(buffer, PSTR("Nozzle(2): "));
            strcpy_P(buffer+11, PREHEAT_FLAG(2) ? PSTR("on") : PSTR("off"));

            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT
                           , 39-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 39-(BED_MENU_OFFSET*LCD_LINE_HEIGHT), PREHEAT_FLAG(2)?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 39-(BED_MENU_OFFSET*LCD_LINE_HEIGHT), PREHEAT_FLAG(2)?checkbox_on:checkbox_off);
        }
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // flag nozzle 1
        if (flags & MENU_SELECTED)
        {
            strcpy_P(buffer, PSTR("Buildplate: "));
            strcpy_P(buffer+12, PREHEAT_FLAG(0) ? PSTR("on") : PSTR("off"));

            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT
                           , 40
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 40, PREHEAT_FLAG(0)?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 40, PREHEAT_FLAG(0)?checkbox_on:checkbox_off);
        }
    }
#endif
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
                          , 47-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
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
                              , 39-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
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
                              , 40
                              , 24
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
}

static void lcd_main_preheat()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    char buffer[32] = {0};
#if TEMP_SENSOR_BED != 0
    if ((!PREHEAT_FLAG(0)) | (current_temperature_bed > target_temperature_bed - 10))
    {
#endif
        // set preheat nozzle temperature
        for(uint8_t e=0; e<EXTRUDERS; ++e)
        {
            if ((PREHEAT_FLAG(e+1)) & (target_temperature[e] < 1))
            {
                target_temperature[e] = (material[e].temperature /5*4);
                target_temperature[e] -= target_temperature[e] % 10;
            }
        }
#if TEMP_SENSOR_BED != 0
    }
#endif

    // bed temperature
    uint8_t ypos = 40;
#if TEMP_SENSOR_BED != 0
    // bed temperature
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature_bed, buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-71, ypos, bedTempGfx);
    // lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(-1));
    ypos -= LCD_LINE_HEIGHT+1;
#endif // TEMP_SENSOR_BED
#if EXTRUDERS > 1
    // temperature second extruder
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[1], buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(1));
    ypos -= LCD_LINE_HEIGHT+1;
#endif // EXTRUDERS
    // temperature first extruder
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[0], buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(0));

    menu.process_submenu(get_preheat_menuoption, 2*EXTRUDERS + 2*BED_MENU_OFFSET + 3);

    uint8_t flags = 0;
    for (uint8_t index=0; index<2*EXTRUDERS + 2*BED_MENU_OFFSET + 3; ++index)
    {
        menu.drawSubMenu(drawPreheatSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Preheat"));
    }

    lcd_lib_update_screen();
}

static void init_preheat()
{
    // init preheat Temperature settings
#if TEMP_SENSOR_BED != 0
    target_temperature_bed = 0;
#endif
    for(uint8_t e=0; e<EXTRUDERS; ++e)
    {
        PREHEAT_FLAG(e+1) = 0;
        target_temperature[e] = 0;//material[e].temperature;
#if TEMP_SENSOR_BED != 0
        target_temperature_bed = max(target_temperature_bed, material[e].bed_temperature);
        if (target_temperature_bed > 0)
        {
            PREHEAT_FLAG(0) = 1;
        }
#endif
    }
    printing_state = PRINT_STATE_HEATING;
    menu.add_menu(menu_t(lcd_main_preheat, MAIN_MENU_ITEM_POS(0)));
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
        opt.setData(MENU_NORMAL, init_preheat);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_print);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_main_maintenance);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, init_material_settings);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, init_material_move);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, init_material_change);
    }
    return opt;
}

void drawMainExpert(uint8_t nr, uint8_t &flags)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+2
                                , 17
                                , 52
                                , 13
                                , PSTR("PREHEAT")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+2
                                , 31
                                , 52
                                , 13
                                , PSTR("PRINT")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+2
                                , 45
                                , 52
                                , 13
                                , PSTR("ADVANCED")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                                , 17
                                , 52
                                , 13
                                , PSTR("SETTINGS")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                                , 31
                                , 52
                                , 13
                                , PSTR("MOVE")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == menu_index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                                , 45
                                , 52
                                , 13
                                , PSTR("CHANGE")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
}

void lcd_menu_main()
{
    lcd_lib_clear();

    if (ui_mode & UI_MODE_EXPERT)
    {
        lcd_lib_draw_vline(LCD_GFX_WIDTH/2, 5, 60);
        lcd_lib_set(0, 0, LCD_GFX_WIDTH-1, 12);
        lcd_lib_clear(LCD_GFX_WIDTH/2, 1, LCD_GFX_WIDTH/2, 12);
        lcd_lib_clear_string_center_atP(32, 4, PSTR("START"));
        lcd_lib_clear_string_center_atP(96, 4, PSTR("MATERIAL"));
    }
    else
    {
        lcd_lib_draw_vline(64, 5, 46);
        lcd_lib_draw_hline(3, 124, 50);
    }

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
            menu.process_submenu(get_main_expert, 6);
        }
        else
        {
            menu.process_submenu(get_main_standard, 3);
        }
    }
    if (ui_mode & UI_MODE_EXPERT)
    {
        for (uint8_t index=0; index<6; ++index)
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
