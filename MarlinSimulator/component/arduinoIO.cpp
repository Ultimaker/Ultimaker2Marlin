#include <Arduino.h>

#include "arduinoIO.h"

#define PA 1
#define PB 2
#define PC 3
#define PD 4
#define PE 5
#define PF 6
#define PG 7
#define PH 8
#define PJ 10
#define PK 11
#define PL 12

arduinoIOSim::arduinoIOSim()
{
    PORTA.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTA_callback));
    PORTB.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTB_callback));
    PORTC.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTC_callback));
    PORTD.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTD_callback));
    PORTE.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTE_callback));
    PORTF.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTF_callback));
    PORTG.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTG_callback));
    PORTH.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTH_callback));
    PORTJ.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTJ_callback));
    PORTK.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTK_callback));
    PORTL.setCallback(DELEGATE(registerDelegate, arduinoIOSim, *this, IO_PORTL_callback));
    
    for(unsigned int n=0; n<11*8; n++)
        portIdxToPinNr[n] = -1;
    for(unsigned int n=0; n<NUM_DIGITAL_PINS; n++)
    {
        if (digital_pin_to_port_PGM[n] == NOT_A_PORT)
            continue;
        int idx = digital_pin_to_port_PGM[n] * 8;
        int mask = digital_pin_to_bit_mask_PGM[n];
        for(unsigned int i=0;i<8;i++)
            if (mask == _BV(i)) idx += i;
        portIdxToPinNr[idx] = n;
    }
}
arduinoIOSim::~arduinoIOSim()
{
}

void arduinoIOSim::registerPortCallback(int portNr, ioDelegate func)
{
    if (portNr < 0 || portNr >= NUM_DIGITAL_PINS)
        return;
    ioWriteDelegate[portNr] = func;
}

void arduinoIOSim::checkPinChange(int portID, uint8_t oldValue, uint8_t newValue)
{
    uint8_t change = oldValue ^ newValue;
    if (change)
    {
        for(unsigned int i=0;i<8;i++)
        {
            if (change & _BV(i))
            {
                int pinNr = portIdxToPinNr[portID * 8 + i];
                if (pinNr >= 0)
                    ioWriteDelegate[pinNr](pinNr, newValue & _BV(i));
            }
        }
    }
}

void arduinoIOSim::IO_PORTA_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PA, oldValue, newValue);
}
void arduinoIOSim::IO_PORTB_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PB, oldValue, newValue);
}
void arduinoIOSim::IO_PORTC_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PC, oldValue, newValue);
}
void arduinoIOSim::IO_PORTD_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PD, oldValue, newValue);
}
void arduinoIOSim::IO_PORTE_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PE, oldValue, newValue);
}
void arduinoIOSim::IO_PORTF_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PF, oldValue, newValue);
}
void arduinoIOSim::IO_PORTG_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PG, oldValue, newValue);
}
void arduinoIOSim::IO_PORTH_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PH, oldValue, newValue);
}
void arduinoIOSim::IO_PORTJ_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PJ, oldValue, newValue);
}
void arduinoIOSim::IO_PORTK_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PK, oldValue, newValue);
}
void arduinoIOSim::IO_PORTL_callback(uint8_t oldValue, uint8_t& newValue)
{
    checkPinChange(PL, oldValue, newValue);
}

bool readOutput(int arduinoPinNr)
{
	uint8_t bit = digitalPinToBitMask(arduinoPinNr);
    uint8_t port = digitalPinToPort(arduinoPinNr);

	if (port == NOT_A_PORT) return false;

	AVRRegistor* out = portOutputRegister(port);

    return (*out) & bit;
}

void writeInput(int arduinoPinNr, bool value)
{
	uint8_t bit = digitalPinToBitMask(arduinoPinNr);
    uint8_t port = digitalPinToPort(arduinoPinNr);

	if (port == NOT_A_PIN) return;

	AVRRegistor* in = portInputRegister(port);
	if (value)
        (*in) |= bit;
    else
        (*in) &=~bit;
}
