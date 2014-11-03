#include <avr/io.h>

#include "adc.h"

adcSim::adcSim()
{
    ADCSRA.setCallback(DELEGATE(registerDelegate, adcSim, *this, ADC_ADCSRA_callback));
    for(unsigned int n=0;n<16;n++)
        adcValue[n] = 0;
}

adcSim::~adcSim()
{
}

void adcSim::ADC_ADCSRA_callback(uint8_t oldValue, uint8_t& newValue)
{
    if ((newValue & _BV(ADEN)) && (newValue & _BV(ADSC)))
    {   //Start ADC conversion
        int idx = ADMUX & (_BV(MUX4)|_BV(MUX3)|_BV(MUX2)|_BV(MUX1)|_BV(MUX0));
        if (ADCSRB & _BV(MUX5))
            idx += 8;
        
        ADC = adcValue[idx];
    }
}
