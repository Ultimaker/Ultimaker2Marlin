#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_maintenance.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_utils.h"
#include "UltiLCD2_menu_prefs.h"
#include "cardreader.h"
#include "lifetime_stats.h"
#include "ConfigurationStore.h"
#include "machinesettings.h"
#include "temperature.h"
#include "pins.h"
#include "preferences.h"
#include "tinkergnome.h"
#include "powerbudget.h"

static void lcd_menu_maintenance_advanced_heatup();
//static void lcd_menu_maintenance_led();
static void lcd_menu_maintenance_extrude();
static void lcd_menu_advanced_version();
static void lcd_menu_advanced_stats();
static void lcd_menu_maintenance_motion();
static void lcd_menu_advanced_factory_reset();
static void lcd_menu_preferences();

void lcd_menu_maintenance()
{
    lcd_tripple_menu(PSTR("BUILD-|PLATE"), PSTR("ADVANCED"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_MAIN(0))
        {
            menu.add_menu(menu_t(NULL, lcd_change_to_previous_menu, homeHead));
            menu.add_menu(menu_t(lcd_menu_first_run_start_bed_leveling));
        }
        else if (IS_SELECTED_MAIN(1))
            menu.add_menu(menu_t(lcd_menu_maintenance_advanced));
        else if (IS_SELECTED_MAIN(2))
            menu.return_to_previous();
    }

    lcd_lib_update_screen();
}

static void lcd_advanced_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
        strcpy_P(buffer, PSTR("< RETURN"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Preferences"));
#if EXTRUDERS > 1
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Swap extruders"));
#endif
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Heatup nozzle"));
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Heatup buildplate"));
#endif
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Home head"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Lower buildplate"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Raise buildplate"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Insert material"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Move material"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Set fan speed"));
    else if ((ui_mode & UI_MODE_EXPERT) && (nr == index++))
        strcpy_P(buffer, PSTR("Adjust buildplate"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Expert functions"));
    else
        strcpy_P(buffer, PSTR("???"));

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_advanced_details(uint8_t nr)
{
    char buffer[32] = {0};
    buffer[0] = '\0';
    if (!(ui_mode & UI_MODE_EXPERT) && (nr > 8+BED_MENU_OFFSET+2*EXTRUDERS))
        ++nr;

#if EXTRUDERS > 1
    if (nr == 2)
    {
        // extruders swapped?
        uint8_t xpos = (swapExtruders() ? LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING : LCD_CHAR_MARGIN_LEFT);
        lcd_lib_draw_stringP(xpos, BOTTOM_MENU_YPOS, PSTR("PRIMARY"));

        xpos = (swapExtruders() ? LCD_CHAR_MARGIN_LEFT : LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6*LCD_CHAR_SPACING);
        lcd_lib_draw_stringP(xpos, BOTTOM_MENU_YPOS, PSTR("SECOND"));

        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2-LCD_CHAR_MARGIN_RIGHT-LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("<->"));
        return;
    }
    else if (nr == EXTRUDERS + 1)
#else
    if (nr == 2)
#endif
    {
#if EXTRUDERS > 1
        int_to_string(int(target_temperature[1]), int_to_string(int(dsp_temperature[1]), int_to_string(int(target_temperature[0]), int_to_string(int(dsp_temperature[0]), buffer, PSTR("C/")), PSTR("C ")), PSTR("C/")), PSTR("C"));
#else
        int_to_string(int(target_temperature[0]), int_to_string(int(dsp_temperature[0]), buffer, PSTR("C/")), PSTR("C"));
#endif // EXTRUDERS
    }
#if TEMP_SENSOR_BED != 0
    else if (nr == EXTRUDERS + 2)
    {
        int_to_string(int(target_temperature_bed), int_to_string(int(dsp_temperature_bed), buffer, PSTR("C/")), PSTR("C"));
    }
#endif
    else if (nr == EXTRUDERS + BED_MENU_OFFSET + 7)
    {
        int_to_string(int(fanSpeed) * 100 / 255, buffer, PSTR("%"));
    }
    else
    {
        return;
    }
    lcd_lib_draw_string_left(BOTTOM_MENU_YPOS, buffer);
}

static void lcd_preferences_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
        strcpy_P(buffer, PSTR("< RETURN"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("User interface"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("LED settings"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Click sound"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Scroll filenames"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Sleep timer"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Screen contrast"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Heater timeout"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Temperature control"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Retraction settings"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Motion settings"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Print area"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Power budget"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Version"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Runtime stats"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Factory reset"));
    else
        strcpy_P(buffer, PSTR("???"));

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_preferences_details(uint8_t nr)
{
    char buffer[32] = {0};
    buffer[0] = '\0';

    if (nr == 1)
    {
        if (ui_mode & UI_MODE_EXPERT)
        {
            strcpy_P(buffer, PSTR("Geek Mode"));
        }
        else
        {
            strcpy_P(buffer, PSTR("Standard Mode"));
        }
    }
    else if (nr == 2)
    {
        int_to_string(led_brightness_level, buffer, PSTR("%"));
    }
    else if (nr == 3)
    {
        if (ui_mode & UI_BEEP_OFF)
        {
            strcpy_P(buffer, PSTR("Off"));
        }
        else if (ui_mode & UI_BEEP_SHORT)
        {
            strcpy_P(buffer, PSTR("Short tick"));
        }
        else
        {
            strcpy_P(buffer, PSTR("Standard"));
        }
    }
    else if (nr == 4)
    {
        if (ui_mode & UI_SCROLL_ENTRY)
        {
            strcpy_P(buffer, PSTR("Enabled"));
        }
        else
        {
            strcpy_P(buffer, PSTR("Disabled"));
        }
    }
    else if (nr == 6)
    {
        int_to_string(float(lcd_contrast)*100/255 + 0.5f, buffer, PSTR("%"));
    }
    else if (nr == 7)
    {
        char *c = buffer;
        if (heater_timeout)
        {
            c = int_to_string(heater_timeout, buffer, PSTR("min / "));
        }
        else
        {
            strcpy_P(buffer, PSTR("off / "));
            c += 6;
        }
        if (heater_check_time)
        {
            int_to_string(heater_check_time, c, PSTR("s"));
        }
        else
        {
            strcpy_P(c, PSTR("off"));
        }
    }
    else if (nr == 13)
    {
        strcpy_P(buffer, PSTR(STRING_CONFIG_H_AUTHOR));
    }
    else
    {
        return;
    }
    lcd_lib_draw_string_left(BOTTOM_MENU_YPOS, buffer);
}

void homeHead()
{
    enquecommand_P(PSTR("G28 X0 Y0"));
}

void homeBed()
{
    enquecommand_P(PSTR("G28 Z0"));
}

void homeAll()
{
    enquecommand_P(PSTR("G28"));
}

static void start_nozzle_heatup()
{
#if EXTRUDERS > 1
    // remove nozzle selection menu
    menu.return_to_previous();
#endif // EXTRUDERS
    menu.add_menu(menu_t(lcd_menu_maintenance_advanced_heatup, MAIN_MENU_ITEM_POS(0), 4));
}

static void start_insert_material()
{
#if EXTRUDERS > 1
    // remove nozzle selection menu
    menu.return_to_previous();
#endif // EXTRUDERS
    lcd_material_change_init(false);
    menu.add_menu(menu_t(lcd_menu_insert_material_preheat));
}

void start_move_material()
{
#if EXTRUDERS > 1
    // remove nozzle selection menu
    menu.return_to_previous();
#endif // EXTRUDERS
    set_extrude_min_temp(0);
    // reset e-position
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS], active_extruder, true);
    // heatup nozzle
    target_temperature[active_extruder] = material[active_extruder].temperature[0];

    if (ui_mode & UI_MODE_EXPERT)
    {
        menu.add_menu(menu_t(lcd_init_extrude, lcd_menu_expert_extrude, NULL));
    }
    else
    {
        menu.add_menu(menu_t(lcd_menu_maintenance_extrude, MAIN_MENU_ITEM_POS(0)));
    }
}

#if EXTRUDERS > 1

FORCE_INLINE static void lcd_dual_nozzle_heatup()
{
    lcd_select_nozzle(start_nozzle_heatup, NULL);
}

FORCE_INLINE static void lcd_dual_insert_material()
{
    lcd_select_nozzle(start_insert_material, NULL);
}

void lcd_dual_move_material()
{
    lcd_select_nozzle(start_move_material, NULL);
}

#endif // EXTRUDERS

void lcd_menu_maintenance_advanced()
{
    lcd_scroll_menu(PSTR("ADVANCED"), BED_MENU_OFFSET + EXTRUDERS + ((ui_mode & UI_MODE_EXPERT) ? 10 : 9), lcd_advanced_item, lcd_advanced_details);
    if (lcd_lib_button_pressed)
    {
        uint8_t index = 0;
        if (IS_SELECTED_SCROLL(index++))
            menu.return_to_previous();
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_preferences, SCROLL_MENU_ITEM_POS(0)));
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(index++))
        {
            menu.add_menu(menu_t(init_swap_menu, lcd_menu_swap_extruder, NULL));
        }
#endif
        else if (IS_SELECTED_SCROLL(index++))
        {
            // heatup nozzle
        #if EXTRUDERS < 2
            active_extruder = 0;
            start_nozzle_heatup();
        #else
            menu.add_menu(menu_t(lcd_dual_nozzle_heatup, MAIN_MENU_ITEM_POS(active_extruder ? 1 : 0)));
        #endif
        }
#if TEMP_SENSOR_BED != 0
        else if (IS_SELECTED_SCROLL(index++))
        {
            menu.add_menu(menu_t(lcd_menu_maintenance_advanced_bed_heatup, MAIN_MENU_ITEM_POS(0), 4));
        }
#endif
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_lib_keyclick();
            homeHead();
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_lib_keyclick();
            homeBed();
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            lcd_lib_keyclick();
            homeBed();
            enquecommand_P(PSTR("G1 Z40"));
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
        // insert material
        #if EXTRUDERS < 2
            active_extruder = 0;
            start_insert_material();
        #else
            menu.add_menu(menu_t(lcd_dual_insert_material, MAIN_MENU_ITEM_POS(active_extruder ? 1 : 0)));
        #endif // EXTRUDERS
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            // move material
        #if EXTRUDERS < 2
            active_extruder = 0;
            start_move_material();
        #else
            menu.add_menu(menu_t(lcd_dual_move_material, MAIN_MENU_ITEM_POS(active_extruder ? 1 : 0)));
        #endif // EXTRUDERS
        }
        else if (IS_SELECTED_SCROLL(index++))
        {
            LCD_EDIT_SETTING_BYTE_PERCENT(fanSpeed, "Fan speed", "%", 0, 100);
        }
        else if ((ui_mode & UI_MODE_EXPERT) && (IS_SELECTED_SCROLL(index++)))
        {
            menu.add_menu(menu_t(lcd_menu_first_run_start_bed_leveling, SCROLL_MENU_ITEM_POS(0)));
        }
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_maintenance_expert, SCROLL_MENU_ITEM_POS(0)));
    }
    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_advanced_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature[active_extruder] = constrain(int(target_temperature[active_extruder]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM)
                                                      , 0, get_maxtemp(active_extruder) - 15);
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_previous_menu();

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Nozzle temperature:"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16] = {0};
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
    // reset heater timeout until target temperature is reached
    if ((degHotend(active_extruder) < 100) || (degHotend(active_extruder) < (degTargetHotend(active_extruder) - 20)))
    {
        last_user_interaction = millis();
    }
    if (lcd_lib_button_pressed || !target_temperature[active_extruder])
    {
        set_extrude_min_temp(EXTRUDE_MINTEMP);
        target_temperature[active_extruder] = 0;
        menu.return_to_previous();
    }
    // reset heater timeout until target temperature is reached
    if ((degTargetHotend(active_extruder) < 120) || (degHotend(active_extruder) < (degTargetHotend(active_extruder) - 20)))
    {
        last_user_interaction = millis();
    }

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(10, PSTR("Nozzle temperature:"));
    lcd_lib_draw_string_centerP(40, PSTR("Rotate to extrude"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));

    char buffer[32] = {0};
    ;
    int_to_string(int(target_temperature[active_extruder]), int_to_string(int(dsp_temperature[active_extruder]), buffer, PSTR("C/")), PSTR("C"));
    lcd_lib_draw_string_center(20, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 30, getHeaterPower(active_extruder));
    lcd_lib_update_screen();
}

#if TEMP_SENSOR_BED != 0
void lcd_menu_maintenance_advanced_bed_heatup()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature_bed = constrain(int(target_temperature_bed) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM)
                                          , 0, BED_MAXTEMP - 15);
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_previous_menu();

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Buildplate temp.:"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16] = {0};
    int_to_string(int(dsp_temperature_bed), buffer, PSTR("C/"));
    int_to_string(int(target_temperature_bed), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 40, getHeaterPower(-1));
    lcd_lib_update_screen();
}
#endif

static void lcd_menu_advanced_version()
{
    lcd_info_screen(NULL, lcd_change_to_previous_menu, PSTR("RETURN"));
    lcd_lib_draw_string_centerP(30, PSTR(STRING_VERSION_CONFIG_H));
    lcd_lib_draw_string_centerP(40, PSTR(STRING_CONFIG_H_AUTHOR));
    lcd_lib_update_screen();
}

static void lcd_menu_advanced_stats()
{
    lcd_info_screen(NULL, lcd_change_to_previous_menu, PSTR("RETURN"));
    lcd_lib_draw_string_centerP(10, PSTR("Machine on for:"));
    char buffer[24] = {0};
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
    eeprom_write_word((uint16_t*)EEPROM_EXPERT_VERSION_OFFSET, 0xFFFF);
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

static void lcd_motion_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[32] = {0};
    if (nr == 0)
        strcpy_P(buffer, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(buffer, PSTR("Acceleration"));
    else if (nr == 2)
        strcpy_P(buffer, PSTR("Max. axis speed"));
    else if (nr == 3)
        strcpy_P(buffer, PSTR("Motor Current"));
    else if (nr == 4)
        strcpy_P(buffer, PSTR("Axis steps/mm"));
    else if (nr == 5)
        strcpy_P(buffer, PSTR("Invert axis"));
    else
       strcpy_P(buffer, PSTR("???"));

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_menu_maintenance_motion()
{
    lcd_scroll_menu(PSTR("MOTION"), 6, lcd_motion_item, NULL);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
            menu.return_to_previous();
        else if (IS_SELECTED_SCROLL(1))
            menu.add_menu(menu_t(lcd_menu_acceleration, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(2))
            menu.add_menu(menu_t(lcd_menu_maxspeed, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(3))
            menu.add_menu(menu_t(lcd_menu_motorcurrent, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(4))
            menu.add_menu(menu_t(lcd_menu_steps, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(5))
            menu.add_menu(menu_t(lcd_menu_axisdirection, MAIN_MENU_ITEM_POS(1)));
    }
    lcd_lib_update_screen();
}

static void lcd_led_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[32] = {0};
    if (nr == 0)
        strcpy_P(buffer, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(buffer, PSTR("Brightness"));
    else if (nr == 2)
        strcpy_P(buffer, PSTR(" Always On"));
    else if (nr == 3)
        strcpy_P(buffer, PSTR(" Always Off"));
    else if (nr == 4)
        strcpy_P(buffer, PSTR(" On while printing"));
    else if (nr == 5)
        strcpy_P(buffer, PSTR(" Glow when done"));
    else
        strcpy_P(buffer, PSTR("???"));
    if (nr - 2 == led_mode)
        buffer[0] = '>';

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_led_details(uint8_t nr)
{
    char buffer[16] = {0};
    if (nr == 0)
        return;
    else if(nr == 1)
    {
        int_to_string(led_brightness_level, buffer, PSTR("%"));
        lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
    }
}

static void init_maintenance_led()
{
    cache._byte[0] = led_mode;
    cache._byte[1] = led_brightness_level;
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
            if ((led_mode != cache._byte[0]) || (led_brightness_level != cache._byte[1]))
                Config_StoreSettings();
            menu.return_to_previous();
        }
        else if (IS_SELECTED_SCROLL(1))
        {
            menu.currentMenu().initMenuFunc = NULL;
            LCD_EDIT_SETTING(led_brightness_level, "Brightness", "%", 0, 100);
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            led_mode = LED_MODE_ALWAYS_ON;
            lcd_lib_keyclick();
        }
        else if (IS_SELECTED_SCROLL(3))
        {
            led_mode = LED_MODE_ALWAYS_OFF;
            lcd_lib_keyclick();
        }
        else if (IS_SELECTED_SCROLL(4))
        {
            led_mode = LED_MODE_WHILE_PRINTING;
            lcd_lib_keyclick();
        }
        else if (IS_SELECTED_SCROLL(5))
        {
            led_mode = LED_MODE_BLINK_ON_DONE;
            lcd_lib_keyclick();
        }
    }
    lcd_lib_update_screen();
}

static void lcd_uimode_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[LINE_ENTRY_TEXT_LENGTH] = {' '};

    if (nr == 0)
    {
        strcpy_P(buffer, PSTR("< RETURN"));
    }
    else if (nr == 1)
    {
        if (!(ui_mode & UI_MODE_EXPERT))
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("Standard Mode"));
    }
    else if (nr == 2)
    {
        if (ui_mode & UI_MODE_EXPERT)
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("Geek Mode"));
    }
    else
    {
        strcpy_P(buffer+1, PSTR("???"));
    }

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_clicksound_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[LINE_ENTRY_TEXT_LENGTH] = {' '};
    if (nr == 0)
    {
        strcpy_P(buffer, PSTR("< RETURN"));
    }
    else if (nr == 1)
    {
        if (!(ui_mode & (UI_BEEP_OFF | UI_BEEP_SHORT)))
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("Standard beep"));
    }
    else if (nr == 2)
    {
        if (ui_mode & UI_BEEP_SHORT)
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("Short tick"));
    }
    else if (nr == 3)
    {
        if (ui_mode & UI_BEEP_OFF)
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("Off"));
    }
    else
    {
        strcpy_P(buffer, PSTR(" ???"));
    }

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_scrollentry_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[LINE_ENTRY_TEXT_LENGTH] = {' '};

    if (nr == 0)
    {
        strcpy_P(buffer, PSTR("< RETURN"));
    }
    else if (nr == 1)
    {
        if (!(ui_mode & UI_SCROLL_ENTRY))
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("No scrolling"));
    }
    else if (nr == 2)
    {
        if (ui_mode & UI_SCROLL_ENTRY)
        {
            strcpy_P(buffer, PSTR(">"));
        }
        strcpy_P(buffer+1, PSTR("Scroll filenames"));
    }

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_menu_uimode()
{
    lcd_scroll_menu(PSTR("User interface"), 3, lcd_uimode_item, NULL);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            ui_mode &= ~UI_MODE_EXPERT;
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            ui_mode |= UI_MODE_EXPERT;
        }
        if (ui_mode != GET_UI_MODE())
        {
            SET_UI_MODE(ui_mode);
        }
        menu.return_to_previous();
    }
    lcd_lib_update_screen();
}

static void lcd_menu_clicksound()
{
    lcd_scroll_menu(PSTR("Click sound"), 4, lcd_clicksound_item, NULL);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            ui_mode &= ~UI_BEEP_OFF;
            ui_mode &= ~UI_BEEP_SHORT;
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            ui_mode &= ~UI_BEEP_OFF;
            ui_mode |= UI_BEEP_SHORT;
        }
        else if (IS_SELECTED_SCROLL(3))
        {
            ui_mode &= ~UI_BEEP_SHORT;
            ui_mode |= UI_BEEP_OFF;
        }
        if (ui_mode != GET_UI_MODE())
        {
            SET_UI_MODE(ui_mode);
        }
        menu.return_to_previous();
    }
    lcd_lib_update_screen();
}

static void lcd_menu_scrollentry()
{
    lcd_scroll_menu(PSTR("Scroll filenames"), 3, lcd_scrollentry_item, NULL);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            ui_mode &= ~UI_SCROLL_ENTRY;
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            ui_mode |= UI_SCROLL_ENTRY;
        }
        if (ui_mode != GET_UI_MODE())
        {
            SET_UI_MODE(ui_mode);
        }
        menu.return_to_previous();
    }
    lcd_lib_update_screen();
}

static void lcd_menu_screen_contrast()
{
    if (lcd_tune_byte(lcd_contrast, 0, 100))
    {
        lcd_contrast = constrain(lcd_contrast, 1, 255);
        lcd_lib_contrast(lcd_contrast);
    }
    if (lcd_lib_button_pressed)
    {
        SET_LCD_CONTRAST(lcd_contrast);
        menu.return_to_previous();
    }

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(10, PSTR("Screen contrast"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));

    char buffer[32] = {0};
    int_to_string(float(lcd_contrast)*100/255 + 0.5f, buffer, PSTR("%"));

    lcd_lib_draw_string_center(22, buffer);

    lcd_lib_draw_bargraph(LCD_CHAR_MARGIN_LEFT+2*LCD_CHAR_SPACING, 34, LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-2*LCD_CHAR_SPACING, 40, float(lcd_contrast)/255);

    lcd_lib_update_screen();
}

static void lcd_menu_preferences()
{
    lcd_scroll_menu(PSTR("PREFERENCES"), BED_MENU_OFFSET + 15, lcd_preferences_item, lcd_preferences_details);
    if (lcd_lib_button_pressed)
    {
        uint8_t index = 0;
        if (IS_SELECTED_SCROLL(index++))
            menu.return_to_previous();
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_uimode));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(init_maintenance_led, lcd_menu_maintenance_led, NULL));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_clicksound));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_scrollentry));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_sleeptimer));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_screen_contrast, 0, 4));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_heatercheck, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(index++))
#if (TEMP_SENSOR_BED != 0) || (EXTRUDERS > 1)
            menu.add_menu(menu_t(lcd_menu_tempcontrol));
#else
            menu.add_menu(menu_t(init_tempcontrol_e1, lcd_menu_tempcontrol_e1, NULL));
#endif
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_retraction, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_maintenance_motion, SCROLL_MENU_ITEM_POS(0)));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_axeslimit, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_powerbudget, MAIN_MENU_ITEM_POS(1)));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_advanced_version, SCROLL_MENU_ITEM_POS(0)));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_advanced_stats, SCROLL_MENU_ITEM_POS(0)));
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_advanced_factory_reset, MAIN_MENU_ITEM_POS(1)));
    }
    lcd_lib_update_screen();
}

#endif//ENABLE_ULTILCD2
