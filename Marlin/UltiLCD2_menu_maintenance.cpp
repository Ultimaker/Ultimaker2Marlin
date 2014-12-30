#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_maintenance.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_material.h"
#include "cardreader.h"
#include "lifetime_stats.h"
#include "ConfigurationStore.h"
#include "temperature.h"
#include "pins.h"
#include "tinkergnome.h"


//static void lcd_menu_maintenance_advanced();
static void lcd_menu_maintenance_advanced_heatup();
static void lcd_menu_maintenance_led();
static void lcd_menu_maintenance_extrude();
static void lcd_menu_maintenance_retraction();
static void lcd_menu_advanced_version();
static void lcd_menu_advanced_stats();
static void lcd_menu_maintenance_motion();
static void lcd_menu_advanced_factory_reset();

void lcd_menu_maintenance()
{
    if (ui_mode & UI_MODE_TINKERGNOME)
    {
        lcd_remove_menu();
        lcd_add_menu(lcd_menu_maintenance_advanced, ENCODER_NO_SELECTION);
    }else{
        lcd_tripple_menu(PSTR("BUILD-|PLATE"), PSTR("ADVANCED"), PSTR("RETURN"));

        if (lcd_lib_button_pressed)
        {
            if (IS_SELECTED_MAIN(0))
                lcd_change_to_menu(lcd_menu_first_run_start_bed_leveling);
            else if (IS_SELECTED_MAIN(1))
                lcd_change_to_menu(lcd_menu_maintenance_advanced);
            else if (IS_SELECTED_MAIN(2))
                lcd_change_to_previous_menu();
        }

        lcd_lib_update_screen();
    }
}

static char* lcd_advanced_item(uint8_t nr)
{
    static const char* itemText[] = {
                                        PSTR("< RETURN")
                                      , PSTR("LED settings")
#if EXTRUDERS < 2
                                      , PSTR("Heatup nozzle")
#else
                                      , PSTR("Heatup first nozzle")
                                      , PSTR("Heatup second nozzle")
#endif
#if TEMP_SENSOR_BED != 0
                                      , PSTR("Heatup buildplate")
#endif
                                      , PSTR("Home head")
                                      , PSTR("Lower buildplate")
                                      , PSTR("Raise buildplate")
                                      , PSTR("Insert material")
#if EXTRUDERS < 2
                                      , PSTR("Move material")
#else
                                      , PSTR("Move material (1)")
                                      , PSTR("Move material (2)")
#endif
                                      , PSTR("Set fan speed")
                                      , PSTR("Retraction settings")
                                      , PSTR("Motion settings")
                                      , PSTR("Adjust buildplate")
                                      , PSTR("Expert settings")
                                      , PSTR("Version")
                                      , PSTR("Runtime stats")
                                      , PSTR("Factory reset")
                                      , PSTR("???")
                                    };

    if (!(ui_mode & UI_MODE_TINKERGNOME) && (nr > 8+BED_MENU_OFFSET+2*EXTRUDERS))
        ++nr;
    strcpy_P(card.longFilename, itemText[constrain(nr, 0, sizeof(itemText)/sizeof(itemText[0])-1)]);
    return card.longFilename;
}

static void lcd_advanced_details(uint8_t nr)
{
    char buffer[16];
    buffer[0] = '\0';
    if (!(ui_mode & UI_MODE_TINKERGNOME) && (nr > 8+BED_MENU_OFFSET+2*EXTRUDERS))
        ++nr;

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
        lcd_lib_draw_stringP(5, BOTTOM_MENU_YPOS, (ui_mode & UI_MODE_TINKERGNOME) ? PSTR("Geek Mode") : PSTR("Standard Mode"));
        return;
    }else if (nr == 11 + BED_MENU_OFFSET + EXTRUDERS * 2)
    {
        lcd_lib_draw_stringP(5, BOTTOM_MENU_YPOS, PSTR(STRING_CONFIG_H_AUTHOR));
        return;
    }else{
        return;
    }
    lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
}

void lcd_menu_maintenance_advanced()
{
    lcd_scroll_menu((ui_mode & UI_MODE_TINKERGNOME) ? PSTR("MAINTENANCE") : PSTR("ADVANCED"), BED_MENU_OFFSET + 2*EXTRUDERS + ((ui_mode & UI_MODE_TINKERGNOME) ? 14 : 13), lcd_advanced_item, lcd_advanced_details);
    if (lcd_lib_button_pressed)
    {
        uint8_t index = 0;
        if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_previous_menu();
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_maintenance_led, 0, lcd_lib_encoder_pos);
        else if (IS_SELECTED_SCROLL(index++))
        {
            active_extruder = 0;
            lcd_change_to_menu(lcd_menu_maintenance_advanced_heatup, 0, lcd_lib_encoder_pos);
        }
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(index++))
        {
            active_extruder = 1;
            lcd_change_to_menu(lcd_menu_maintenance_advanced_heatup, 0, lcd_lib_encoder_pos);
        }
#endif
#if TEMP_SENSOR_BED != 0
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_bed_heatup, 0, lcd_lib_encoder_pos);
#endif
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 X0 Y0"));
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_lib_beep();
            enquecommand_P(PSTR("G28 Z0"));
            enquecommand_P(PSTR("G1 Z40"));
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_change_to_menu(lcd_menu_insert_material, 0, lcd_lib_encoder_pos);
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            set_extrude_min_temp(0);
            active_extruder = 0;
            target_temperature[active_extruder] = material[active_extruder].temperature;
            lcd_change_to_menu(lcd_menu_maintenance_extrude, 0, lcd_lib_encoder_pos);
        }
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(index++))
        {
            set_extrude_min_temp(0);
            active_extruder = 1;
            target_temperature[active_extruder] = material[active_extruder].temperature;
            lcd_change_to_menu(lcd_menu_maintenance_extrude, 0, lcd_lib_encoder_pos);
        }
#endif
        else if (IS_SELECTED_SCROLL(index++))
            LCD_EDIT_SETTING_BYTE_PERCENT(fanSpeed, "Fan speed", "%", 0, 100);
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_maintenance_retraction, SCROLL_MENU_ITEM_POS(0), lcd_lib_encoder_pos);
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_maintenance_motion, SCROLL_MENU_ITEM_POS(0), lcd_lib_encoder_pos);
        else if ((ui_mode & UI_MODE_TINKERGNOME) && (IS_SELECTED_SCROLL(index++)))
            lcd_change_to_menu(lcd_menu_first_run_start_bed_leveling, SCROLL_MENU_ITEM_POS(0), lcd_lib_encoder_pos);
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_maintenance_uimode, SCROLL_MENU_ITEM_POS(0), lcd_lib_encoder_pos);
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_advanced_version, SCROLL_MENU_ITEM_POS(0), lcd_lib_encoder_pos);
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_advanced_stats, SCROLL_MENU_ITEM_POS(0), lcd_lib_encoder_pos);
        else if (IS_SELECTED_SCROLL(index++))
            lcd_change_to_menu(lcd_menu_advanced_factory_reset, SCROLL_MENU_ITEM_POS(1), lcd_lib_encoder_pos);
    }
}

static void lcd_menu_maintenance_advanced_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature[active_extruder] = int(target_temperature[active_extruder]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM);
        if (target_temperature[active_extruder] < 0)
            target_temperature[active_extruder] = 0;
        if (target_temperature[active_extruder] > HEATER_0_MAXTEMP - 15)
            target_temperature[active_extruder] = HEATER_0_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_previous_menu();

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Nozzle temperature:"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(dsp_temperature[active_extruder]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[active_extruder]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 40, getHeaterPower(active_extruder));
    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_extrude()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        if (printing_state == PRINT_STATE_NORMAL && movesplanned() < 3)
        {
            current_position[E_AXIS] += lcd_lib_encoder_pos * 0.1;
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 10, active_extruder);
            lcd_lib_encoder_pos = 0;
        }
    }
    if (lcd_lib_button_pressed)
    {
        set_extrude_min_temp(EXTRUDE_MINTEMP);
        target_temperature[active_extruder] = 0;
        lcd_change_to_previous_menu();
    }

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(10, PSTR("Nozzle temperature:"));
    lcd_lib_draw_string_centerP(40, PSTR("Rotate to extrude"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(dsp_temperature[active_extruder]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[active_extruder]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(20, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 30, getHeaterPower(active_extruder));
    lcd_lib_update_screen();
}

#if TEMP_SENSOR_BED != 0
void lcd_menu_maintenance_advanced_bed_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature_bed = int(target_temperature_bed) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM);
        if (target_temperature_bed < 0)
            target_temperature_bed = 0;
        if (target_temperature_bed > BED_MAXTEMP - 15)
            target_temperature_bed = BED_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_previous_menu();

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Buildplate temp.:"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(dsp_temperature_bed), buffer, PSTR("C/"));
    int_to_string(int(target_temperature_bed), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 40, getHeaterPower(-1));
    lcd_lib_update_screen();
}
#endif

static void lcd_menu_advanced_version()
{
    lcd_info_screen(NULL, lcd_change_to_previous_menu, PSTR("Return"));
    lcd_lib_draw_string_centerP(30, PSTR(STRING_VERSION_CONFIG_H));
    lcd_lib_draw_string_centerP(40, PSTR(STRING_CONFIG_H_AUTHOR));
    lcd_lib_update_screen();
}

static void lcd_menu_advanced_stats()
{
    lcd_info_screen(NULL, lcd_change_to_previous_menu, PSTR("Return"));
    lcd_lib_draw_string_centerP(10, PSTR("Machine on for:"));
    char buffer[16];
    char* c = int_to_string(lifetime_minutes / 60, buffer, PSTR(":"));
    if (lifetime_minutes % 60 < 10)
        *c++ = '0';
    c = int_to_string(lifetime_minutes % 60, c);
    lcd_lib_draw_string_center(20, buffer);

    lcd_lib_draw_string_centerP(30, PSTR("Printing:"));
    c = int_to_string(lifetime_print_minutes / 60, buffer, PSTR(":"));
    if (lifetime_print_minutes % 60 < 10)
        *c++ = '0';
    c = int_to_string(lifetime_print_minutes % 60, c);
    strcpy_P(c, PSTR(" Mat:"));
    c += 5;
    c = int_to_string(lifetime_print_centimeters / 100, c, PSTR("m"));
    lcd_lib_draw_string_center(40, buffer);
    lcd_lib_update_screen();
}

static void doMachineRestart()
{
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

static void doFactoryReset()
{
    lcd_change_to_previous_menu();
    //Clear the EEPROM settings so they get read from default.
    eeprom_write_byte((uint8_t*)100, 0);
    eeprom_write_byte((uint8_t*)101, 0);
    eeprom_write_byte((uint8_t*)102, 0);
    eeprom_write_byte((uint8_t*)EEPROM_FIRST_RUN_DONE_OFFSET, 0);
    eeprom_write_byte((uint8_t*)EEPROM_UI_MODE_OFFSET, 0);
    eeprom_write_byte(EEPROM_MATERIAL_COUNT_OFFSET(), 0);
    doMachineRestart();
}

static void lcd_menu_advanced_factory_reset()
{
    lcd_question_screen(NULL, doFactoryReset, PSTR("YES"), NULL, lcd_change_to_previous_menu, PSTR("NO"));

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
    lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
}

static void lcd_menu_maintenance_retraction()
{
    lcd_scroll_menu(PSTR("RETRACTION"), 3, lcd_retraction_item, lcd_retraction_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
        {
            Config_StoreSettings();
            lcd_change_to_previous_menu();
        }
        else if (IS_SELECTED_SCROLL(1))
            LCD_EDIT_SETTING_FLOAT001(retract_length, "Retract length", "mm", 0, 50);
        else if (IS_SELECTED_SCROLL(2))
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
    else if (nr == 6)
        strcpy_P(card.longFilename, PSTR("Current X/Y"));
    else if (nr == 7)
        strcpy_P(card.longFilename, PSTR("Current Z"));
    else if (nr == 8)
        strcpy_P(card.longFilename, PSTR("Current E"));
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
    else if(nr == 6)
        int_to_string(motor_current_setting[0], buffer, PSTR("mA"));
    else if(nr == 7)
        int_to_string(motor_current_setting[1], buffer, PSTR("mA"));
    else if(nr == 8)
        int_to_string(motor_current_setting[2], buffer, PSTR("mA"));
    lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
}

static void lcd_menu_maintenance_motion()
{
    lcd_scroll_menu(PSTR("MOTION"), 9, lcd_motion_item, lcd_motion_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
        {
            digipot_current(0, motor_current_setting[0]);
            digipot_current(1, motor_current_setting[1]);
            digipot_current(2, motor_current_setting[2]);
            Config_StoreSettings();
            lcd_change_to_previous_menu();
        }
        else if (IS_SELECTED_SCROLL(1))
            LCD_EDIT_SETTING_FLOAT100(acceleration, "Acceleration", "mm/sec^2", 0, 20000);
        else if (IS_SELECTED_SCROLL(2))
            LCD_EDIT_SETTING_FLOAT1(max_xy_jerk, "X/Y Jerk", "mm/sec", 0, 100);
        else if (IS_SELECTED_SCROLL(3))
            LCD_EDIT_SETTING_FLOAT1(max_feedrate[X_AXIS], "Max speed X", "mm/sec", 0, 1000);
        else if (IS_SELECTED_SCROLL(4))
            LCD_EDIT_SETTING_FLOAT1(max_feedrate[Y_AXIS], "Max speed Y", "mm/sec", 0, 1000);
        else if (IS_SELECTED_SCROLL(5))
            LCD_EDIT_SETTING_FLOAT1(max_feedrate[Z_AXIS], "Max speed Z", "mm/sec", 0, 1000);
        else if (IS_SELECTED_SCROLL(6))
            LCD_EDIT_SETTING(motor_current_setting[0], "Current X/Y", "mA", 0, 1300);
        else if (IS_SELECTED_SCROLL(7))
            LCD_EDIT_SETTING(motor_current_setting[1], "Current Z", "mA", 0, 1300);
        else if (IS_SELECTED_SCROLL(8))
            LCD_EDIT_SETTING(motor_current_setting[2], "Current E", "mA", 0, 1300);
    }
}

static char* lcd_led_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Brightness"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR(" Always On"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR(" Always Off"));
    else if (nr == 4)
        strcpy_P(card.longFilename, PSTR(" On while printing"));
    else if (nr == 5)
        strcpy_P(card.longFilename, PSTR(" Glow when done"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    if (nr - 2 == led_mode)
        card.longFilename[0] = '>';
    return card.longFilename;
}

static void lcd_led_details(uint8_t nr)
{
    char buffer[16];
    if (nr == 0)
        return;
    else if(nr == 1)
    {
        int_to_string(led_brightness_level, buffer, PSTR("%"));
        lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
    }
}

static void lcd_menu_maintenance_led()
{
    analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
    lcd_scroll_menu(PSTR("LED"), 6, lcd_led_item, lcd_led_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
        {
            if (led_mode != LED_MODE_ALWAYS_ON)
                analogWrite(LED_PIN, 0);
            Config_StoreSettings();
            lcd_change_to_previous_menu();
        }
        else if (IS_SELECTED_SCROLL(1))
        {
            LCD_EDIT_SETTING(led_brightness_level, "Brightness", "%", 0, 100);
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            led_mode = LED_MODE_ALWAYS_ON;
            lcd_lib_beep();
        }
        else if (IS_SELECTED_SCROLL(3))
        {
            led_mode = LED_MODE_ALWAYS_OFF;
            lcd_lib_beep();
        }
        else if (IS_SELECTED_SCROLL(4))
        {
            led_mode = LED_MODE_WHILE_PRINTING;
            lcd_lib_beep();
        }
        else if (IS_SELECTED_SCROLL(5))
        {
            led_mode = LED_MODE_BLINK_ON_DONE;
            lcd_lib_beep();
        }
    }
}

#endif//ENABLE_ULTILCD2
