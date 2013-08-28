#include <avr/pgmspace.h>

#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "marlin.h"
#include "cardreader.h"//This code uses the card.longFilename as buffer to store data, to save memory.
#include "temperature.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_material.h"

struct materialSettings material = {0, 0, 100, 100, 2.85};

void doCooldown();//TODO
void lcd_menu_main();//TODO
static void lcd_menu_change_material_preheat();
static void lcd_menu_change_material_remove();
static void lcd_menu_change_material_remove_wait_user();
static void lcd_menu_change_material_remove_wait_user_ready();
static void lcd_menu_change_material_insert_wait_user();
static void lcd_menu_change_material_insert_wait_user_ready();
static void lcd_menu_change_material_insert_forward();
static void lcd_menu_change_material_insert();
static void lcd_menu_material_select();
static void lcd_menu_material_selected();
static void lcd_menu_material_settings();

static void cancelMaterialInsert()
{
    digipot_current(2, motor_current_setting[2]);//Set E motor power to default.
    doCooldown();
}

void lcd_menu_material()
{
    lcd_tripple_menu(PSTR("CHANGE"), PSTR("SETTINGS"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            minProgress = 0;
            lcd_change_to_menu(lcd_menu_change_material_preheat);//TODO: If multiple extruders, select extruder.
        }
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_material_select, MENU_ITEM_POS(0));
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_main);
    }

    lcd_lib_update_screen();
}

static void lcd_menu_change_material_preheat()
{
    setTargetHotend(material.temperature + 10, 0);
    int16_t temp = degHotend(0) - 20;
    int16_t target = degTargetHotend(0) - 10 - 20;
    if (temp < 0) temp = 0;
    if (temp > target)
    {
        volume_to_filament_length = 1.0;//Set the extrusion to 1mm per given value, so we can move the filament a set distance.
        
        digipot_current(2, motor_current_setting[2] / 2);//Set the E motor power lower to we skip instead of grind.
        
        float old_max_feedrate_e = max_feedrate[E_AXIS];
        float old_retract_acceleration = retract_acceleration;
        max_feedrate[E_AXIS] = FILAMENT_REVERSAL_SPEED;
        retract_acceleration = FILAMENT_LONG_MOVE_ACCELERATION;
        
        current_position[E_AXIS] = 0;
        plan_set_e_position(current_position[E_AXIS]);
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], -1.0, FILAMENT_REVERSAL_SPEED, 0);
        for(uint8_t n=0;n<6;n++)
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], (n+1)*-FILAMENT_REVERSAL_LENGTH/6, FILAMENT_REVERSAL_SPEED, 0);
        
        max_feedrate[E_AXIS] = old_max_feedrate_e;
        retract_acceleration = old_retract_acceleration;
        
        currentMenu = lcd_menu_change_material_remove;
        temp = target;
    }

    uint8_t progress = uint8_t(temp * 125 / target);
    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;
    
    lcd_info_screen(lcd_menu_main, cancelMaterialInsert);
    lcd_lib_draw_stringP(3, 10, PSTR("Heating printhead"));
    lcd_lib_draw_stringP(3, 20, PSTR("for material removal"));

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_remove()
{
    lcd_info_screen(lcd_menu_main, cancelMaterialInsert);
    lcd_lib_draw_stringP(3, 20, PSTR("Reversing material"));
    
    if (!blocks_queued())
    {
        lcd_lib_beep();
        led_glow_dir = led_glow = 0;
        currentMenu = lcd_menu_change_material_remove_wait_user;
        SELECT_MENU_ITEM(0);
        //Disable the extruder motor so you can pull out the remaining filament.
        disable_e0();
        disable_e1();
        disable_e2();
    }

    long pos = -st_get_position(E_AXIS);
    long targetPos = lround(FILAMENT_REVERSAL_LENGTH*axis_steps_per_unit[E_AXIS]);
    uint8_t progress = (pos * 125 / targetPos);
    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_remove_wait_user_ready()
{
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);
    lcd_change_to_menu(lcd_menu_change_material_insert_wait_user, MENU_ITEM_POS(0));
    
    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i X%i Y%i"), int(homing_feedrate[0]), X_MAX_LENGTH/2, 10);
    enquecommand(buffer);
}

static void lcd_menu_change_material_remove_wait_user()
{
    LED_GLOW();

    lcd_question_screen(NULL, lcd_menu_change_material_remove_wait_user_ready, PSTR("READY"), lcd_menu_main, cancelMaterialInsert, PSTR("CANCEL"));
    lcd_lib_draw_string_centerP(20, PSTR("Remove material"));
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert_wait_user()
{
    LED_GLOW();

    if (movesplanned() < 2)
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_SPEED, 0);
    }
    
    lcd_question_screen(NULL, lcd_menu_change_material_insert_wait_user_ready, PSTR("READY"), lcd_menu_main, cancelMaterialInsert, PSTR("CANCEL"));
    lcd_lib_draw_string_centerP(10, PSTR("Insert new material"));
    lcd_lib_draw_string_centerP(20, PSTR("from the backside of"));
    lcd_lib_draw_string_centerP(30, PSTR("your machine,"));
    lcd_lib_draw_string_centerP(40, PSTR("above the arrow."));
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert_wait_user_ready()
{
    //Override the max feedrate and acceleration values to get a better insert speed and speedup/slowdown
    float old_max_feedrate_e = max_feedrate[E_AXIS];
    float old_retract_acceleration = retract_acceleration;
    max_feedrate[E_AXIS] = FILAMENT_INSERT_FAST_SPEED;
    retract_acceleration = FILAMENT_LONG_MOVE_ACCELERATION;
    
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);
    for(uint8_t n=0;n<6;n++)
    {
        current_position[E_AXIS] += FILAMENT_FORWARD_LENGTH / 6;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_FAST_SPEED, 0);
    }
    
    //Put back origonal values.
    max_feedrate[E_AXIS] = old_max_feedrate_e;
    retract_acceleration = old_retract_acceleration;
    
    lcd_change_to_menu(lcd_menu_change_material_insert_forward);
}

static void lcd_menu_change_material_insert_forward()
{
    lcd_info_screen(lcd_menu_main, cancelMaterialInsert);
    lcd_lib_draw_stringP(3, 20, PSTR("Forwarding material"));
    
    if (!blocks_queued())
    {
        lcd_lib_beep();
        led_glow_dir = led_glow = 0;
        
        currentMenu = lcd_menu_change_material_insert;
        SELECT_MENU_ITEM(0);
    }

    long pos = st_get_position(E_AXIS);
    long targetPos = lround(FILAMENT_FORWARD_LENGTH*axis_steps_per_unit[E_AXIS]);
    uint8_t progress = (pos * 125 / targetPos);
    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void materialInsertReady()
{
    current_position[E_AXIS] -= 20;
    plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 25*60, 0);
    cancelMaterialInsert();
}

static void lcd_menu_change_material_insert()
{
    LED_GLOW();
    
    lcd_question_screen(lcd_menu_main, materialInsertReady, PSTR("READY"), lcd_menu_main, cancelMaterialInsert, PSTR("CANCEL"));
    lcd_lib_draw_string_centerP(20, PSTR("Wait till material"));
    lcd_lib_draw_string_centerP(30, PSTR("comes out the nozzle"));

    if (movesplanned() < 2)
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_EXTRUDE_SPEED, 0);
    }
    
    lcd_lib_update_screen();
}

static char* lcd_material_select_callback(uint8_t nr)
{
    uint8_t count = eeprom_read_byte(EEPROM_MATERIAL_COUNT_OFFSET());
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr > count)
        strcpy_P(card.longFilename, PSTR("Customize"));
    else{
        eeprom_read_block(card.longFilename, EEPROM_MATERIAL_NAME_OFFSET(nr - 1), 8);
    }
    return card.longFilename;
}

static void lcd_material_select_details_callback(uint8_t nr)
{
    uint8_t count = eeprom_read_byte(EEPROM_MATERIAL_COUNT_OFFSET());
    if (nr == 0)
    {
        
    }
    else if (nr <= count)
    {
        char buffer[32];
        char* c = buffer;
        nr -= 1;
        
        if (led_glow_dir)
        {
            c = float_to_string(eeprom_read_float(EEPROM_MATERIAL_DIAMETER_OFFSET(nr)), c, PSTR("mm"));
            while(c < buffer + 10) *c++ = ' ';
            strcpy_P(c, PSTR("Flow:"));
            c += 5;
            c = int_to_string(eeprom_read_word(EEPROM_MATERIAL_FLOW_OFFSET(nr)), c, PSTR("%"));
        }else{
            c = int_to_string(eeprom_read_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(nr)), c, PSTR("C"));
            *c++ = ' ';
            c = int_to_string(eeprom_read_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(nr)), c, PSTR("C"));
            while(c < buffer + 10) *c++ = ' ';
            strcpy_P(c, PSTR("Fan: "));
            c += 5;
            c = int_to_string(eeprom_read_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(nr)), c, PSTR("%"));
        }
        lcd_lib_draw_string(5, 53, buffer);
    }else{
        lcd_lib_draw_string_centerP(53, PSTR("Modify the settings"));
    }
}

static void lcd_menu_material_select()
{
    uint8_t count = eeprom_read_byte(EEPROM_MATERIAL_COUNT_OFFSET());
    
    lcd_scroll_menu(PSTR("MATERIAL"), count + 2, lcd_material_select_callback, lcd_material_select_details_callback);
    if (lcd_lib_button_pressed)
    {
        printf("%i\n", count);
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_material);
        else if (IS_SELECTED(count + 1))
            lcd_change_to_menu(lcd_menu_material_settings);
        else{
            lcd_material_set_material(SELECTED_MENU_ITEM() - 1);
            
            lcd_change_to_menu(lcd_menu_material_selected, MENU_ITEM_POS(0));
        }
    }
}

static void lcd_menu_material_selected()
{
    lcd_info_screen(lcd_menu_main, NULL, PSTR("OK"));
    lcd_lib_draw_string_centerP(20, PSTR("Selected material:"));
    lcd_lib_draw_string_center(30, card.longFilename);
    lcd_lib_update_screen();
}

static char* lcd_material_settings_callback(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Temperature"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("Heated bed"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Diameter"));
    else if (nr == 4)
        strcpy_P(card.longFilename, PSTR("Fan"));
    else if (nr == 5)
        strcpy_P(card.longFilename, PSTR("Flow %"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_material_settings_details_callback(uint8_t nr)
{
    char buffer[10];
    buffer[0] = '\0';
    if (nr == 0)
    {
        return;
    }else if (nr == 1)
    {
        int_to_string(material.temperature, buffer, PSTR("C"));
    }else if (nr == 2)
    {
        int_to_string(material.bed_temperature, buffer, PSTR("C"));
    }else if (nr == 3)
    {
        float_to_string(material.diameter, buffer, PSTR("mm"));
    }else if (nr == 4)
    {
        int_to_string(material.fan_speed, buffer, PSTR("%"));
    }else if (nr == 5)
    {
        int_to_string(material.flow, buffer, PSTR("%"));
    }
    lcd_lib_draw_string(5, 53, buffer);
}

static void lcd_menu_material_settings()
{
    lcd_scroll_menu(PSTR("MATERIAL"), 6, lcd_material_settings_callback, lcd_material_settings_details_callback);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_change_to_menu(lcd_menu_material);
            lcd_material_store_current_material();
        }else if (IS_SELECTED(1))
            LCD_EDIT_SETTING(material.temperature, "Temperature", "C", 0, HEATER_0_MAXTEMP - 15);
        else if (IS_SELECTED(2))
            LCD_EDIT_SETTING(material.bed_temperature, "Bed Temperature", "C", 0, BED_MAXTEMP - 15);
        else if (IS_SELECTED(3))
            LCD_EDIT_SETTING_FLOAT(material.diameter, "Material Diameter", "mm", 0, 1000);
        else if (IS_SELECTED(4))
            LCD_EDIT_SETTING(material.fan_speed, "Fan speed", "%", 0, 100);
        else if (IS_SELECTED(5))
            LCD_EDIT_SETTING(material.flow, "Material flow", "%", 1, 1000);
    }
}

void lcd_material_reset_defaults()
{
    //Fill in the defaults
    char buffer[8];
    
    strcpy_P(buffer, PSTR("PLA"));
    eeprom_write_block(buffer, EEPROM_MATERIAL_NAME_OFFSET(0), 4);
    eeprom_write_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(0), 210);
    eeprom_write_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(0), 65);
    eeprom_write_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(0), 100);
    eeprom_write_word(EEPROM_MATERIAL_FLOW_OFFSET(0), 100);
    eeprom_write_float(EEPROM_MATERIAL_DIAMETER_OFFSET(0), 2.85);

    strcpy_P(buffer, PSTR("ABS"));
    eeprom_write_block(buffer, EEPROM_MATERIAL_NAME_OFFSET(1), 4);
    eeprom_write_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(1), 240);
    eeprom_write_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(1), 80);
    eeprom_write_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(1), 50);
    eeprom_write_word(EEPROM_MATERIAL_FLOW_OFFSET(1), 107);
    eeprom_write_float(EEPROM_MATERIAL_DIAMETER_OFFSET(1), 2.85);
    
    eeprom_write_byte(EEPROM_MATERIAL_COUNT_OFFSET(), 2);
}

void lcd_material_set_material(uint8_t nr)
{
    material.temperature = eeprom_read_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(nr));
    material.bed_temperature = eeprom_read_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(nr));
    material.flow = eeprom_read_word(EEPROM_MATERIAL_FLOW_OFFSET(nr));

    material.fan_speed = eeprom_read_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(nr));
    material.diameter = eeprom_read_float(EEPROM_MATERIAL_DIAMETER_OFFSET(nr));
    eeprom_read_block(card.longFilename, EEPROM_MATERIAL_NAME_OFFSET(nr), 8);
    card.longFilename[8] = '\0';
    
    lcd_material_store_current_material();
}

void lcd_material_read_current_material()
{
    material.temperature = eeprom_read_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT));
    material.bed_temperature = eeprom_read_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT));
    material.flow = eeprom_read_word(EEPROM_MATERIAL_FLOW_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT));

    material.fan_speed = eeprom_read_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT));
    material.diameter = eeprom_read_float(EEPROM_MATERIAL_DIAMETER_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT));
}

void lcd_material_store_current_material()
{
    eeprom_write_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT), material.temperature);
    eeprom_write_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT), material.bed_temperature);
    eeprom_write_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT), material.flow);
    eeprom_write_word(EEPROM_MATERIAL_FLOW_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT), material.fan_speed);
    eeprom_write_float(EEPROM_MATERIAL_DIAMETER_OFFSET(EEPROM_MATERIAL_SETTINGS_MAX_COUNT), material.diameter);
}

#endif//ENABLE_ULTILCD2
