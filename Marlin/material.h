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

    void set_material_from_eeprom(int nr);
};

extern struct materialSettings material[EXTRUDERS];

#endif  // __MATERIAL_H
