#include "heater.h"
#include "arduinoIO.h"

heaterSim::heaterSim(int heaterPinNr, adcSim* adc, int temperatureADCNr, float heaterStrength)
{
    this->heaterPinNr = heaterPinNr;
    this->adc = adc;
    this->temperatureADCNr = temperatureADCNr;
    this->heaterStrength = heaterStrength;

    this->temperature = 20;
}

heaterSim::~heaterSim()
{
}

void heaterSim::tick()
{
    if (readOutput(heaterPinNr))
        temperature += 0.03 * heaterStrength;
    else if (temperature > 20)
        temperature -= 0.03 * heaterStrength;
    adc->adcValue[temperatureADCNr] = 231 + temperature * 81 / 100;//Not accurate, but accurate enough.
}

void heaterSim::draw(int x, int y)
{
    char buffer[32] = {0};
    sprintf(buffer, "%iC", int(temperature));
    drawString(x, y, buffer, 0xFFFFFF);
}
