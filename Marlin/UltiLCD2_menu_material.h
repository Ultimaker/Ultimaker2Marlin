#ifndef ULTI_LCD2_MENU_MATERIAL_H
#define ULTI_LCD2_MENU_MATERIAL_H

/*
EEPROM structure:
                   Location      Size
Total:             0x0000-0x1000
Settings:          0x0064-0x00E0 0x7C? (careful with this one)
FirstRunDone:      0x0400-0x0400 0x01
Tinker settings    0x0401-
RuntimeStats:      0x0700-0x071C 0x1C
Materials:         0x0800-0x09B1 (8+16)*18+1=0x1B1
ExtraTemperatures: 0x0a00-0x0C40 (16*18*2)=0x240
RetractionSettings:0x0c50-0x0FB0 (16*18*3)=0x360 Byte for retraction speed. Int16 for retraction length. 3 bytes per material+nozzle combo.
*/

//Introducing extra set of material temperatures, one for each possible nozzle.
//Maximum total nozzles configurations per material
#define MAX_MATERIAL_NOZZLE_CONFIGURATIONS 16
//Total material temperatures that are currently being used. (0.4, 0.25, 0.6, 0.8, 1.0)
#define MATERIAL_NOZZLE_COUNT 5

#define MATERIAL_NAME_SIZE 8

struct materialSettings
{
    int16_t temperature[MATERIAL_NOZZLE_COUNT];
    float retraction_length[MATERIAL_NOZZLE_COUNT];
    float retraction_speed[MATERIAL_NOZZLE_COUNT];
#if TEMP_SENSOR_BED != 0
    int16_t bed_temperature;
#endif
    uint8_t fan_speed; // 0-100% of requested speed by GCode
    int16_t flow;      // Flow modification in %
    float diameter;    // Filament diameter in mm
    char name[MATERIAL_NAME_SIZE+1];
};

extern struct materialSettings material[EXTRUDERS];

#define FILAMENT_FAST_STEPS  26500
#define FILAMENT_LONG_ACCELERATION_STEPS    7200

#define FILAMENT_REVERSAL_LENGTH      (FILAMENT_BOWDEN_LENGTH + 50)
#define FILAMENT_FORWARD_LENGTH       (FILAMENT_BOWDEN_LENGTH - 50)
#define FILAMENT_INSERT_SPEED         2     //Initial insert speed to grab the filament.
#define FILAMENT_INSERT_EXTRUDE_SPEED 1     //Final speed when extruding
#define FILAMENT_LONG_MOVE_JERK       1

#define EEPROM_RETRACTION_LENGTH_SCALE 256
#define EEPROM_RETRACTION_SPEED_SCALE 4

#define EEPROM_MATERIAL_SETTINGS_OFFSET 0x800
#define EEPROM_MATERIAL_EXTRA_TEMPERATURES_OFFSET 0xa00
#define EEPROM_MATERIAL_EXTRA_RETRACTION_SETTINGS_OFFSET 0xc50
#define EEPROM_MATERIAL_SETTINGS_MAX_COUNT 16
#define EEPROM_MATERIAL_SETTINGS_SIZE   (8 + 16)
#define EEPROM_MATERIAL_COUNT_OFFSET()            ((uint8_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 0))
#define EEPROM_MATERIAL_NAME_OFFSET(n)            ((uint8_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n)))
#define EEPROM_MATERIAL_TEMPERATURE_OFFSET(n)     ((uint16_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + MATERIAL_NAME_SIZE))
#define EEPROM_MATERIAL_EXTRA_TEMPERATURE_OFFSET(n, m)  ((uint16_t*)(EEPROM_MATERIAL_EXTRA_TEMPERATURES_OFFSET + MAX_MATERIAL_NOZZLE_CONFIGURATIONS * 2 * uint16_t(n) + 2 * uint16_t(m)))
#define EEPROM_MATERIAL_EXTRA_RETRACTION_LENGTH_OFFSET(n, m)  ((uint16_t*)(EEPROM_MATERIAL_EXTRA_RETRACTION_SETTINGS_OFFSET + MAX_MATERIAL_NOZZLE_CONFIGURATIONS * 3 * uint16_t(n) + 3 * uint16_t(m)))
#define EEPROM_MATERIAL_EXTRA_RETRACTION_SPEED_OFFSET(n, m)  ((uint8_t*)(EEPROM_MATERIAL_EXTRA_RETRACTION_SETTINGS_OFFSET + MAX_MATERIAL_NOZZLE_CONFIGURATIONS * 3 * uint16_t(n) + 3 * uint16_t(m) + 2))
#define EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(n) ((uint16_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + MATERIAL_NAME_SIZE + 2))
#define EEPROM_MATERIAL_FAN_SPEED_OFFSET(n)       ((uint8_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + MATERIAL_NAME_SIZE + 4))
#define EEPROM_MATERIAL_FLOW_OFFSET(n)            ((uint16_t*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + MATERIAL_NAME_SIZE + 5))
#define EEPROM_MATERIAL_DIAMETER_OFFSET(n)        ((float*)(EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * uint16_t(n) + MATERIAL_NAME_SIZE + 7))

void lcd_menu_material_main();
bool lcd_material_verify_material_settings();
void lcd_material_reset_defaults();
void lcd_material_set_material(uint8_t nr, uint8_t e);
void lcd_material_store_material(uint8_t nr);
void lcd_material_read_current_material();
void lcd_material_store_current_material();
void lcd_menu_change_material_preheat();
void lcd_menu_insert_material_preheat();
void lcd_material_change_init(bool printing);
void lcd_menu_material_main_return();
void lcd_menu_material_select();

// Oh yes, these totally do not belong here. But I need to put them somewhere.
// Anyhow, these functions convert a nozzle size to an index in the material-temperature setting array or vise-versa.
uint8_t nozzleSizeToTemperatureIndex(float nozzle_size);
float nozzleIndexToNozzleSize(uint8_t nozzle_index);

#endif//ULTI_LCD2_MENU_MATERIAL_H
