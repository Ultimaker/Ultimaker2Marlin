#ifndef TEMPERATURE_ADS101X
#define TEMPERATURE_ADS101X

#include <stdint.h>

void initTemperatureADS101X();
void temperatureADS101XSetupAIN0();
void temperatureADS101XSetupAIN1();
void temperatureADS101XRequestResult();
int16_t temperatureADS101XGetResult();

#endif//TEMPERATURE_ADS101X
