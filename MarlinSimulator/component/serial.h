#ifndef SERIAL_SIM_H
#define SERIAL_SIM_H

#include "base.h"

#define SERIAL_LINE_COUNT 30
class serialSim : public simBaseComponent
{
public:
    serialSim();
    virtual ~serialSim();
    
    virtual void draw(int x, int y);

private:
    int recvLine, recvPos;
    char recvBuffer[SERIAL_LINE_COUNT][80];
    
    void UART_UCSR0A_callback(uint8_t oldValue, uint8_t& newValue);
    void UART_UDR0_callback(uint8_t oldValue, uint8_t& newValue);
};

#endif//SERIAL_SIM_H
