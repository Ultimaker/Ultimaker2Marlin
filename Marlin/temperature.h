/*
  temperature.h - temperature controller
  Part of Marlin

  Copyright (c) 2011 Erik van der Zalm

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

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "Marlin.h"
#include "planner.h"
#ifdef PID_ADD_EXTRUSION_RATE
  #include "stepper.h"
#endif

// public functions
void tp_init();  //initialise the heating
void manage_heater(); //it is critical that this is called periodically.

// low level conversion routines
// do not use these routines and variables outside of temperature.cpp
// THESE ARE NO LONGER CHANGED, LEFT PURELY FOR BACKWARDS COMPATIBILITY
extern int target_temperature[EXTRUDERS];
extern float current_temperature[EXTRUDERS];
extern int target_temperature_bed;
extern float current_temperature_bed;
#ifdef TEMP_SENSOR_1_AS_REDUNDANT
  extern float redundant_temperature;
#endif

#ifdef PIDTEMP
  extern float Kp,Ki,Kd,Kc;
  float scalePID_i(float i);
  float scalePID_d(float d);
  float unscalePID_i(float i);
  float unscalePID_d(float d);

#endif
#ifdef PIDTEMPBED
  extern float bedKp,bedKi,bedKd;
#endif

//high level conversion routines, for use outside of temperature.cpp
//inline so that there is no performance decrease.
//deg=degreeCelsius

extern float _temp_target[EXTRUDERS * 2];
extern float _temp_current[EXTRUDERS * 2];

#define TEMP_SYRINGE 0
#define TEMP_NEEDLE 1

FORCE_INLINE uint8_t getTempId(uint8_t extruder, uint8_t heater) {
  return extruder * 2 + heater;
}

FORCE_INLINE float degHotend(uint8_t extruder) {
  return current_temperature[extruder];
};

FORCE_INLINE float degTargetHotend(uint8_t extruder) {
  return target_temperature[extruder];
};

FORCE_INLINE void setTargetHotend(const float &celsius, uint8_t extruder) {
  target_temperature[extruder] = celsius;
  if (target_temperature[extruder] >= HEATER_0_MAXTEMP - 15)
    target_temperature[extruder] = HEATER_0_MAXTEMP - 15;
};

void disable_all_heaters();
void updatePID();

void PID_autotune(float temp, int extruder, int ncycles);

#endif  // TEMPERATURE_H

