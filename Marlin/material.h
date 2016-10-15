/*
  material.h - material settings
  Part of Marlin

  Copyright (c) 2016 Robert Diamond

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef __MATERIAL_H
#define __MATERIAL_H

#include "Configuration.h"  // for EXTRUDERS

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

    void set_material_from_eeprom(uint8_t nr);
};

extern struct materialSettings material[EXTRUDERS];

#endif  // __MATERIAL_H
