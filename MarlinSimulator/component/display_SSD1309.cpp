#include <avr/io.h>

#include "display_SSD1309.h"

displaySDD1309Sim::displaySDD1309Sim(i2cSim* i2c, int id)
{
    i2c->registerDevice(id, DELEGATE(i2cMessageDelegate, displaySDD1309Sim, *this, processMessage));
    lcd_data_pos = 0;
}

displaySDD1309Sim::~displaySDD1309Sim()
{
}

void displaySDD1309Sim::processMessage(uint8_t* message, int length)
{
    if (message[1] == 0x40)
    {
        //Data
        for(int n=2;n<length;n++)
        {
            lcd_data[lcd_data_pos] = message[n];
            lcd_data_pos = (lcd_data_pos + 1) % 1024;
        }
    }else if (message[1] == 0x00)
    {
        //Command
        for(int n=2;n<length;n++)
        {
            if ((message[n] & 0xF0) == 0x00)
            {
                lcd_data_pos = (lcd_data_pos & 0xFF0) | (message[n] & 0x0F);
            }else if ((message[n] & 0xF0) == 0x10)
            {
                lcd_data_pos = (lcd_data_pos & 0xF8F) | ((message[n] & 0x07) << 4);
            }else if ((message[n] & 0xF0) == 0xB0)
            {
                lcd_data_pos = (lcd_data_pos & 0x07F) | ((message[n] & 0x0F) << 7);
            }else if (message[n] == 0x20) { /*LCD_COMMAND_SET_ADDRESSING_MODE*/
            }else if (message[n] == 0x40) { /*Set start line*/
            }else if (message[n] == 0x81) { /*LCD_COMMAND_CONTRAST*/ n++;
            }else if (message[n] == 0xA1) { /*Segment remap*/
            }else if (message[n] == 0xA4) { /*LCD_COMMAND_FULL_DISPLAY_ON_DISABLE*/
            }else if (message[n] == 0xA5) { /*LCD_COMMAND_FULL_DISPLAY_ON_ENABLE*/
            }else if (message[n] == 0xA8) { /*Multiplex ratio*/ n++;
            }else if (message[n] == 0xC8) { /*COM scan output direction*/
            }else if (message[n] == 0xD3) { /*Display offset*/ n++;
            }else if (message[n] == 0xD5) { /*Display clock divider/freq*/ n++;
            }else if (message[n] == 0xD9) { /*Precharge period*/ n++;
            }else if (message[n] == 0xDA) { /*COM pins hardware configuration*/ n++;
            }else if (message[n] == 0xDB) { /*VCOMH Deslect level*/ n++;
            }else if (message[n] == 0xFD) { /*LCD_COMMAND_LOCK_COMMANDS*/ n++;
            }else if (message[n] == 0xAE) { //Display OFF
            }else if (message[n] == 0xAF) { //Display ON
            }else{
                printf("Unknown LCD CMD: %02x\n", message[n]);
            }
        }
    }
}


#define LCD_GFX_WIDTH 128
#define LCD_GFX_HEIGHT 64
void displaySDD1309Sim::draw(int x, int y)
{
    drawRect(x, y, LCD_GFX_WIDTH + 2, LCD_GFX_HEIGHT + 2, 0xFFFFFF);
    drawRect(x+1, y+1, LCD_GFX_WIDTH, LCD_GFX_HEIGHT, 0x101010);
    for(int _y=0;_y<64;_y++)
    {
        for(int _x=0;_x<128;_x++)
        {
            if (lcd_data[_x + (_y/8) * LCD_GFX_WIDTH] & _BV(_y%8))
            {
                drawRect(x+_x+1, y+_y+1, 1, 1, 0x8080FF);
                drawRect(x+_x+1, y+_y+1, 0, 1, 0x8080AF);
                drawRect(x+_x+1, y+_y+1, 1, 0, 0x8080AF);
            }else{
                drawRect(x+_x+1, y+_y+1, 0, 1, 0x202020);
                drawRect(x+_x+1, y+_y+1, 1, 0, 0x202020);
            }
        }
    }
/*
    SDL_LockSurface(screen);
    for(int y=0;y<64;y++)
    {
        for(int _y=0;_y<DRAW_SCALE;_y++)
        {
            uint32_t* pix = ((uint32_t*)screen->pixels) + screen->pitch / 4 * (y * DRAW_SCALE + _y + DRAW_SCALE);
            for(int _n=0;_n<DRAW_SCALE;_n++)
                *pix++ = 0xFFFFFF;
            for(int x=0;x<128;x++)
            {
                for(int _n=0;_n<DRAW_SCALE;_n++)
                {
                    if (_n == 0 || _y == 0)
                        *pix++ = (lcd_data[x + (y/8) * LCD_GFX_WIDTH] & _BV(y%8)) ? 0x8080AF : 0x101010;
                    else
                        *pix++ = (lcd_data[x + (y/8) * LCD_GFX_WIDTH] & _BV(y%8)) ? 0x8080FF : 0x202020;
                }
            }
            for(int _n=0;_n<DRAW_SCALE;_n++)
                *pix++ = 0xFFFFFF;
        }
    }
    for(int _y=0;_y<DRAW_SCALE;_y++)
    {
        uint32_t* pix = ((uint32_t*)screen->pixels) + screen->pitch / 4 * (_y);
        uint32_t* pix2 = ((uint32_t*)screen->pixels) + screen->pitch / 4 * (64 * DRAW_SCALE + _y + DRAW_SCALE);
        for(int x=0;x<130;x++)
            for(int _n=0;_n<DRAW_SCALE;_n++)
            {
                *pix++ = 0xFFFFFF;
                *pix2++ = 0xFFFFFF;
            }
    }

    SDL_UnlockSurface(screen);
*/
}
