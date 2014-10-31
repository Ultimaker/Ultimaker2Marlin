#include <avr/io.h>

#include "led_PCA9632.h"

ledPCA9632Sim::ledPCA9632Sim(i2cSim* i2c, int id)
{
    i2c->registerDevice(id, DELEGATE(i2cMessageDelegate, ledPCA9632Sim, *this, processMessage));
}

ledPCA9632Sim::~ledPCA9632Sim()
{
}

void ledPCA9632Sim::processMessage(uint8_t* message, int length)
{
    int regAddr = message[1] & 0x0F;
    for(int n=2; n<length; n++)
    {
        switch(regAddr)
        {
        case 0x00://MODE1
            break;
        case 0x01://MODE2
            break;
        case 0x02://PWM0
            pwm[0] = message[n];
            break;
        case 0x03://PWM1
            pwm[1] = message[n];
            break;
        case 0x04://PWM2
            pwm[2] = message[n];
            break;
        case 0x05://PWM3
            pwm[3] = message[n];
            break;
        case 0x06://GRPPWM
            break;
        case 0x07://GRPFREQ
            break;
        case 0x08://LEDOUT
            ledout = message[n];
            break;
        default:
            printf("LED: %02x: %02x\n", regAddr, message[n]);
            break;
        }
        
        if (message[1] & 0x80)
        {
            regAddr++;
            if (regAddr > 0x0C)
                regAddr = 0;
        }
    }
}

void ledPCA9632Sim::draw(int x, int y)
{
    int r=0,g=0,b=0;
    if ((ledout & 0x03) == 1) r = 0xff;
    if ((ledout & 0x03) == 2) r = pwm[0];
    if (((ledout >> 2) & 0x03) == 1) g = 0xff;
    if (((ledout >> 2) & 0x03) == 2) g = pwm[1];
    if (((ledout >> 4) & 0x03) == 1) b = 0xff;
    if (((ledout >> 4) & 0x03) == 2) b = pwm[2];
    drawRect(x, y, 128, 3, r << 16 | g << 8 | b);
}
