#ifndef LED_PCA9532_SIM_H
#define LED_PCA9532_SIM_H

#include "i2c.h"

class ledPCA9632Sim : public simBaseComponent
{
public:
    ledPCA9632Sim(i2cSim* i2c, int id = 0xC0);
    virtual ~ledPCA9632Sim();
    
    virtual void draw(int x, int y);
private:
    void processMessage(uint8_t* message, int length);

    int pwm[4];
    int ledout;
};


#endif//LED_PCA9532_SIM_H
