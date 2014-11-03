#ifndef DISPLAY_HD44780_SIM_H
#define DISPLAY_HD44780_SIM_H

#include "base.h"
#include "arduinoIO.h"

class displayHD44780Sim : public simBaseComponent
{
public:
    displayHD44780Sim(arduinoIOSim* arduinoIO, int rsPinNr, int enablePinNr, int d4PinNr, int d5PinNr, int d6PinNr, int d7PinNr);
    virtual ~displayHD44780Sim();

    virtual void draw(int x, int y);
private:
    int rsPin;
    int enablePin;
    int d4Pin;
    int d5Pin;
    int d6Pin;
    int d7Pin;
    
    bool readUpper;
    bool upperRS;
    int upperBits;

    bool writeData;
    int dataPos;
    char data[0x80];
    char customFontData[0x40];

    void enablePinUpdate(int pinNr, bool high);
};

#endif//STEPPER_SIM_H

