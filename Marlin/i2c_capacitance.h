#ifndef I2C_CAPACITANCE_H
#define I2C_CAPACITANCE_H

#include <stdint.h>

void i2cCapacitanceInit();
void i2cCapacitanceStart();
bool i2cCapacitanceDone(uint16_t& value);

#endif//I2C_CAPACITANCE_H