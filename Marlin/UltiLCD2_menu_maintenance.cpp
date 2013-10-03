#include "configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_maintenance.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_material.h"
#include "cardreader.h"
#include "ConfigurationStore.h"
#include "temperature.h"
#include "pins.h"


static void lcd_menu_maintenance_first_run_select();
static void lcd_menu_maintenance_advanced();
void lcd_menu_maintenance_advanced_heatup();
void lcd_menu_maintenance_advanced_bed_heatup();
static void lcd_menu_maintenance_extrude();
static void lcd_menu_maintenance_retraction();
static void lcd_menu_advanced_version();
static void lcd_menu_maintenance_motion();
static void lcd_menu_advanced_factory_reset();
static void lcd_menu_TODO();

void lcd_menu_maintenance()
{
    lcd_tripple_menu(PSTR("CALIBRATE"), PSTR("ADVANCED"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance_first_run_select);
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_maintenance_advanced);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_main);
    }

    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_select()
{
    lcd_tripple_menu(PSTR("BED"), PSTR("..."), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_change_to_menu(lcd_menu_first_run_start_bed_leveling);
        }else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_TODO);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance);
    }

    lcd_lib_update_screen();
}

static char* lcd_advanced_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Heatup head"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("Heatup bed"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Home head"));
    else if (nr == 4)
        strcpy_P(card.longFilename, PSTR("Lower bed"));
    else if (nr == 5)
        strcpy_P(card.longFilename, PSTR("Raise bed"));
    else if (nr == 6)
        strcpy_P(card.longFilename, PSTR("Move material"));
    else if (nr == 7)
        strcpy_P(card.longFilename, PSTR("Retraction settings"));
    else if (nr == 8)
        strcpy_P(card.longFilename, PSTR("Motion settings"));
    else if (nr == 9)
        strcpy_P(card.longFilename, PSTR("Version"));
    else if (nr == 10)
        strcpy_P(card.longFilename, PSTR("Factory reset"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_advanced_details(uint8_t nr)
{
    if (nr == 0)
    {
        
    }else if(nr == 1)
    {
        lcd_lib_draw_stringP(5, 53, PSTR("Heatup the head"));
    }else if(nr == 2)
    {
        lcd_lib_draw_stringP(5, 53, PSTR("Heatup the bed"));
    }else if(nr == 8)
    {
        lcd_lib_draw_stringP(5, 53, PSTR("Clear all settings"));
    }
}

static void lcd_menu_maintenance_advanced()
{
    lcd_scroll_menu(PSTR("ADVANCED"), 11, lcd_advanced_item, lcd_advanced_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance);
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_heatup, 0);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_bed_heatup, 0);
        else if (IS_SELECTED(3))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 X0 Y0"));
        }
        else if (IS_SELECTED(4))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
        }
        else if (IS_SELECTED(5))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
            enquecommand_P(PSTR("G1 Z40"));
        }
        else if (IS_SELECTED(6))
        {
            set_extrude_min_temp(0);
            target_temperature[0] = material.temperature;
            lcd_change_to_menu(lcd_menu_maintenance_extrude, 0);
        }
        else if (IS_SELECTED(7))
            lcd_change_to_menu(lcd_menu_maintenance_retraction, MENU_ITEM_POS(0));
        else if (IS_SELECTED(8))
            lcd_change_to_menu(lcd_menu_maintenance_motion, MENU_ITEM_POS(0));
        else if (IS_SELECTED(9))
            lcd_change_to_menu(lcd_menu_advanced_version, MENU_ITEM_POS(0));
        else if (IS_SELECTED(10))
            lcd_change_to_menu(lcd_menu_advanced_factory_reset, MENU_ITEM_POS(1));
    }
}

void lcd_menu_maintenance_advanced_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM != 0)
    {
        target_temperature[0] = int(target_temperature[0]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM);
        if (target_temperature[0] < 0)
            target_temperature[0] = 0;
        if (target_temperature[0] > HEATER_0_MAXTEMP - 15)
            target_temperature[0] = HEATER_0_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);
    
    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Head temperature:"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(current_temperature[0]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[0]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}

void lcd_menu_maintenance_extrude()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM != 0)
    {
        if (movesplanned() < 3)
        {
            current_position[E_AXIS] += lcd_lib_encoder_pos * 0.1;
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 10, active_extruder);
            lcd_lib_encoder_pos = 0;
        }
    }
    if (lcd_lib_button_pressed)
    {
        set_extrude_min_temp(EXTRUDE_MINTEMP);
        target_temperature[0] = 0;
        lcd_change_to_menu(previousMenu, previousEncoderPos);
    }
    
    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Head temperature:"));
    lcd_lib_draw_string_centerP(40, PSTR("Rotate to extrude"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(current_temperature[0]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[0]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}

void lcd_menu_maintenance_advanced_bed_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM != 0)
    {
        target_temperature_bed = int(target_temperature_bed) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MENU_ITEM);
        if (target_temperature_bed < 0)
            target_temperature_bed = 0;
        if (target_temperature_bed > BED_MAXTEMP - 15)
            target_temperature_bed = BED_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);
    
    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Bed temperature:"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(current_temperature_bed), buffer, PSTR("C/"));
    int_to_string(int(target_temperature_bed), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}

void lcd_menu_advanced_version()
{
    lcd_info_screen(previousMenu, NULL, PSTR("Return"));
    lcd_lib_draw_string_centerP(30, PSTR(STRING_VERSION_CONFIG_H));
    lcd_lib_draw_string_centerP(40, PSTR(STRING_CONFIG_H_AUTHOR));
    lcd_lib_update_screen();
}

static void doFactoryReset()
{
    //Clear the EEPROM settings so they get read from default.
    eeprom_write_byte((uint8_t*)100, 0);
    eeprom_write_byte((uint8_t*)101, 0);
    eeprom_write_byte((uint8_t*)102, 0);
    eeprom_write_byte((uint8_t*)EEPROM_FIRST_RUN_DONE_OFFSET, 0);
    eeprom_write_byte(EEPROM_MATERIAL_COUNT_OFFSET(), 0);
    
    cli();
    //NOTE: Jumping to address 0 is not a fully proper way to reset.
    // Letting the watchdog timeout is a better reset, but the bootloader does not continue on a watchdog timeout.
    // So we disable interrupts and hope for the best!
    //Jump to address 0x0000
#ifdef __AVR__
    asm volatile(
            "clr	r30		\n\t"
            "clr	r31		\n\t"
            "ijmp	\n\t"
            );
#else
    //TODO
#endif
}

static void lcd_menu_advanced_factory_reset()
{
    lcd_question_screen(NULL, doFactoryReset, PSTR("YES"), previousMenu, NULL, PSTR("NO"));
    
    lcd_lib_draw_string_centerP(10, PSTR("Reset everything"));
    lcd_lib_draw_string_centerP(20, PSTR("to default?"));
    lcd_lib_update_screen();
}


static char* lcd_retraction_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Retract length"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("Retract speed"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_retraction_details(uint8_t nr)
{
    char buffer[16];
    if (nr == 0)
        return;
    else if(nr == 1)
        float_to_string(retract_length, buffer, PSTR("mm"));
    else if(nr == 2)
        int_to_string(retract_feedrate / 60 + 0.5, buffer, PSTR("mm/sec"));
    lcd_lib_draw_string(5, 53, buffer);
}

static void lcd_menu_maintenance_retraction()
{
    lcd_scroll_menu(PSTR("RETRACTION"), 3, lcd_retraction_item, lcd_retraction_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance_advanced, MENU_ITEM_POS(6));
        else if (IS_SELECTED(1))
            LCD_EDIT_SETTING_FLOAT001(retract_length, "Retract length", "mm", 0, 50);
        else if (IS_SELECTED(2))
            LCD_EDIT_SETTING_SPEED(retract_feedrate, "Retract speed", "mm/sec", 0, max_feedrate[E_AXIS] * 60);
    }
}

static char* lcd_motion_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Acceleration"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("X/Y Jerk"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Max speed X"));
    else if (nr == 4)
        strcpy_P(card.longFilename, PSTR("Max speed Y"));
    else if (nr == 5)
        strcpy_P(card.longFilename, PSTR("Max speed Z"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_motion_details(uint8_t nr)
{
    char buffer[16];
    if (nr == 0)
        return;
    else if(nr == 1)
        int_to_string(acceleration, buffer, PSTR("mm/sec^2"));
    else if(nr == 2)
        int_to_string(max_xy_jerk, buffer, PSTR("mm/sec"));
    else if(nr == 3)
        int_to_string(max_feedrate[X_AXIS], buffer, PSTR("mm/sec"));
    else if(nr == 4)
        int_to_string(max_feedrate[Y_AXIS], buffer, PSTR("mm/sec"));
    else if(nr == 5)
        int_to_string(max_feedrate[Z_AXIS], buffer, PSTR("mm/sec"));
    lcd_lib_draw_string(5, 53, buffer);
}

static void lcd_menu_maintenance_motion()
{
    lcd_scroll_menu(PSTR("MOTION"), 6, lcd_motion_item, lcd_motion_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance_advanced, MENU_ITEM_POS(7));
        else if (IS_SELECTED(1))
            LCD_EDIT_SETTING_FLOAT100(acceleration, "Acceleration", "mm/sec^2", 0, 20000);
        else if (IS_SELECTED(2))
            LCD_EDIT_SETTING_FLOAT1(max_xy_jerk, "X/Y Jerk", "mm/sec", 0, 100);
        else if (IS_SELECTED(3))
            LCD_EDIT_SETTING_FLOAT1(max_feedrate[X_AXIS], "Max speed X", "mm/sec", 0, 1000);
        else if (IS_SELECTED(4))
            LCD_EDIT_SETTING_FLOAT1(max_feedrate[Y_AXIS], "Max speed Y", "mm/sec", 0, 1000);
        else if (IS_SELECTED(5))
            LCD_EDIT_SETTING_FLOAT1(max_feedrate[Z_AXIS], "Max speed Z", "mm/sec", 0, 1000);
    }
}

static void lcd_menu_TODO()
{
    lcd_info_screen(previousMenu, NULL, PSTR("OK"));
    
    lcd_lib_draw_string_centerP(20, PSTR("UNIMPLEMENTED"));

    lcd_lib_update_screen();
}

#endif//ENABLE_ULTILCD2
