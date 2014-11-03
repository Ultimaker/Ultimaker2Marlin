#include <avr/io.h>

#include "i2c.h"

i2cSim::i2cSim()
{
    TWCR.setCallback(DELEGATE(registerDelegate, i2cSim, *this, I2C_TWCR_callback));
}

i2cSim::~i2cSim()
{
}

void i2cSim::I2C_TWCR_callback(uint8_t oldValue, uint8_t& newValue)
{
    if ((oldValue ^ newValue) == _BV(TWIE) && (newValue & _BV(TWIE)))
        return;
    if ((newValue & _BV(TWINT)) && (newValue & _BV(TWEN)))
    {
        if (newValue & _BV(TWSTA))
        {
            if (i2cMessagePos > 0)
            {
                if (i2cDevice[i2cMessage[0]])
                    i2cDevice[i2cMessage[0]](i2cMessage, i2cMessagePos);
                else
                    printf("i2c message to unknown device ID: %02x\n", i2cMessage[0]);
            }
            i2cMessagePos = 0;
            i2cMessage[0] = 0xFF;
            TWSR = 0x08;
        } else if (newValue & _BV(TWSTO))
        {
            if (i2cDevice[i2cMessage[0]])
                i2cDevice[i2cMessage[0]](i2cMessage, i2cMessagePos);
            else
                printf("i2c message to unknown device ID: %02x\n", i2cMessage[0]);
            i2cMessagePos = 0;
            newValue &=~_BV(TWINT);
            newValue &=~_BV(TWSTO);
            TWSR = 0x00;
        }else{
            i2cMessage[i2cMessagePos] = TWDR;
            i2cMessagePos++;
            TWDR = 0x00;
            if (TWSR == 0x08)
                TWSR = 0x18;
            else if (TWSR == 0x18)
                TWSR = 0x28;
        }
    }
}

void i2cSim::registerDevice(int id, i2cMessageDelegate func)
{
    if (id < 0 || id > 255)
        return;
    i2cDevice[id] = func;
}
