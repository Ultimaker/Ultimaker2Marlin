#include "Configuration.h"
#include "pins.h"
#include "UltiLCD2_low_lib.h"
#include "i2c_driver.h"

#ifdef ENABLE_ULTILCD2
/**
 * Implementation of the LCD display routines for a SSD1309 OLED graphical display connected with i2c.
 **/
#define LCD_GFX_WIDTH 128
#define LCD_GFX_HEIGHT 64

#define LCD_RESET_PIN 5
#define LCD_CS_PIN    6

#define I2C_LED_ADDRESS 0b1100000

#define I2C_LCD_ADDRESS 0b0111100
#define I2C_LCD_SEND_COMMAND 0x00
#define I2C_LCD_SEND_DATA    0x40

#define LCD_COMMAND_CONTRAST                0x81
#define LCD_COMMAND_FULL_DISPLAY_ON_DISABLE 0xA4
#define LCD_COMMAND_FULL_DISPLAY_ON_ENABLE  0xA5
#define LCD_COMMAND_INVERT_DISABLE          0xA6
#define LCD_COMMAND_INVERT_ENABLE           0xA7
#define LCD_COMMAND_DISPLAY_OFF             0xAE
#define LCD_COMMAND_DISPLAY_ON              0xAF
#define LCD_COMMAND_NOP                     0xE3
#define LCD_COMMAND_LOCK_COMMANDS           0xFD

#define LCD_COMMAND_SET_ADDRESSING_MODE     0x20

/** Backbuffer for LCD */
static i2cCommand led_command;
static uint8_t led_command_buffer[4];

static i2cCommand lcd_position_command;
static uint8_t lcd_position_command_buffer[4];

static uint8_t lcd_buffer[(1+LCD_GFX_WIDTH) * LCD_GFX_HEIGHT / 8];
static i2cCommand lcd_data_command[LCD_GFX_HEIGHT / 8];

void lcd_lib_init()
{
    i2cCommand lcd_init_command;
    uint8_t lcd_init_buffer[24];
    i2cCommand led_init_command;
    uint8_t led_init_buffer[10];

    i2cDriverCommandSetup(led_command, I2C_LED_ADDRESS << 1 | i2cWriteBit, 1, led_command_buffer, sizeof(led_command_buffer));
    i2cDriverCommandSetup(lcd_position_command, I2C_LCD_ADDRESS << 1 | i2cWriteBit, 11, lcd_position_command_buffer, sizeof(lcd_position_command_buffer));
    for(uint8_t n=0; n<LCD_GFX_HEIGHT / 8; n++)
        i2cDriverCommandSetup(lcd_data_command[n], I2C_LCD_ADDRESS << 1 | i2cWriteBit, 0, &lcd_buffer[(LCD_GFX_WIDTH + 1) * n], LCD_GFX_WIDTH + 1);

    i2cDriverCommandSetup(lcd_init_command, I2C_LCD_ADDRESS << 1 | i2cWriteBit, 1, lcd_init_buffer, sizeof(lcd_init_buffer));
    i2cDriverCommandSetup(led_init_command, I2C_LED_ADDRESS << 1 | i2cWriteBit, 1, led_init_buffer, sizeof(led_init_buffer));

    SET_OUTPUT(LCD_CS_PIN);
    SET_OUTPUT(LCD_RESET_PIN);

    //Set the beeper as output.
    SET_OUTPUT(BEEPER);
    
    //Set the encoder bits and encoder button as inputs with pullup
    SET_INPUT(BTN_EN1);
    SET_INPUT(BTN_EN2);
    SET_INPUT(BTN_ENC);
    WRITE(BTN_EN1, 1);
    WRITE(BTN_EN2, 1);
    WRITE(BTN_ENC, 1);

    SET_INPUT(SDCARDDETECT);
    WRITE(SDCARDDETECT, HIGH);

    WRITE(LCD_CS_PIN, 0);

    WRITE(LCD_RESET_PIN, 0);
    _delay_ms(1);
    WRITE(LCD_RESET_PIN, 1);
    _delay_ms(1);
    
    led_init_buffer[0] = 0x80;//Write from address 0 with auto-increase.
    led_init_buffer[1] = 0x80;//MODE1
    led_init_buffer[2] = 0x1C;//MODE2
    led_init_buffer[3] = 0;//PWM0=Red
    led_init_buffer[4] = 0;//PWM1=Green
    led_init_buffer[5] = 0;//PWM2=Blue
    led_init_buffer[6] = 0x00;//PWM3
    led_init_buffer[7] = 0xFF;//GRPPWM
    led_init_buffer[8] = 0x00;//GRPFREQ
    led_init_buffer[9] = 0xAA;//LEDOUT
    i2cDriverExecuteAndWait(&led_init_command);
    
    lcd_init_buffer[0] = I2C_LCD_SEND_COMMAND;
    lcd_init_buffer[1] = LCD_COMMAND_LOCK_COMMANDS;
    lcd_init_buffer[2] = 0x12;
    lcd_init_buffer[3] = LCD_COMMAND_DISPLAY_OFF;
    lcd_init_buffer[4] = 0xD5;//Display clock divider/freq;
    lcd_init_buffer[5] = 0xA0;
    lcd_init_buffer[6] = 0xA8;//Multiplex ratio
    lcd_init_buffer[7] = 0x3F;
    lcd_init_buffer[8] = 0xD3;//Display offset
    lcd_init_buffer[9] = 0x00;
    lcd_init_buffer[10] = 0x40;//Set start line
    lcd_init_buffer[11] = 0xA1;//Segment remap;
    lcd_init_buffer[12] = 0xC8;//COM scan output direction;
    lcd_init_buffer[13] = 0xDA;//COM pins hardware configuration;
    lcd_init_buffer[14] = 0x12;
    lcd_init_buffer[15] = LCD_COMMAND_CONTRAST;
    lcd_init_buffer[16] = 0xDF;
    lcd_init_buffer[17] = 0xD9;//Pre charge period
    lcd_init_buffer[18] = 0x82;
    lcd_init_buffer[19] = 0xDB;//VCOMH Deslect level
    lcd_init_buffer[20] = 0x34;
    lcd_init_buffer[21] = LCD_COMMAND_SET_ADDRESSING_MODE;
    lcd_init_buffer[22] = LCD_COMMAND_FULL_DISPLAY_ON_DISABLE;
    lcd_init_buffer[23] = LCD_COMMAND_DISPLAY_ON;
    i2cDriverExecuteAndWait(&lcd_init_command);

    lcd_lib_buttons_update_interrupt();
    lcd_lib_buttons_update();
    lcd_lib_encoder_pos = 0;
    lcd_lib_update_screen();
}

void lcd_lib_update_screen()
{
    //Set the drawin position to 0,0
    lcd_position_command_buffer[0] = I2C_LCD_SEND_COMMAND;
    lcd_position_command_buffer[1] = 0x00 | (0 & 0x0F);
    lcd_position_command_buffer[2] = 0x10 | (0 >> 4);
    lcd_position_command_buffer[3] = 0xB0 | 0;
    i2cDriverPlan(&lcd_position_command);

    //Send all the display data, LCD_GFX_WIDTHx8 pixels at a time. Each block takes 3.225 milliseconds to send.
    for(uint8_t n=0; n<LCD_GFX_HEIGHT / 8; n++)
    {
        lcd_buffer[n*(LCD_GFX_WIDTH+1)] = I2C_LCD_SEND_DATA;
        i2cDriverPlan(&lcd_data_command[n]);
    }
}

bool lcd_lib_update_ready()
{
    if (!lcd_position_command.finished)
        return false;
    for(uint8_t n=0; n<LCD_GFX_HEIGHT / 8; n++)
        if (!lcd_data_command[n].finished)
            return false;
    return true;
}

void lcd_lib_led_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (!led_command.finished)
        return;

    led_command_buffer[0] = 0x82;//Start at address 2 with auto-increase enabled.
    led_command_buffer[1] = r;//PWM0
    led_command_buffer[2] = g;//PWM1
    led_command_buffer[3] = b;//PWM2
    i2cDriverPlan(&led_command);
}

static const uint8_t lcd_font_7x5[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x00,// (space)
	0x00, 0x00, 0x5F, 0x00, 0x00,// !
	0x00, 0x07, 0x00, 0x07, 0x00,// "
	0x14, 0x7F, 0x14, 0x7F, 0x14,// #
	0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
	0x23, 0x13, 0x08, 0x64, 0x62,// %
	0x36, 0x49, 0x55, 0x22, 0x50,// &
	0x00, 0x05, 0x03, 0x00, 0x00,// '
	0x00, 0x1C, 0x22, 0x41, 0x00,// (
	0x00, 0x41, 0x22, 0x1C, 0x00,// )
	0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
	0x08, 0x08, 0x3E, 0x08, 0x08,// +
	0x00, 0x50, 0x30, 0x00, 0x00,// ,
	0x08, 0x08, 0x08, 0x08, 0x08,// -
	0x00, 0x60, 0x60, 0x00, 0x00,// .
	0x20, 0x10, 0x08, 0x04, 0x02,// /
	0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
	0x00, 0x42, 0x7F, 0x40, 0x00,// 1
	0x42, 0x61, 0x51, 0x49, 0x46,// 2
	0x21, 0x41, 0x45, 0x4B, 0x31,// 3
	0x18, 0x14, 0x12, 0x7F, 0x10,// 4
	0x27, 0x45, 0x45, 0x45, 0x39,// 5
	0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
	0x01, 0x71, 0x09, 0x05, 0x03,// 7
	0x36, 0x49, 0x49, 0x49, 0x36,// 8
	0x06, 0x49, 0x49, 0x29, 0x1E,// 9
	0x00, 0x36, 0x36, 0x00, 0x00,// :
	0x00, 0x56, 0x36, 0x00, 0x00,// ;
	0x00, 0x08, 0x14, 0x22, 0x41,// <
	0x14, 0x14, 0x14, 0x14, 0x14,// =
	0x41, 0x22, 0x14, 0x08, 0x00,// >
	0x02, 0x01, 0x51, 0x09, 0x06,// ?
	0x32, 0x49, 0x79, 0x41, 0x3E,// @
	0x7E, 0x11, 0x11, 0x11, 0x7E,// A
	0x7F, 0x49, 0x49, 0x49, 0x36,// B
	0x3E, 0x41, 0x41, 0x41, 0x22,// C
	0x7F, 0x41, 0x41, 0x22, 0x1C,// D
	0x7F, 0x49, 0x49, 0x49, 0x41,// E
	0x7F, 0x09, 0x09, 0x01, 0x01,// F
	0x3E, 0x41, 0x41, 0x51, 0x32,// G
	0x7F, 0x08, 0x08, 0x08, 0x7F,// H
	0x00, 0x41, 0x7F, 0x41, 0x00,// I
	0x20, 0x40, 0x41, 0x3F, 0x01,// J
	0x7F, 0x08, 0x14, 0x22, 0x41,// K
	0x7F, 0x40, 0x40, 0x40, 0x40,// L
	0x7F, 0x02, 0x04, 0x02, 0x7F,// M
	0x7F, 0x04, 0x08, 0x10, 0x7F,// N
	0x3E, 0x41, 0x41, 0x41, 0x3E,// O
	0x7F, 0x09, 0x09, 0x09, 0x06,// P
	0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
	0x7F, 0x09, 0x19, 0x29, 0x46,// R
	0x46, 0x49, 0x49, 0x49, 0x31,// S
	0x01, 0x01, 0x7F, 0x01, 0x01,// T
	0x3F, 0x40, 0x40, 0x40, 0x3F,// U
	0x1F, 0x20, 0x40, 0x20, 0x1F,// V
	0x7F, 0x20, 0x18, 0x20, 0x7F,// W
	0x63, 0x14, 0x08, 0x14, 0x63,// X
	0x03, 0x04, 0x78, 0x04, 0x03,// Y
	0x61, 0x51, 0x49, 0x45, 0x43,// Z
	0x00, 0x00, 0x7F, 0x41, 0x41,// [
	0x02, 0x04, 0x08, 0x10, 0x20,// "\"
	0x41, 0x41, 0x7F, 0x00, 0x00,// ]
	0x04, 0x02, 0x01, 0x02, 0x04,// ^
	0x40, 0x40, 0x40, 0x40, 0x40,// _
	0x00, 0x01, 0x02, 0x04, 0x00,// `
	0x20, 0x54, 0x54, 0x54, 0x78,// a
	0x7F, 0x48, 0x44, 0x44, 0x38,// b
	0x38, 0x44, 0x44, 0x44, 0x20,// c
	0x38, 0x44, 0x44, 0x48, 0x7F,// d
	0x38, 0x54, 0x54, 0x54, 0x18,// e
	0x08, 0x7E, 0x09, 0x01, 0x02,// f
	0x08, 0x14, 0x54, 0x54, 0x3C,// g
	0x7F, 0x08, 0x04, 0x04, 0x78,// h
	0x00, 0x44, 0x7D, 0x40, 0x00,// i
	0x20, 0x40, 0x44, 0x3D, 0x00,// j
	0x00, 0x7F, 0x10, 0x28, 0x44,// k
	0x00, 0x41, 0x7F, 0x40, 0x00,// l
	0x7C, 0x04, 0x18, 0x04, 0x78,// m
	0x7C, 0x08, 0x04, 0x04, 0x78,// n
	0x38, 0x44, 0x44, 0x44, 0x38,// o
	0x7C, 0x14, 0x14, 0x14, 0x08,// p
	0x08, 0x14, 0x14, 0x18, 0x7C,// q
	0x7C, 0x08, 0x04, 0x04, 0x08,// r
	0x48, 0x54, 0x54, 0x54, 0x20,// s
	0x04, 0x3F, 0x44, 0x40, 0x20,// t
	0x3C, 0x40, 0x40, 0x20, 0x7C,// u
	0x1C, 0x20, 0x40, 0x20, 0x1C,// v
	0x3C, 0x40, 0x30, 0x40, 0x3C,// w
	0x44, 0x28, 0x10, 0x28, 0x44,// x
	0x0C, 0x50, 0x50, 0x50, 0x3C,// y
	0x44, 0x64, 0x54, 0x4C, 0x44,// z
	0x00, 0x08, 0x36, 0x41, 0x00,// {
	0x00, 0x00, 0x7F, 0x00, 0x00,// |
	0x00, 0x41, 0x36, 0x08, 0x00,// }
	0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
	0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
};

void lcd_lib_draw_string(uint8_t x, uint8_t y, const char* str)
{
    uint8_t* dst = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1);
    uint8_t* dst2 = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1) + (LCD_GFX_WIDTH+1);
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;
    while(*str)
    {
        const uint8_t* src = lcd_font_7x5 + (*str - ' ') * 5;
        
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (*str - ' ') * 5;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            dst2++;
        }
        str++;
    }
}

void lcd_lib_clear_string(uint8_t x, uint8_t y, const char* str)
{
    uint8_t* dst = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1);
    uint8_t* dst2 = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1) + (LCD_GFX_WIDTH+1);
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;
    while(*str)
    {
        const uint8_t* src = lcd_font_7x5 + (*str - ' ') * 5;
        
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (*str - ' ') * 5;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            dst2++;
        }
        str++;
    }
}

void lcd_lib_draw_string_center(uint8_t y, const char* str)
{
    lcd_lib_draw_string(64 - strlen(str) * 3, y, str);
}

void lcd_lib_clear_string_center(uint8_t y, const char* str)
{
    lcd_lib_clear_string(64 - strlen(str) * 3, y, str);
}

void lcd_lib_draw_stringP(uint8_t x, uint8_t y, const char* pstr)
{
    uint8_t* dst = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1);
    uint8_t* dst2 = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1) + (LCD_GFX_WIDTH+1);
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;
    
    for(char c = pgm_read_byte(pstr); c; c = pgm_read_byte(++pstr))
    {
        const uint8_t* src = lcd_font_7x5 + (c - ' ') * 5;
        
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (c - ' ') * 5;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            *dst2 = (*dst2) | pgm_read_byte(src++) >> yshift2; dst2++;
            dst2++;
        }
    }
}

void lcd_lib_clear_stringP(uint8_t x, uint8_t y, const char* pstr)
{
    uint8_t* dst = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1);
    uint8_t* dst2 = lcd_buffer + 1 + x + (y / 8) * (LCD_GFX_WIDTH+1) + (LCD_GFX_WIDTH+1);
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;

    for(char c = pgm_read_byte(pstr); c; c = pgm_read_byte(++pstr))
    {
        const uint8_t* src = lcd_font_7x5 + (c - ' ') * 5;
        
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (c - ' ') * 5;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            *dst2 = (*dst2) &~(pgm_read_byte(src++) >> yshift2); dst2++;
            dst2++;
        }
    }
}

void lcd_lib_draw_string_centerP(uint8_t y, const char* pstr)
{
    lcd_lib_draw_stringP(64 - strlen_P(pstr) * 3, y, pstr);
}

void lcd_lib_clear_string_centerP(uint8_t y, const char* pstr)
{
    lcd_lib_clear_stringP(64 - strlen_P(pstr) * 3, y, pstr);
}

void lcd_lib_draw_string_center_atP(uint8_t x, uint8_t y, const char* pstr)
{
    const char* split = strchr_P(pstr, '|');
    if (split)
    {
        char buf[10];
        strncpy_P(buf, pstr, split - pstr);
        buf[split - pstr] = '\0';
        lcd_lib_draw_string(x - strlen(buf) * 3, y - 5, buf);
        lcd_lib_draw_stringP(x - strlen_P(split+1) * 3, y + 5, split+1);
    }else{
        lcd_lib_draw_stringP(x - strlen_P(pstr) * 3, y, pstr);
    }
}

void lcd_lib_clear_string_center_atP(uint8_t x, uint8_t y, const char* pstr)
{
    const char* split = strchr_P(pstr, '|');
    if (split)
    {
        char buf[10];
        strncpy_P(buf, pstr, split - pstr);
        buf[split - pstr] = '\0';
        lcd_lib_clear_string(x - strlen(buf) * 3, y - 5, buf);
        lcd_lib_clear_stringP(x - strlen_P(split+1) * 3, y + 5, split+1);
    }else{
        lcd_lib_clear_stringP(x - strlen_P(pstr) * 3, y, pstr);
    }
}

void lcd_lib_draw_hline(uint8_t x0, uint8_t x1, uint8_t y)
{
    uint8_t* dst = lcd_buffer + 1 + x0 + (y / 8) * (LCD_GFX_WIDTH + 1);
    uint8_t mask = 0x01 << (y % 8);
    
    while(x0 <= x1)
    {
        *dst++ |= mask;
        x0 ++;
    }
}

void lcd_lib_draw_vline(uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + 1 + x + (y0 / 8) * (LCD_GFX_WIDTH + 1);
    uint8_t* dst1 = lcd_buffer + 1 + x + (y1 / 8) * (LCD_GFX_WIDTH + 1);
    if (dst0 == dst1)
    {
        *dst0 |= (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
    }else{
        *dst0 |= 0xFF << (y0 % 8);
        dst0 += (LCD_GFX_WIDTH + 1);
        while(dst0 != dst1)
        {
            *dst0 = 0xFF;
            dst0 += (LCD_GFX_WIDTH + 1);
        }
        *dst1 |= 0xFF >> (7 - (y1 % 8));
    }
}

void lcd_lib_draw_box(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    lcd_lib_draw_vline(x0, y0+1, y1-1);
    lcd_lib_draw_vline(x1, y0+1, y1-1);
    lcd_lib_draw_hline(x0+1, x1-1, y0);
    lcd_lib_draw_hline(x0+1, x1-1, y1);
}

void lcd_lib_draw_shade(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + 1 + x0 + (y0 / 8) * (LCD_GFX_WIDTH + 1);
    uint8_t* dst1 = lcd_buffer + 1 + x0 + (y1 / 8) * (LCD_GFX_WIDTH + 1);
    if (dst0 == dst1)
    {
        //uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        //*dstA0 |= (mask & 0xEE);
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ |= mask & ((x & 1) ? 0xAA : 0x55);
        dst0 += (LCD_GFX_WIDTH + 1);
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; x<=x1; x++)
                *dst++ |= (x & 1) ? 0xAA : 0x55;
            dst0 += (LCD_GFX_WIDTH + 1);
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ |= mask & ((x & 1) ? 0xAA : 0x55);
    }
}

void lcd_lib_clear()
{
    memset(lcd_buffer, 0, sizeof(lcd_buffer));
}

void lcd_lib_set()
{
    memset(lcd_buffer, 0xFF, sizeof(lcd_buffer));
}

void lcd_lib_clear(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + 1 + x0 + (y0 / 8) * (LCD_GFX_WIDTH + 1);
    uint8_t* dst1 = lcd_buffer + 1 + x0 + (y1 / 8) * (LCD_GFX_WIDTH + 1);
    if (dst0 == dst1)
    {
        uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        for(uint8_t x=x0; x<=x1; x++)
            *dst0++ &=~mask;
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ &=~mask;
        dst0 += (LCD_GFX_WIDTH + 1);
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; x<=x1; x++)
                *dst++ = 0x00;
            dst0 += (LCD_GFX_WIDTH + 1);
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ &=~mask;
    }
}

void lcd_lib_invert(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + 1 + x0 + (y0 / 8) * (LCD_GFX_WIDTH + 1);
    uint8_t* dst1 = lcd_buffer + 1 + x0 + (y1 / 8) * (LCD_GFX_WIDTH + 1);
    if (dst0 == dst1)
    {
        uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        for(uint8_t x=x0; x<=x1; x++)
            *dst0++ ^= mask;
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ ^= mask;
        dst0 += (LCD_GFX_WIDTH + 1);
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; x<=x1; x++)
                *dst++ ^= 0xFF;
            dst0 += (LCD_GFX_WIDTH + 1);
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ ^= mask;
    }
}

void lcd_lib_set(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + 1 + x0 + (y0 / 8) * (LCD_GFX_WIDTH + 1);
    uint8_t* dst1 = lcd_buffer + 1 + x0 + (y1 / 8) * (LCD_GFX_WIDTH + 1);
    if (dst0 == dst1)
    {
        uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        for(uint8_t x=x0; x<=x1; x++)
            *dst0++ |= mask;
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ |= mask;
        dst0 += (LCD_GFX_WIDTH + 1);
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; x<=x1; x++)
                *dst++ = 0xFF;
            dst0 += (LCD_GFX_WIDTH + 1);
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; x<=x1; x++)
            *dst++ |= mask;
    }
}

void lcd_lib_draw_gfx(uint8_t x, uint8_t y, const uint8_t* gfx)
{
    uint8_t w = pgm_read_byte(gfx++);
    uint8_t h = (pgm_read_byte(gfx++) + 7) / 8;
    uint8_t shift = y % 8;
    uint8_t shift2 = 8 - shift;
    y /= 8;
    
    for(; h; h--)
    {
        if (y >= LCD_GFX_HEIGHT / 8) break;
        
        uint8_t* dst0 = lcd_buffer + 1 + x + y * (LCD_GFX_WIDTH + 1);
        uint8_t* dst1 = lcd_buffer + 1 + x + y * (LCD_GFX_WIDTH + 1) + (LCD_GFX_WIDTH + 1);
        for(uint8_t _w = w; _w; _w--)
        {
            uint8_t c = pgm_read_byte(gfx++);
            *dst0++ |= c << shift;
            if (shift && y < 7)
                *dst1++ |= c >> shift2;
        }
        y++;
    }
}

void lcd_lib_clear_gfx(uint8_t x, uint8_t y, const uint8_t* gfx)
{
    uint8_t w = pgm_read_byte(gfx++);
    uint8_t h = (pgm_read_byte(gfx++) + 7) / 8;
    uint8_t shift = y % 8;
    uint8_t shift2 = 8 - shift;
    y /= 8;
    
    for(; h; h--)
    {
        if (y >= LCD_GFX_HEIGHT / 8) break;
        
        uint8_t* dst0 = lcd_buffer + 1 + x + y * (LCD_GFX_WIDTH + 1);
        uint8_t* dst1 = lcd_buffer + 1 + x + y * (LCD_GFX_WIDTH + 1) + (LCD_GFX_WIDTH + 1);
        for(uint8_t _w = w; _w; _w--)
        {
            uint8_t c = pgm_read_byte(gfx++);
            *dst0++ &=~(c << shift);
            if (shift && y < 7)
                *dst1++ &=~(c >> shift2);
        }
        y++;
    }
}

void lcd_lib_beep()
{
#define _BEEP(c, n) for(int8_t _i=0;_i<c;_i++) { WRITE(BEEPER, HIGH); _delay_us(n); WRITE(BEEPER, LOW); _delay_us(n); }
    _BEEP(20, 366);
    _BEEP(10, 150);
#undef _BEEP
}

int8_t lcd_lib_encoder_pos_interrupt = 0;
int16_t lcd_lib_encoder_pos = 0;
bool lcd_lib_button_pressed = false;
bool lcd_lib_button_down;

#define ENCODER_ROTARY_BIT_0 _BV(0)
#define ENCODER_ROTARY_BIT_1 _BV(1)
/* Warning: This function is called from interrupt context */
void lcd_lib_buttons_update_interrupt()
{
    static uint8_t lastEncBits = 0;
    
    uint8_t encBits = 0;
    if(!READ(BTN_EN1)) encBits |= ENCODER_ROTARY_BIT_0;
    if(!READ(BTN_EN2)) encBits |= ENCODER_ROTARY_BIT_1;
    
    if(encBits != lastEncBits)
    {
        switch(encBits)
        {
        case encrot0:
            if(lastEncBits==encrot3)
                lcd_lib_encoder_pos_interrupt++;
            else if(lastEncBits==encrot1)
                lcd_lib_encoder_pos_interrupt--;
            break;
        case encrot1:
            if(lastEncBits==encrot0)
                lcd_lib_encoder_pos_interrupt++;
            else if(lastEncBits==encrot2)
                lcd_lib_encoder_pos_interrupt--;
            break;
        case encrot2:
            if(lastEncBits==encrot1)
                lcd_lib_encoder_pos_interrupt++;
            else if(lastEncBits==encrot3)
                lcd_lib_encoder_pos_interrupt--;
            break;
        case encrot3:
            if(lastEncBits==encrot2)
                lcd_lib_encoder_pos_interrupt++;
            else if(lastEncBits==encrot0)
                lcd_lib_encoder_pos_interrupt--;
            break;
        }
        lastEncBits = encBits;
    }
}

void lcd_lib_buttons_update()
{
    lcd_lib_encoder_pos += lcd_lib_encoder_pos_interrupt;
    lcd_lib_encoder_pos_interrupt = 0;

    uint8_t buttonState = !READ(BTN_ENC);
    lcd_lib_button_pressed = (buttonState && !lcd_lib_button_down);
    lcd_lib_button_down = buttonState;
}

char* int_to_string(int i, char* temp_buffer, const char* p_postfix)
{
    char* c = temp_buffer;
    if (i < 0)
    {
        *c++ = '-'; 
        i = -i;
    }
    if (i >= 10000)
        *c++ = ((i/10000)%10)+'0';
    if (i >= 1000)
        *c++ = ((i/1000)%10)+'0';
    if (i >= 100)
        *c++ = ((i/100)%10)+'0';
    if (i >= 10)
        *c++ = ((i/10)%10)+'0';
    *c++ = ((i)%10)+'0';
    *c = '\0';
    if (p_postfix)
    {
        strcpy_P(c, p_postfix);
        c += strlen_P(p_postfix);
    }
    return c;
}

char* int_to_time_string(unsigned long i, char* temp_buffer)
{
    char* c = temp_buffer;
    uint8_t hours = i / 60 / 60;
    uint8_t mins = (i / 60) % 60;
    uint8_t secs = i % 60;
    
    if (hours > 0)
    {
        if (hours > 99)
            *c++ = '0' + hours / 100;
        if (hours > 9)
            *c++ = '0' + (hours / 10) % 10;
        *c++ = '0' + hours % 10;
        if (hours > 1)
        {
            strcpy_P(c, PSTR(" hours"));
            return c + 6;
        }
        strcpy_P(c, PSTR(" hour"));
        return c + 5;
    }
    if (mins > 0)
    {
        if (mins > 9)
            *c++ = '0' + (mins / 10) % 10;
        *c++ = '0' + mins % 10;
        strcpy_P(c, PSTR(" min"));
        return c + 4;
    }
    if (secs > 9)
        *c++ = '0' + secs / 10;
    *c++ = '0' + secs % 10;
    strcpy_P(c, PSTR(" sec"));
    return c + 4;
    /*
    if (hours > 99)
        *c++ = '0' + hours / 100;
    *c++ = '0' + (hours / 10) % 10;
    *c++ = '0' + hours % 10;
    *c++ = ':';
    *c++ = '0' + mins / 10;
    *c++ = '0' + mins % 10;
    *c++ = ':';
    *c++ = '0' + secs / 10;
    *c++ = '0' + secs % 10;
    *c = '\0';
    return c;
    */
}

char* float_to_string(float f, char* temp_buffer, const char* p_postfix)
{
    int32_t i = f * 100.0 + 0.5;
    char* c = temp_buffer;
    if (i < 0)
    {
        *c++ = '-'; 
        i = -i;
    }
    if (i >= 10000)
        *c++ = ((i/10000)%10)+'0';
    if (i >= 1000)
        *c++ = ((i/1000)%10)+'0';
    *c++ = ((i/100)%10)+'0';
    *c++ = '.';
    *c++ = ((i/10)%10)+'0';
    *c++ = ((i)%10)+'0';
    *c = '\0';
    if (p_postfix)
    {
        strcpy_P(c, p_postfix);
        c += strlen_P(p_postfix);
    }
    return c;
}

#endif//ENABLE_ULTILCD2
