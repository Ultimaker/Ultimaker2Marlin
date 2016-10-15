#ifndef FILAMENT_SENSOR_H
#define FILAMENT_SENSOR_H

#include "Configuration.h"
#include "pins.h"

// @NEB filament outage handling
void filament_sensor_init();
bool checkFilamentSensor();
void lcd_menu_filament_outage();
// @NEB

#endif //FILAMENT_SENSOR_H
