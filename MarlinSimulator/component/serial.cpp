#include <avr/io.h>
#include <string.h>

#include "serial.h"

serialSim::serialSim()
{
    UCSR0A.setCallback(DELEGATE(registerDelegate, serialSim, *this, UART_UCSR0A_callback));
    UDR0.setCallback(DELEGATE(registerDelegate, serialSim, *this, UART_UDR0_callback));

    UCSR0A = 0;
    
    recvLine = 0;
    recvPos = 0;
    memset(recvBuffer, '\0', sizeof(recvBuffer));
}

serialSim::~serialSim()
{
}

void serialSim::UART_UCSR0A_callback(uint8_t oldValue, uint8_t& newValue)
{
    //Always mark "write ready" flag, so the serial code never waits.
    newValue |= _BV(UDRE0);
}
void serialSim::UART_UDR0_callback(uint8_t oldValue, uint8_t& newValue)
{
    recvBuffer[recvLine][recvPos] = newValue;
    recvPos++;
    if (recvPos == 80 || newValue == '\n')
    {
        recvPos = 0;
        recvLine++;
        if (recvLine == SERIAL_LINE_COUNT)
        {
            for(unsigned int n=0; n<SERIAL_LINE_COUNT-1;n++)
                memcpy(recvBuffer[n], recvBuffer[n+1], 80);
            recvLine--;
            memset(recvBuffer[recvLine], '\0', 80);
        }
    }
}

void serialSim::draw(int x, int y)
{
    for(unsigned int n=0; n<SERIAL_LINE_COUNT;n++)
        drawStringSmall(x, y+n*3, recvBuffer[n], 0xFFFFFF);
}
