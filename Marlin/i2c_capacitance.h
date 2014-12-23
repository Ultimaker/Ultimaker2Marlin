#ifndef I2C_CAPACITANCE_H
#define I2C_CAPACITANCE_H

#include <stdint.h>

#ifdef ENABLE_BED_LEVELING_PROBE

void i2cCapacitanceInit();
void i2cCapacitanceStart();
bool i2cCapacitanceDone(uint16_t& value);

#endif//ENABLE_BED_LEVELING_PROBE

#endif//I2C_CAPACITANCE_H
