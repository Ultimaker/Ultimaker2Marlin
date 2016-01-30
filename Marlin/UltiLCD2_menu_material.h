#ifndef ULTI_LCD2_MENU_MATERIAL_H
#define ULTI_LCD2_MENU_MATERIAL_H

struct materialSettings
{
    int16_t temperature;
#if TEMP_SENSOR_BED != 0
    int16_t bed_temperature;
#endif
    uint8_t fan_speed; //0-100% of requested speed by GCode
    int16_t flow;      //Flow modification in %
    float diameter; //Filament diameter in mm
    int16_t change_temperature;      //Temperature for the hotend during the change material procedure.
    int8_t change_preheat_wait_time; //when reaching the change material temperature, wait for this amount of seconds for the temperature to stabalize and the material to heatup.
};

extern struct materialSettings material[EXTRUDERS];

#define FILAMENT_REVERSAL_LENGTH      (FILAMANT_BOWDEN_LENGTH + 50)
#define FILAMENT_REVERSAL_SPEED       100
#define FILAMENT_LONG_MOVE_ACCELERATION 30

#define FILAMENT_FORWARD_LENGTH       (FILAMANT_BOWDEN_LENGTH - 50)
#define FILAMENT_INSERT_SPEED         2     //Initial insert speed to grab the filament.
#define FILAMENT_INSERT_FAST_SPEED    100   //Speed during the forward length
#define FILAMENT_INSERT_EXTRUDE_SPEED 2     //Final speed when extruding

#define EEPROM_MATERIAL_SETTINGS_OFFSET 0x800
#define EEPROM_MATERIAL_EXTRA_TEMPERATURES_OFFSET 0xa00
#define EEPROM_MATERIAL_CHANGE_TEMPERATURE_OFFSET 0xD00
#define EEPROM_MATERIAL_CHANGE_WAIT_TIME_OFFSET 0xD30
#define EEPROM_MATERIAL_SETTINGS_MAX_COUNT 16
#define EEPROM_MATERIAL_SETTINGS_SIZE   (8 + 16)
#define EEPROM_MATERIAL_COUNT_OFFSET()            ((uint8_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 0))
#define EEPROM_MATERIAL_NAME_OFFSET(n)            ((uint8_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n)))
#define EEPROM_MATERIAL_TEMPERATURE_OFFSET(n)     ((uint16_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + 8))
#define EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(n) ((uint16_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + 10))
#define EEPROM_MATERIAL_FAN_SPEED_OFFSET(n)       ((uint8_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + 12))
#define EEPROM_MATERIAL_FLOW_OFFSET(n)            ((uint16_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + 13))
#define EEPROM_MATERIAL_DIAMETER_OFFSET(n)        ((float*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + 15))
#define EEPROM_MATERIAL_CHANGE_TEMPERATURE(n)     ((uint16_t*)(EEPROM_MATERIAL_CHANGE_TEMPERATURE_OFFSET + uint16_t(n) * 2))
#define EEPROM_MATERIAL_CHANGE_WAIT_TIME(n)       ((uint8_t*)(EEPROM_MATERIAL_CHANGE_WAIT_TIME_OFFSET + uint16_t(n)))

void lcd_menu_material();
void lcd_change_to_menu_change_material(menuFunc_t return_menu);
void lcd_change_to_menu_insert_material(menuFunc_t return_menu);
bool lcd_material_verify_material_settings();
void lcd_material_reset_defaults();
void lcd_material_set_material(uint8_t nr, uint8_t e);
void lcd_material_store_material(uint8_t nr);
void lcd_material_read_current_material();
void lcd_material_store_current_material();

#endif//ULTI_LCD2_MENU_MATERIAL_H
