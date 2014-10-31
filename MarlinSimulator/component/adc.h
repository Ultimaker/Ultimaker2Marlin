#ifndef ADC_SIM_H
#define ADC_SIM_H

#include "base.h"

class adcSim : public simBaseComponent
{
public:
    adcSim();
    virtual ~adcSim();
    
    int adcValue[16];
    
private:
    void ADC_ADCSRA_callback(uint8_t oldValue, uint8_t& newValue);
};

#endif//I2C_SIM_H
