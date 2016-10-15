#include "stepper.h"
#include "arduinoIO.h"

stepperSim::stepperSim(arduinoIOSim* arduinoIO, int enablePinNr, int stepPinNr, int dirPinNr, bool invertDir)
{
    this->minStepValue = -1;
    this->maxStepValue = -1;
    this->stepValue = 0;
    this->minEndstopPin = -1;
    this->maxEndstopPin = -1;

    this->invertDir = invertDir;
    this->enablePin = enablePinNr;
    this->stepPin = stepPinNr;
    this->dirPin = dirPinNr;

    arduinoIO->registerPortCallback(stepPinNr, DELEGATE(ioDelegate, stepperSim, *this, stepPinUpdate));
}
stepperSim::~stepperSim()
{
}

void stepperSim::stepPinUpdate(int pinNr, bool high)
{
    if (high)//Only step on high->low transition.
        return;
    if (readOutput(enablePin))
        return;
    if (readOutput(dirPin) == invertDir)
        stepValue --;
    else
        stepValue ++;
    if (minStepValue == -1)
        return;
    if (stepValue < minStepValue)
        stepValue = minStepValue;
    if (stepValue > maxStepValue)
        stepValue = maxStepValue;
    if (minEndstopPin > -1)
        writeInput(minEndstopPin, stepValue != minStepValue);
    if (maxEndstopPin > -1)
        writeInput(maxEndstopPin, stepValue != maxStepValue);
}

void stepperSim::setEndstops(int minEndstopPinNr, int maxEndstopPinNr)
{
    minEndstopPin = minEndstopPinNr;
    maxEndstopPin = maxEndstopPinNr;

    writeInput(minEndstopPin, stepValue != minStepValue);
    writeInput(maxEndstopPin, stepValue != maxStepValue);
}

void stepperSim::draw(int x, int y)
{
    char buffer[32] = {0};
    sprintf(buffer, "%i steps", int(stepValue));
    drawString(x, y, buffer, 0xFFFFFF);
}
