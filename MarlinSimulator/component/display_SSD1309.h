#ifndef DISPLAY_SSD1309_SIM_H
#define DISPLAY_SSD1309_SIM_H

#include "i2c.h"

class displaySDD1309Sim : public simBaseComponent
{
public:
    displaySDD1309Sim(i2cSim* i2c, int id = 0x78);
    virtual ~displaySDD1309Sim();
    
    virtual void draw(int x, int y);
private:
    void processMessage(uint8_t* message, int length);

    int lcd_data_pos;
    uint8_t lcd_data[1024];
};

#endif//DISPLAY_SSD1309_SIM_H
