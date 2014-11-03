#ifndef STEPPER_SIM_H
#define STEPPER_SIM_H

#include "base.h"
#include "arduinoIO.h"

class stepperSim : public simBaseComponent
{
private:
    int minStepValue;
    int maxStepValue;
    int stepValue;
    bool invertDir;
    int enablePin, stepPin, dirPin;
    int minEndstopPin, maxEndstopPin;
public:
    stepperSim(arduinoIOSim* arduinoIO, int enablePinNr, int stepPinNr, int dirPinNr, bool invertDir);
    virtual ~stepperSim();
    
    virtual void draw(int x, int y);
    
    void setRange(int minValue, int maxValue) { minStepValue = minValue; maxStepValue = maxValue; stepValue = (maxValue + minValue) / 2; }
    void setEndstops(int minEndstopPinNr, int maxEndstopPinNr);
    int getPosition() { return stepValue; }
private:
    void stepPinUpdate(int pinNr, bool high);
};

#endif//STEPPER_SIM_H
