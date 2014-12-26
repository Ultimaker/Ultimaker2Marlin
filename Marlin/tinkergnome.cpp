#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "cardreader.h"
#include "temperature.h"
#include "UltiLCD2.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_maintenance.h"

#include "tinkergnome.h"
#include "UltiLCD2_low_lib.h"

#define LCD_TIMEOUT_TO_STATUS 15000

#define LCD_DETAIL_CACHE_START ((LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))+1)
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
#define LCD_DETAIL_CACHE_MATERIAL(n) (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*n])
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])

static void lcd_menu_print_tune_speed();
static void lcd_menu_print_tune_fan();

// print menu options
//This code uses the lcd_cache as buffer to store the titles, to save memory.

menuoption_t printOptions[] = {
                                  {"TUNE", 32, 58, 56, LCD_CHAR_HEIGHT, lcd_menu_print_tune, NULL, STATE_NONE}
                                 ,{"ABORT", 96, 58, 56, LCD_CHAR_HEIGHT, lcd_menu_print_abort, NULL, STATE_NONE}
                                 ,{LCD_CACHE_FILENAME(0), 30, 36, 28, LCD_CHAR_HEIGHT, lcd_menu_print_tune_speed, NULL, STATE_NONE}
                                 ,{LCD_CACHE_FILENAME(1), 30, 45, 28, LCD_CHAR_HEIGHT, lcd_menu_print_tune_fan, NULL, STATE_NONE}
                                 ,{LCD_CACHE_FILENAME(2), 100, 54-(EXTRUDERS*9)-(BED_MENU_OFFSET*9), 28, LCD_CHAR_HEIGHT, lcd_menu_print_tune_heatup_nozzle0, NULL, STATE_NONE}
#if EXTRUDERS > 1
                                 ,{LCD_CACHE_FILENAME(3), 100, 45-(BED_MENU_OFFSET*9), 28, LCD_CHAR_HEIGHT, lcd_menu_print_tune_heatup_nozzle1, NULL, STATE_NONE}
#endif
#if TEMP_SENSOR_BED != 0
                                 ,{LCD_CACHE_FILENAME(4), 100, 45, 28, LCD_CHAR_HEIGHT, lcd_menu_maintenance_advanced_bed_heatup, NULL, STATE_NONE}
#endif
                              };

uint8_t ui_mode = UI_MODE_STANDARD;

void tinkergnome_init()
{
    if (GET_UI_MODE() == UI_MODE_TINKERGNOME)
        ui_mode = UI_MODE_TINKERGNOME;
    else
        ui_mode = UI_MODE_STANDARD;
}


void lcd_lib_draw_string_left(uint8_t y, const char* str)
{
    lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT, y, str);
}

void lcd_lib_draw_string_right(uint8_t x, uint8_t y, const char* str)
{
    lcd_lib_draw_string(x - (strlen(str) * LCD_CHAR_WIDTH), y, str);
}

void lcd_lib_draw_string_right(uint8_t y, const char* str)
{
    lcd_lib_draw_string_right(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT, y, str);
}

void lcd_lib_draw_string_leftP(uint8_t y, const char* pstr)
{
    lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, y, pstr);
}

void lcd_lib_draw_string_rightP(uint8_t y, const char* pstr)
{
    lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (strlen_P(pstr) * LCD_CHAR_WIDTH), y, pstr);
}

void lcd_lib_draw_string_rightP(uint8_t x, uint8_t y, const char* pstr)
{
    lcd_lib_draw_stringP(x - (strlen_P(pstr) * LCD_CHAR_WIDTH), y, pstr);
}

char* int_to_time_string_tg(unsigned long i, char* temp_buffer)
{
    char* c = temp_buffer;
    uint8_t hours = i / 60 / 60;
    uint8_t mins = (i / 60) % 60;
    uint8_t secs = i % 60;

    if (!hours & !mins) {
        *c++ = '0';
        *c++ = '0';
        *c++ = ':';
        *c++ = '0' + secs / 10;
        *c++ = '0' + secs % 10;
    }else{
        if (hours > 99)
            *c++ = '0' + hours / 100;
        *c++ = '0' + (hours / 10) % 10;
        *c++ = '0' + hours % 10;
        *c++ = ':';
        *c++ = '0' + mins / 10;
        *c++ = '0' + mins % 10;
        *c++ = 'h';
    }

    *c = '\0';
    return c;
}

static char* lcd_advanced_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("LED settings"));
    else if (nr == 2)
#if EXTRUDERS < 2
        strcpy_P(card.longFilename, PSTR("Heatup nozzle"));
#else
        strcpy_P(card.longFilename, PSTR("Heatup first nozzle"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Heatup second nozzle"));
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == 2 + EXTRUDERS)
        strcpy_P(card.longFilename, PSTR("Heatup buildplate"));
#endif
    else if (nr == 2 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(card.longFilename, PSTR("Home head"));
    else if (nr == 3 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(card.longFilename, PSTR("Lower buildplate"));
    else if (nr == 4 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(card.longFilename, PSTR("Raise buildplate"));
    else if (nr == 5 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(card.longFilename, PSTR("Insert material"));
    else if (nr == 6 + BED_MENU_OFFSET + EXTRUDERS)
#if EXTRUDERS < 2
        strcpy_P(card.longFilename, PSTR("Move material"));
#else
        strcpy_P(card.longFilename, PSTR("Move material (1)"));
    else if (nr == 7 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(card.longFilename, PSTR("Move material (2)"));
#endif
    else if (nr == 6 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Set fan speed"));
    else if (nr == 7 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Retraction settings"));
    else if (nr == 8 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Motion settings"));
    else if (nr == 9 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Buildplate"));
    else if (nr == 10 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Expert Settings"));
    else if (nr == 11 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Version"));
    else if (nr == 12 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Runtime stats"));
    else if (nr == 13 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(card.longFilename, PSTR("Factory reset"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

void lcd_progressline(uint8_t y, uint8_t progress)
{
    if (progress)
    {
        lcd_lib_set(LCD_GFX_WIDTH-3, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), LCD_GFX_WIDTH-1, LCD_GFX_HEIGHT-1);
        lcd_lib_set(0, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), 2, LCD_GFX_HEIGHT-1);
    }
}


static void lcd_advanced_details(uint8_t nr)
{
    char buffer[16];
    buffer[0] = '\0';
    if (nr == 1)
    {
        int_to_string(led_brightness_level, buffer, PSTR("%"));
    }else if (nr == 2)
    {
        int_to_string(int(dsp_temperature[0]), buffer, PSTR("C/"));
        int_to_string(int(target_temperature[0]), buffer+strlen(buffer), PSTR("C"));
#if EXTRUDERS > 1
    }else if (nr == 3)
    {
        int_to_string(int(dsp_temperature[1]), buffer, PSTR("C/"));
        int_to_string(int(target_temperature[1]), buffer+strlen(buffer), PSTR("C"));
#endif
#if TEMP_SENSOR_BED != 0
    }else if (nr == 2 + EXTRUDERS)
    {
        int_to_string(int(dsp_temperature_bed), buffer, PSTR("C/"));
        int_to_string(int(target_temperature_bed), buffer+strlen(buffer), PSTR("C"));
#endif
    }else if (nr == 6 + BED_MENU_OFFSET + EXTRUDERS * 2)
    {
        int_to_string(int(fanSpeed) * 100 / 255, buffer, PSTR("%"));
    }else if (nr == 10 + BED_MENU_OFFSET + EXTRUDERS * 2)
    {
        strcpy_P(buffer, ui_mode == UI_MODE_TINKERGNOME ? PSTR("Geek Mode") : PSTR("Standard"));
    }else if (nr == 11 + BED_MENU_OFFSET + EXTRUDERS * 2)
    {
        lcd_lib_draw_stringP(5, 53, PSTR(STRING_CONFIG_H_AUTHOR));
        return;
    }else{
        return;
    }
    lcd_lib_draw_string_left(53, buffer);
}

void lcd_menu_maintenance_tg()
{
    lcd_scroll_menu(PSTR("MAINTENANCE"), 13 + BED_MENU_OFFSET + EXTRUDERS * 2, lcd_advanced_item, lcd_advanced_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
            lcd_change_to_menu(lcd_menu_main);
        else
        if (IS_SELECTED_SCROLL(1))
            lcd_change_to_menu(lcd_menu_maintenance_led, 0);
        else if (IS_SELECTED_SCROLL(2))
        {
            active_extruder = 0;
            lcd_change_to_menu(lcd_menu_maintenance_advanced_heatup, 0);
        }
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(3))
        {
            active_extruder = 1;
            lcd_change_to_menu(lcd_menu_maintenance_advanced_heatup, 0);
        }
#endif
#if TEMP_SENSOR_BED != 0
        else if (IS_SELECTED_SCROLL(2 + EXTRUDERS))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_bed_heatup, 0);
#endif
        else if (IS_SELECTED_SCROLL(2 + BED_MENU_OFFSET + EXTRUDERS))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 X0 Y0"));
        }
        else if (IS_SELECTED_SCROLL(3 + BED_MENU_OFFSET + EXTRUDERS))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
        }
        else if (IS_SELECTED_SCROLL(4 + BED_MENU_OFFSET + EXTRUDERS))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
            enquecommand_P(PSTR("G1 Z40"));
        }
        else if (IS_SELECTED_SCROLL(5 + BED_MENU_OFFSET + EXTRUDERS))
        {
            lcd_change_to_menu(lcd_menu_insert_material, 0);
        }
        else if (IS_SELECTED_SCROLL(6 + BED_MENU_OFFSET + EXTRUDERS))
        {
            set_extrude_min_temp(0);
            active_extruder = 0;
            target_temperature[active_extruder] = material[active_extruder].temperature;
            lcd_change_to_menu(lcd_menu_maintenance_extrude, 0);
        }
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(7 + BED_MENU_OFFSET + EXTRUDERS))
        {
            set_extrude_min_temp(0);
            active_extruder = 1;
            target_temperature[active_extruder] = material[active_extruder].temperature;
            lcd_change_to_menu(lcd_menu_maintenance_extrude, 0);
        }
#endif
        else if (IS_SELECTED_SCROLL(6 + BED_MENU_OFFSET + EXTRUDERS * 2))
            LCD_EDIT_SETTING_BYTE_PERCENT(fanSpeed, "Fan speed", "%", 0, 100);
        else if (IS_SELECTED_SCROLL(7 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_maintenance_retraction, SCROLL_MENU_ITEM_POS(0));
        else if (IS_SELECTED_SCROLL(8 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_maintenance_motion, SCROLL_MENU_ITEM_POS(0));
        else if (IS_SELECTED_SCROLL(9 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_first_run_start_bed_leveling, SCROLL_MENU_ITEM_POS(0));
        else if (IS_SELECTED_SCROLL(10 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_maintenance_uimode, SCROLL_MENU_ITEM_POS(0));
        else if (IS_SELECTED_SCROLL(11 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_advanced_version, SCROLL_MENU_ITEM_POS(0));
        else if (IS_SELECTED_SCROLL(12 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_advanced_stats, SCROLL_MENU_ITEM_POS(0));
        else if (IS_SELECTED_SCROLL(13 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_advanced_factory_reset, SCROLL_MENU_ITEM_POS(1));
    }
}


void lcd_draw_menu_option(const menuoption_t &option)
{
    if (option.state != STATE_NONE)
    {
        // draw frame
        lcd_lib_draw_box(option.centerX-(option.width/2), option.centerY-2-(option.height/2),
                         option.centerX-2+(option.width/2), option.centerY-2+(option.height/2));
    }
    if (option.state & STATE_SELECTED)
    {
        // draw box
        lcd_lib_set(option.centerX+1-(option.width/2), option.centerY-1-(option.height/2),
                    option.centerX-3+(option.width/2), option.centerY-3+(option.height/2));
    }

    const char* split = strchr(option.title, '|');

    if (split)
    {
        char buf[10];
        strncpy(buf, option.title, split - option.title);
        buf[split - option.title] = '\0';

        if (option.state & STATE_SELECTED)
        {
            lcd_lib_clear_string(option.centerX - strlen(buf)*LCD_CHAR_WIDTH/2, option.centerY - (LCD_CHAR_HEIGHT/2), buf);
            lcd_lib_clear_string(option.centerX - strlen(split+1)*LCD_CHAR_WIDTH/2, option.centerY + (LCD_CHAR_HEIGHT/2), split+1);
        } else {
            lcd_lib_draw_string(option.centerX - strlen(buf)*LCD_CHAR_WIDTH/2, option.centerY - (LCD_CHAR_HEIGHT/2), buf);
            lcd_lib_draw_string(option.centerX - strlen(split+1)*LCD_CHAR_WIDTH/2, option.centerY + (LCD_CHAR_HEIGHT/2), split+1);
        }
    }else{
        if (option.state & STATE_SELECTED)
        {
            lcd_lib_clear_string(option.centerX-(strlen(option.title)*LCD_CHAR_WIDTH/2), option.centerY-(LCD_CHAR_HEIGHT/2), option.title);
        } else {
            lcd_lib_draw_string(option.centerX-(strlen(option.title)*LCD_CHAR_WIDTH/2), option.centerY-(LCD_CHAR_HEIGHT/2), option.title);
        }
    }

}

static void lcd_menu_print_tune_speed()
{
    currentMenu = previousMenu;
    lcd_lib_encoder_pos = previousEncoderPos;
    LCD_EDIT_SETTING(feedmultiply, "Print speed", "%", 10, 1000);
}

static void lcd_menu_print_tune_fan()
{
    currentMenu = previousMenu;
    lcd_lib_encoder_pos = previousEncoderPos;
    LCD_EDIT_SETTING_BYTE_PERCENT(fanSpeed, "Fan speed", "%", 0, 100);
}

void lcd_process_menu_options(menuoption_t options[], uint8_t len)
{

    static unsigned long timeout = 0;
    static uint16_t lastEncoderPos = 0;

    if ((lcd_lib_encoder_pos != lastEncoderPos) || (lcd_lib_button_pressed))
    {
        timeout = millis() + LCD_TIMEOUT_TO_STATUS;
        lastEncoderPos = lcd_lib_encoder_pos;
    }

    if (lcd_lib_encoder_pos != ENCODER_NO_SELECTION)
    {
        // adjust encoder position
        if (lcd_lib_encoder_pos < 0)
            lcd_lib_encoder_pos += len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        else if (lcd_lib_encoder_pos >= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_lib_encoder_pos -= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }

    // update menu option state
    menuFunc_t menuFunc = NULL;
    bool bReset = (timeout < millis());
    for (uint8_t index=0; index<len; ++index) {
        menuoption_t &opt(options[index]);
        if (bReset)
        {
            opt.state = STATE_NONE;
        }else{
            if (IS_SELECTED_MAIN(index))
            {
                opt.state = STATE_SELECTED;
                menuFunc = opt.menuFunc;
            }else{
                opt.state = STATE_NONE;
            }
        }
    }
    if (menuFunc && lcd_lib_button_pressed) lcd_change_to_menu(menuFunc, 0);
}

void lcd_menu_print_heatup_tg()
{
    lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));

#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > target_temperature_bed - 10)
    {
#endif
        for(uint8_t e=0; e<EXTRUDERS; e++)
        {
            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] > 0)
                continue;
            target_temperature[e] = material[e].temperature;
        }

        if (current_temperature_bed >= target_temperature_bed - TEMP_WINDOW * 2 && !is_command_queued())
        {
            bool ready = true;
            for(uint8_t e=0; e<EXTRUDERS; e++)
                if (current_temperature[e] < target_temperature[e] - TEMP_WINDOW)
                    ready = false;

            if (ready)
            {
                doStartPrint();
                currentMenu = lcd_menu_printing_tg;
            }
        }
#if TEMP_SENSOR_BED != 0
    }
#endif

    uint8_t progress = 125;
    for(uint8_t e=0; e<EXTRUDERS; e++)
    {
        if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] < 1)
            continue;
        if (current_temperature[e] > 20)
            progress = min(progress, (current_temperature[e] - 20) * 125 / (target_temperature[e] - 20 - TEMP_WINDOW));
        else
            progress = 0;
    }
#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > 20)
        progress = min(progress, (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW));
    else if (target_temperature_bed > current_temperature_bed - 20)
        progress = 0;
#endif

    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;

    lcd_lib_draw_string_leftP(14, PSTR("Heating up... "));

    char buffer[8];
    int_to_string(progress*100/124, buffer, PSTR("%"));
    lcd_lib_draw_string_right(14, buffer);

    lcd_progressline(2, progress);

    // temperature extruder 1
    uint8_t ypos = 40;
#if TEMP_SENSOR_BED != 0
    int_to_string(target_temperature_bed, buffer, PSTR("C"));
    lcd_lib_draw_string_right(ypos, buffer);
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature_bed, buffer, PSTR("C"));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    ypos -= 9;
#endif // TEMP_SENSOR_BED
#if EXTRUDERS > 1
    int_to_string(target_temperature[1], buffer, PSTR("C"));
    lcd_lib_draw_string_right(ypos, buffer);
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[1], buffer, PSTR("C"));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    ypos -= 9;
#endif // EXTRUDERS
    int_to_string(target_temperature[0], buffer, PSTR("C"));
    lcd_lib_draw_string_right(ypos, buffer);
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[0], buffer, PSTR("C"));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);

    lcd_lib_draw_string_left(4, card.longFilename);
    lcd_lib_draw_hline(3, 124, 12);

    lcd_lib_update_screen();
}

void lcd_menu_printing_tg()
{
    lcd_process_menu_options(printOptions, sizeof(printOptions)/sizeof(printOptions[0]));

    lcd_basic_screen();

    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    char buffer[16];
    char* c;
    uint8_t len;
    uint8_t index;

    switch(printing_state)
    {
    default:
        // z position
        lcd_lib_draw_string_leftP(14, PSTR("Z"));
        float_to_string(current_position[Z_AXIS], buffer, NULL);
        lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_WIDTH+LCD_CHAR_WIDTH/2, 14, buffer);

        // speed
        c = printOptions[2].title;
        strcpy_P(c, PSTR("Speed "));
        len = int_to_string(feedmultiply, c+strlen(c), PSTR("%")) - c;
        printOptions[2].centerX = LCD_CHAR_MARGIN_LEFT + (len*LCD_CHAR_WIDTH/2);
        printOptions[2].width = len*LCD_CHAR_WIDTH+4;

        // fan speed
        c = printOptions[3].title;
        strcpy_P(c, PSTR("Fan "));
        len = int_to_string(int(fanSpeed) * 100 / 255, c+strlen(c), PSTR("%")) - c;
        printOptions[3].centerX = LCD_CHAR_MARGIN_LEFT + (len*LCD_CHAR_WIDTH/2);
        printOptions[3].width = len*LCD_CHAR_WIDTH+4;

        // temperature extruder 1
        c = printOptions[4].title;
        len = int_to_string(dsp_temperature[0], c, PSTR("C")) - c;
        printOptions[4].width = len*LCD_CHAR_WIDTH+4;
        printOptions[4].centerX = LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (len*LCD_CHAR_WIDTH/2);
#if EXTRUDERS > 1
        // temperature extruder 2
        c = printOptions[5].title;
        len = int_to_string(dsp_temperature[1], c, PSTR("C")) - c;
        printOptions[5].width = len*LCD_CHAR_WIDTH+4;
        printOptions[5].centerX = LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (len*LCD_CHAR_WIDTH/2);
#endif
#if TEMP_SENSOR_BED != 0
        // temperature build-plate
        index = 3+EXTRUDERS+BED_MENU_OFFSET;
        c = printOptions[index].title;
        len = int_to_string(dsp_temperature_bed, c, PSTR("C")) - c;
        printOptions[index].width = len*LCD_CHAR_WIDTH+4;
        printOptions[index].centerX = LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (len*LCD_CHAR_WIDTH/2);
#endif

        for (uint8_t index=2; index<sizeof(printOptions)/sizeof(printOptions[0]); ++index) {
            lcd_draw_menu_option(printOptions[index]);
        }

        lcd_lib_draw_string_left(4, card.longFilename);
        lcd_lib_draw_hline(3, 124, 12);
        break;
    case PRINT_STATE_WAIT_USER:
        lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
        lcd_lib_draw_string_centerP(20, PSTR("Press button"));
        lcd_lib_draw_string_centerP(30, PSTR("to continue"));
        break;
    case PRINT_STATE_HEATING:
        lcd_lib_draw_string_centerP(20, PSTR("Heating"));
        c = int_to_string(dsp_temperature[0], buffer, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature[0], c, PSTR("C"));
        lcd_lib_draw_string_center(30, buffer);
        break;
    case PRINT_STATE_HEATING_BED:
        lcd_lib_draw_string_centerP(20, PSTR("Heating buildplate"));
        c = int_to_string(dsp_temperature_bed, buffer, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature_bed, c, PSTR("C"));
        lcd_lib_draw_string_center(30, buffer);
        break;
    }
    float printTimeMs = (millis() - starttime);
    float printTimeSec = printTimeMs / 1000L;
    float totalTimeMs = float(printTimeMs) * float(card.getFileSize()) / float(card.getFilePos());
    static float totalTimeSmoothSec;
    totalTimeSmoothSec = (totalTimeSmoothSec * 999L + totalTimeMs / 1000L) / 1000L;
    if (isinf(totalTimeSmoothSec))
        totalTimeSmoothSec = totalTimeMs;

    if (LCD_DETAIL_CACHE_TIME() == 0 && printTimeSec < 60)
    {
        totalTimeSmoothSec = totalTimeMs / 1000;
        // lcd_lib_draw_stringP(5, 10, PSTR("Time left unknown"));

    }else{
        unsigned long totalTimeSec;
        if (printTimeSec < LCD_DETAIL_CACHE_TIME() / 2)
        {
            float f = float(printTimeSec) / float(LCD_DETAIL_CACHE_TIME() / 2);
            if (f > 1.0)
                f = 1.0;
            totalTimeSec = float(totalTimeSmoothSec) * f + float(LCD_DETAIL_CACHE_TIME()) * (1 - f);
        }else{
            totalTimeSec = totalTimeSmoothSec;
        }
        unsigned long timeLeftSec;
        if (printTimeSec > totalTimeSec)
            timeLeftSec = 1;
        else
            timeLeftSec = totalTimeSec - printTimeSec;


        // lcd_lib_draw_stringP(5, 10, PSTR("Time left"));

        int_to_time_string_tg(timeLeftSec, buffer);
        lcd_lib_draw_string(58, 14, buffer);

    }

    int_to_string(progress*100/124, buffer, PSTR("%"));
    // draw string right aligned
    lcd_lib_draw_string_right(14, buffer);

    // lcd_progressline(2, progress);

    lcd_draw_menu_option(printOptions[0]);
    lcd_draw_menu_option(printOptions[1]);

    lcd_lib_update_screen();
}

static char* lcd_uimode_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR(" Standard Mode"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR(" Geek Mode"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    if (nr - 1 == led_mode)
        card.longFilename[0] = '>';
    return card.longFilename;
}

static void lcd_uimode_details(uint8_t nr)
{
    // nothing displayed yet
}

void lcd_menu_maintenance_uimode()
{
    lcd_scroll_menu(PSTR("Expert settings"), 3, lcd_uimode_item, lcd_uimode_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
        {
        }
        else if (IS_SELECTED_SCROLL(1))
        {
            ui_mode = UI_MODE_STANDARD;
            SET_UI_MODE(ui_mode);
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            ui_mode = UI_MODE_TINKERGNOME;
            SET_UI_MODE(ui_mode);
        }
        lcd_change_to_menu(lcd_menu_maintenance_tg, SCROLL_MENU_ITEM_POS(10 + BED_MENU_OFFSET + EXTRUDERS * 2));
    }
}

#endif//ENABLE_ULTILCD2
