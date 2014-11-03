#ifndef I2C_SIM_H
#define I2C_SIM_H

#include "base.h"

typedef delegate<uint8_t*, int> i2cMessageDelegate;

class i2cSim : public simBaseComponent
{
public:
    i2cSim();
    virtual ~i2cSim();
    
    void registerDevice(int id, i2cMessageDelegate func);
    
    void I2C_TWCR_callback(uint8_t oldValue, uint8_t& newValue);
private:
    int i2cMessagePos;
    uint8_t i2cMessage[2048];
    i2cMessageDelegate i2cDevice[256];
};

#endif//I2C_SIM_H
