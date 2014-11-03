#include "display_HD44780.h"
#include "arduinoIO.h"

displayHD44780Sim::displayHD44780Sim(arduinoIOSim* arduinoIO, int rsPinNr, int enablePinNr, int d4PinNr, int d5PinNr, int d6PinNr, int d7PinNr)
{
    this->rsPin = rsPinNr;
    this->enablePin = enablePinNr;
    this->d4Pin = d4PinNr;
    this->d5Pin = d5PinNr;
    this->d6Pin = d6PinNr;
    this->d7Pin = d7PinNr;
    
    readUpper = true;
    writeData = true;
    dataPos = 0;
    memset(data, ' ', 0x80);
    arduinoIO->registerPortCallback(enablePinNr, DELEGATE(ioDelegate, displayHD44780Sim, *this, enablePinUpdate));
}
displayHD44780Sim::~displayHD44780Sim()
{
}

void displayHD44780Sim::draw(int x, int y)
{
    int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    for(unsigned int y=0; y<4; y++)
    {
        for(unsigned int x=0; x<20; x++)
        {
            char c = data[x+row_offsets[y]];
            
            drawRect(x* 6, y * 9, 5, 8, 0x202080);
            
            if (c < 8)
            {
                for(unsigned int m=0; m<8; m++)
                {
                    int bits = customFontData[c * 8 + m];
                    for(unsigned int n=0; n<5; n++)
                    {
                        if (bits & _BV(4-n))
                            drawRect(x * 6 + n, y * 9 + m, 1, 1, 0xAAAAEE);
                    }
                }
            }else{
                drawChar(x * 6, y * 9, c, 0xAAAAEE);
            }
        }
    }
}

void displayHD44780Sim::enablePinUpdate(int pinNr, bool high)
{
    if (!high) return;
    
    int n = 0;
    if (readOutput(d4Pin)) n |= _BV(0);
    if (readOutput(d5Pin)) n |= _BV(1);
    if (readOutput(d6Pin)) n |= _BV(2);
    if (readOutput(d7Pin)) n |= _BV(3);
    bool rs = readOutput(rsPin);
    
    if (readUpper || rs != upperRS)
    {
        upperBits = n;
        readUpper = false;
        upperRS = rs;
        return;
    }
    readUpper = true;
    
    n |= upperBits << 4;
    
    if (!rs)
    {
        if (n == 0x01)//LCD_CLEARDISPLAY
        {
            memset(data, ' ', 0x80);
            dataPos = 0;
            writeData = true;
        }else if (n == 0x02)//LCD_RETURNHOME
        {
            dataPos = 0;
            writeData = true;
        }else if ((n & 0xFC) == 0x04)//LCD_ENTRYMODESET
        {
        }else if ((n & 0xF8) == 0x08)//LCD_DISPLAYCONTROL
        {
        }else if ((n & 0xF0) == 0x10)//LCD_CURSORSHIFT
        {
        }else if ((n & 0xE0) == 0x20)//LCD_FUNCTIONSET
        {
        }else if ((n & 0xC0) == 0x40)//LCD_SETCGRAMADDR
        {
            dataPos = (n & 0x3F);
            writeData = false;
        }else if (n & 0x80)//LCD_SETDDRAMADDR
        {
            dataPos = (n & 0x7F);
            writeData = true;
        }else{
            printf("Unknown HD44780 command: %02x\n", n);
        }
    }else{
        if (writeData)
        {
            data[dataPos] = n;
            dataPos = (dataPos + 1) % 0x80;
        }else{
            customFontData[dataPos] = n;
            dataPos = (dataPos + 1) % 0x40;
        }
    }
}
