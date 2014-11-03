#ifndef HEATER_SIM_H
#define HEATER_SIM_H

#include "base.h"
#include "adc.h"

class heaterSim : public simBaseComponent
{
public:
    heaterSim(int heaterPinNr, adcSim* adc, int temperatureADCNr, float heaterStrength = 1.0);
    virtual ~heaterSim();
    
    virtual void tick();
    virtual void draw(int x, int y);
private:
    float temperature;
    float heaterStrength;

    int heaterPinNr;
    adcSim* adc;
    int temperatureADCNr;
};

#endif//I2C_SIM_H
