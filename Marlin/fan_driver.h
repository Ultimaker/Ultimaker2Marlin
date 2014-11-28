#ifndef FAN_DRIVER_H
#define FAN_DRIVER_H

#include <stdint.h>

void initFans();
void setCoolingFanSpeed(uint8_t fan_speed);
void setHotendCoolingFanSpeed(uint8_t fan_speed);

#endif
