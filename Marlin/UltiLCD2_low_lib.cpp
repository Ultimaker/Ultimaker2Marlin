#include "Configuration.h"

#ifdef ENABLE_ULTILCD2
#include "pins.h"
#include "preferences.h"
#include "UltiLCD2_low_lib.h"
#include "tinkergnome.h"

/**
 * Implementation of the LCD display routines for a SSD1309 OLED graphical display connected with i2c.
 **/
#define LCD_RESET_PIN 5
#define I2C_SDA_PIN   20
#define I2C_SCL_PIN   21

#define I2C_FREQ 400000

//The TWI interrupt routine conflicts with an interrupt already defined by Arduino, if you are using the Arduino IDE.
// Not running the screen update from interrupts causes a 25ms delay each screen refresh. Which will cause issues during printing.
// I recommend against using the Arduino IDE and setup a proper development environment.
#define USE_TWI_INTERRUPT 1

#define I2C_WRITE   0x00
#define I2C_READ    0x01

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

unsigned long last_user_interaction=0;

/** Backbuffer for LCD */
#define LCD_BUFFER_SIZE  (LCD_GFX_WIDTH * LCD_GFX_HEIGHT / 8)
uint8_t lcd_buffer[LCD_BUFFER_SIZE];
uint8_t led_r, led_g, led_b;
uint8_t led_glow = 0;
uint8_t led_glow_dir;

/**
 * i2c communication low level functions.
 */
static inline void i2c_start()
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
}

static inline void i2c_restart()
{
    while (!(TWCR & (1<<TWINT))) {}
    i2c_start();
}

static inline void i2c_send_raw(uint8_t data)
{
    while (!(TWCR & (1<<TWINT))) {}
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
}

static inline void i2c_end()
{
    while (!(TWCR & (1<<TWINT))) {}
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

static void i2c_led_write(uint8_t addr, uint8_t data)
{
    i2c_start();
    i2c_send_raw(I2C_LED_ADDRESS << 1 | I2C_WRITE);
    i2c_send_raw(addr);
    i2c_send_raw(data);
    i2c_end();
}

// set LCD contrast
void lcd_lib_contrast(uint8_t data)
{
    i2c_start();
    i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
    i2c_send_raw(I2C_LCD_SEND_COMMAND);
    i2c_send_raw(LCD_COMMAND_CONTRAST);
    i2c_send_raw(data);
    i2c_end();
}

void lcd_lib_init()
{
    SET_OUTPUT(LCD_RESET_PIN);

    SET_OUTPUT(I2C_SDA_PIN);
    SET_OUTPUT(I2C_SCL_PIN);

    //Set unused pins in the 10 pin connector to GND to improve shielding of the cable.
    SET_OUTPUT(LCD_PINS_D4); WRITE(LCD_PINS_D4, 0); //RXD3/PJ1
    SET_OUTPUT(LCD_PINS_ENABLE); WRITE(LCD_PINS_ENABLE, 0); //TXD3/PJ0
    SET_OUTPUT(LCD_PINS_D7); WRITE(LCD_PINS_D7, 0); //PH3

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

    WRITE(I2C_SDA_PIN, 1);
    WRITE(I2C_SCL_PIN, 1);

    WRITE(LCD_RESET_PIN, 0);
    _delay_ms(1);
    WRITE(LCD_RESET_PIN, 1);
    _delay_ms(1);

    //ClockFreq = (F_CPU) / (16 + 2*TWBR * 4^TWPS)
    //TWBR = ((F_CPU / ClockFreq) - 16)/2*4^TWPS
    TWBR = ((F_CPU / I2C_FREQ) - 16)/2*1;
    TWSR = 0x00;

    i2c_led_write(0, 0x80);//MODE1
    i2c_led_write(1, 0x1C);//MODE2
    i2c_led_write(2, led_r);//PWM0
    i2c_led_write(3, led_g);//PWM1
    i2c_led_write(4, led_b);//PWM2
    i2c_led_write(5, 0x00);//PWM3
    i2c_led_write(6, 0xFF);//GRPPWM
    i2c_led_write(7, 0x00);//GRPFREQ
    i2c_led_write(8, 0xAA);//LEDOUT

    i2c_start();
    i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
    i2c_send_raw(I2C_LCD_SEND_COMMAND);

    i2c_send_raw(LCD_COMMAND_LOCK_COMMANDS);
    i2c_send_raw(0x12);

    i2c_send_raw(LCD_COMMAND_DISPLAY_OFF);

    i2c_send_raw(0xD5);//Display clock divider/freq
    i2c_send_raw(0xA0);

    i2c_send_raw(0xA8);//Multiplex ratio
    i2c_send_raw(0x3F);

    i2c_send_raw(0xD3);//Display offset
    i2c_send_raw(0x00);

    i2c_send_raw(0x40);//Set start line

    i2c_send_raw(0xA1);//Segment remap

    i2c_send_raw(0xC8);//COM scan output direction
    i2c_send_raw(0xDA);//COM pins hardware configuration
    i2c_send_raw(0x12);

    i2c_send_raw(LCD_COMMAND_CONTRAST);
    i2c_send_raw(lcd_contrast);

    i2c_send_raw(0xD9);//Pre charge period
    i2c_send_raw(0x82);

    i2c_send_raw(0xDB);//VCOMH Deslect level
    i2c_send_raw(0x34);

    i2c_send_raw(LCD_COMMAND_SET_ADDRESSING_MODE);

    i2c_send_raw(LCD_COMMAND_FULL_DISPLAY_ON_DISABLE);

    i2c_send_raw(LCD_COMMAND_DISPLAY_ON);
    i2c_end();

    lcd_lib_buttons_update_interrupt();
    lcd_lib_buttons_update();
    lcd_lib_encoder_pos = 0;
    lcd_lib_update_screen();
}

#if USE_TWI_INTERRUPT
uint16_t lcd_update_pos = 0;
ISR(TWI_vect)
{
    if (lcd_update_pos == LCD_GFX_WIDTH*LCD_GFX_HEIGHT/8u)
    {
        i2c_end();
    }
    else
    {
        i2c_send_raw(lcd_buffer[lcd_update_pos]);
        TWCR |= _BV(TWIE);
        lcd_update_pos++;
    }
}
#endif

void led_update()
{
    // force update of the encoder led ring
    sleep_state |= SLEEP_UPDATE_LED;
}

void lcd_lib_update_screen()
{
    if (lcd_timeout > 0)
    {
        const unsigned long timeout=last_user_interaction + (lcd_timeout*MILLISECONDS_PER_MINUTE);
        if (timeout < millis())
        {
            if (!(sleep_state & SLEEP_LCD_DIMMED))
            {
                sleep_state ^= SLEEP_LCD_DIMMED;
                if (lcd_sleep_contrast > 0)
                {
                    // reduce contrast
                    lcd_lib_contrast(min(lcd_sleep_contrast, lcd_contrast));
                } else
                {
                    // switch LCD off
                    i2c_start();
                    i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
                    i2c_send_raw(I2C_LCD_SEND_COMMAND);
                    i2c_send_raw(LCD_COMMAND_DISPLAY_OFF);
                    i2c_end();
                }
                if (!led_sleep_glow)
                {
                    // screen saver is active - switch off the encoder led
                    i2c_led_write(2, 0);//PWM0
                    i2c_led_write(3, 0);//PWM1
                    i2c_led_write(4, 0);//PWM2
                }
            }
            if (led_sleep_glow && !(sleep_state & SLEEP_LED_OFF))
            {
                uint8_t glow = (int(led_glow) * led_sleep_glow) >> 8;
                lcd_lib_led_color(glow, constrain(glow << 1, 0, 160), glow);
                led_update();
            }
        }
        else if (sleep_state & SLEEP_LCD_DIMMED)
        {
            sleep_state ^= SLEEP_LCD_DIMMED;
            // reactivate led ring
            LED_NORMAL

            if (lcd_sleep_contrast > 0)
            {
                // increase contrast back to normal
                lcd_lib_contrast(lcd_contrast);
            } else
            {
                // switch LCD on
                i2c_start();
                i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
                i2c_send_raw(I2C_LCD_SEND_COMMAND);
                i2c_send_raw(LCD_COMMAND_DISPLAY_ON);
                i2c_end();
            }
        }
    }

    if (sleep_state & SLEEP_UPDATE_LED)
    {
        sleep_state &= ~SLEEP_UPDATE_LED;
        // set values for encoder led ring
        i2c_led_write(2, led_r);//PWM0
        i2c_led_write(3, led_g);//PWM1
        i2c_led_write(4, led_b);//PWM2
    }

    if (!(sleep_state & SLEEP_LCD_DIMMED) || lcd_sleep_contrast)
    {
        // update screen content
        i2c_start();
        i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
        //Set the drawing position to 0,0
        i2c_send_raw(I2C_LCD_SEND_COMMAND);
        i2c_send_raw(0x00 | (0 & 0x0F));
        i2c_send_raw(0x10 | (0 >> 4));
        i2c_send_raw(0xB0 | 0);

        i2c_restart();
        i2c_send_raw(I2C_LCD_ADDRESS << 1 | I2C_WRITE);
        i2c_send_raw(I2C_LCD_SEND_DATA);
    #if USE_TWI_INTERRUPT
        lcd_update_pos = 0;
        TWCR |= _BV(TWIE);
    #else
        for(uint16_t n=0;n<LCD_GFX_WIDTH*LCD_GFX_HEIGHT/8;n++)
        {
            i2c_send_raw(lcd_buffer[n]);
        }
        i2c_end();
    #endif
    }
}

bool lcd_lib_update_ready()
{
#if USE_TWI_INTERRUPT
    return !(TWCR & _BV(TWIE));
#else
    return true;
#endif
}

void lcd_lib_led_color(uint8_t r, uint8_t g, uint8_t b)
{
    led_r = r;
    led_g = g;
    led_b = b;
}

uint8_t lcd_lib_led_brightness()
{
    uint16_t sum = (led_r+led_g+led_b);
    return sum/3;
}

// norpchen
// the baseline ASCII->font table offset.  Was 32 (space) but shifted down as I added custom characters to the font
// had to add them to the start of the table, because we're using char, which are signed and top out at 128, which is already the max table value.
#define FONT_BASE_CHAR 0x1D

static const uint8_t lcd_font_7x5[] PROGMEM = {
	0x11, 0x15, 0x0A, 0x00, 0x00,       // ^3 " CUBED_SYMBOL "
	0x19, 0x15, 0x12, 0x00, 0x00,       // ^2 " SQUARED_SYMBOL "
	0x06, 0x09, 0x09, 0x06, 0x00,    	// deg C  " DEGREE_C_SYMBOL "

//	0x30, 0x0C, 0x43, 0xA8, 0x90,		//s " PER_SECOND_SYMBOL "   -- fugly, dont use

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
    uint8_t* dst = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH;
    uint8_t* dst2 = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH + LCD_GFX_WIDTH;
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;
    while ((*str) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE-5) && ((dst2-lcd_buffer) < LCD_BUFFER_SIZE-5))
    {
        const uint8_t* src = lcd_font_7x5 + (*str - FONT_BASE_CHAR) * 5;

        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (*str - FONT_BASE_CHAR) * 5;
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
    uint8_t* dst = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH;
    uint8_t* dst2 = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH + LCD_GFX_WIDTH;
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;
    while ((*str) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE-5) && ((dst2-lcd_buffer) < LCD_BUFFER_SIZE-5))
    {
        const uint8_t* src = lcd_font_7x5 + (*str - FONT_BASE_CHAR) * 5;

        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (*str - FONT_BASE_CHAR) * 5;
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
    lcd_lib_draw_string(LCD_GFX_WIDTH/2 - min(strlen(str), LINE_ENTRY_TEXT_LENGHT) * (LCD_CHAR_SPACING/2), y, str);
}

void lcd_lib_clear_string_center(uint8_t y, const char* str)
{
    lcd_lib_clear_string(LCD_GFX_WIDTH/2 - min(strlen(str), LINE_ENTRY_TEXT_LENGHT) * (LCD_CHAR_SPACING/2), y, str);
}

void lcd_lib_draw_stringP(uint8_t x, uint8_t y, const char* pstr)
{
    uint8_t* dst = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH;
    uint8_t* dst2 = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH + LCD_GFX_WIDTH;
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;

    for(char c = pgm_read_byte(pstr); c && ((dst-lcd_buffer) < LCD_BUFFER_SIZE-5) && ((dst2-lcd_buffer) < LCD_BUFFER_SIZE-5); c = pgm_read_byte(++pstr))
    {
        const uint8_t* src = lcd_font_7x5 + (c - FONT_BASE_CHAR) * 5;

        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        *dst = (*dst) | pgm_read_byte(src++) << yshift; dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (c - FONT_BASE_CHAR) * 5;
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
    uint8_t* dst = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH;
    uint8_t* dst2 = lcd_buffer + x + (y / 8) * LCD_GFX_WIDTH + LCD_GFX_WIDTH;
    uint8_t yshift = y % 8;
    uint8_t yshift2 = 8 - yshift;

    for(char c = pgm_read_byte(pstr); c && ((dst-lcd_buffer) < LCD_BUFFER_SIZE-5) && ((dst2-lcd_buffer) < LCD_BUFFER_SIZE-5); c = pgm_read_byte(++pstr))
    {
        const uint8_t* src = lcd_font_7x5 + (c - FONT_BASE_CHAR) * 5;

        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        *dst = (*dst) &~(pgm_read_byte(src++) << yshift); dst++;
        dst++;

        if (yshift != 0)
        {
            src = lcd_font_7x5 + (c - FONT_BASE_CHAR) * 5;
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
    lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 - strlen_P(pstr) * (LCD_CHAR_SPACING/2), y, pstr);
}

void lcd_lib_clear_string_centerP(uint8_t y, const char* pstr)
{
    lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 - strlen_P(pstr) * (LCD_CHAR_SPACING/2), y, pstr);
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
    uint8_t* dst = lcd_buffer + x0 + (y / 8) * LCD_GFX_WIDTH;
    uint8_t mask = 0x01 << (y % 8);

    while ((x0 <= x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE))
    {
        *dst++ |= mask;
        x0 ++;
    }
}

void lcd_lib_draw_vline(uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + x + (y0 / 8) * LCD_GFX_WIDTH;
    uint8_t* dst1 = lcd_buffer + x + (y1 / 8) * LCD_GFX_WIDTH;

    if (((dst0-lcd_buffer) < LCD_BUFFER_SIZE) && ((dst1-lcd_buffer) < LCD_BUFFER_SIZE))
    {
        if (dst0 == dst1)
        {
            *dst0 |= (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        }else{
            *dst0 |= 0xFF << (y0 % 8);
            dst0 += LCD_GFX_WIDTH;
            while(dst0 != dst1)
            {
                *dst0 = 0xFF;
                dst0 += LCD_GFX_WIDTH;
            }
            *dst1 |= 0xFF >> (7 - (y1 % 8));
        }
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
    uint8_t* dst0 = lcd_buffer + x0 + (y0 / 8) * LCD_GFX_WIDTH;
    uint8_t* dst1 = lcd_buffer + x0 + (y1 / 8) * LCD_GFX_WIDTH;
    if (dst0 == dst1)
    {
        //uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        //*dstA0 |= (mask & 0xEE);
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst++ |= mask & ((x & 1) ? 0xAA : 0x55);
        dst0 += LCD_GFX_WIDTH;
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
                *dst++ |= (x & 1) ? 0xAA : 0x55;
            dst0 += LCD_GFX_WIDTH;
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
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
    uint8_t* dst0 = lcd_buffer + x0 + (y0 / 8) * LCD_GFX_WIDTH;
    uint8_t* dst1 = lcd_buffer + x0 + (y1 / 8) * LCD_GFX_WIDTH;
    if (dst0 == dst1)
    {
        uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        for(uint8_t x=x0; (x<=x1) && ((dst0-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst0++ &=~mask;
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst++ &=~mask;
        dst0 += LCD_GFX_WIDTH;
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
                *dst++ = 0x00;
            dst0 += LCD_GFX_WIDTH;
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst++ &=~mask;
    }
}

void lcd_lib_invert(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + x0 + (y0 / 8) * LCD_GFX_WIDTH;
    uint8_t* dst1 = lcd_buffer + x0 + (y1 / 8) * LCD_GFX_WIDTH;
    if (dst0 == dst1)
    {
        uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        for(uint8_t x=x0; (x<=x1) && ((dst0-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst0++ ^= mask;
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst++ ^= mask;
        dst0 += LCD_GFX_WIDTH;
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
                *dst++ ^= 0xFF;
            dst0 += LCD_GFX_WIDTH;
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst++ ^= mask;
    }
}

void lcd_lib_set(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    uint8_t* dst0 = lcd_buffer + x0 + (y0 / 8) * LCD_GFX_WIDTH;
    uint8_t* dst1 = lcd_buffer + x0 + (y1 / 8) * LCD_GFX_WIDTH;
    if (dst0 == dst1)
    {
        uint8_t mask = (0xFF << (y0 % 8)) & (0xFF >> (7 - (y1 % 8)));
        for(uint8_t x=x0; (x<=x1) && ((dst0-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst0++ |= mask;
    }else{
        uint8_t mask = 0xFF << (y0 % 8);
        uint8_t* dst = dst0;
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
            *dst++ |= mask;
        dst0 += LCD_GFX_WIDTH;
        while(dst0 != dst1)
        {
            dst = dst0;
            for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
                *dst++ = 0xFF;
            dst0 += LCD_GFX_WIDTH;
        }
        dst = dst1;
        mask = 0xFF >> (7 - (y1 % 8));
        for(uint8_t x=x0; (x<=x1) && ((dst-lcd_buffer) < LCD_BUFFER_SIZE); x++)
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

        uint8_t* dst0 = lcd_buffer + x + y * LCD_GFX_WIDTH;
        uint8_t* dst1 = lcd_buffer + x + y * LCD_GFX_WIDTH + LCD_GFX_WIDTH;
        for(uint8_t _w = w; _w && ((dst0-lcd_buffer) < LCD_BUFFER_SIZE) && ((dst1-lcd_buffer) < LCD_BUFFER_SIZE); _w--)
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

        uint8_t* dst0 = lcd_buffer + x + y * LCD_GFX_WIDTH;
        uint8_t* dst1 = lcd_buffer + x + y * LCD_GFX_WIDTH + LCD_GFX_WIDTH;
        for(uint8_t _w = w; _w && ((dst0-lcd_buffer) < LCD_BUFFER_SIZE) && ((dst1-lcd_buffer) < LCD_BUFFER_SIZE); _w--)
        {
            uint8_t c = pgm_read_byte(gfx++);
            *dst0++ &=~(c << shift);
            if (shift && y < 7)
                *dst1++ &=~(c >> shift2);
        }
        y++;
    }
}

#define _BEEP(c, n) for(uint8_t _i=0;_i<c;_i++) { WRITE(BEEPER, HIGH); _delay_us(n); WRITE(BEEPER, LOW); _delay_us(n); }
void lcd_lib_keyclick()
{
    if (ui_mode & UI_BEEP_OFF)
    {
        return;
    }
    else if (ui_mode & UI_BEEP_SHORT)
    {
        _BEEP(15, 60);
        _BEEP(10, 50);
    }
    else
    {
        lcd_lib_beep();
    }
}

void lcd_lib_beep()
{
    _BEEP(20, 366);
    _BEEP(10, 150);
}

//-----------------------------------------------------------------------------------------------------------------
// very short tick for UI feedback -- 1 millisecond  long
void lcd_lib_tick()
{
    _BEEP(10, 50);
//	for (int a =0; a<10; ++a)
//    {
//        WRITE(BEEPER, HIGH);
//        _delay_us (50);
//        WRITE(BEEPER, LOW);
//        _delay_us(50);
//    }
}
#undef _BEEP


int8_t lcd_lib_encoder_pos_interrupt = 0;
int16_t lcd_lib_encoder_pos = 0;
bool lcd_lib_button_pressed = false;
bool lcd_lib_button_down;

#define ENCODER_ROTARY_BIT_0 _BV(0)
#define ENCODER_ROTARY_BIT_1 _BV(1)
#define INC_ENCODER_POS(n) do { if (n<+127) { ++n; } } while(0)
#define DEC_ENCODER_POS(n) do { if (n>-127) { --n; } } while(0)
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
                INC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            else if(lastEncBits==encrot1)
                DEC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            break;
        case encrot1:
            if(lastEncBits==encrot0)
                INC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            else if(lastEncBits==encrot2)
                DEC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            break;
        case encrot2:
            if(lastEncBits==encrot1)
                INC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            else if(lastEncBits==encrot3)
                DEC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            break;
        case encrot3:
            if(lastEncBits==encrot2)
                INC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            else if(lastEncBits==encrot0)
                DEC_ENCODER_POS(lcd_lib_encoder_pos_interrupt);
            break;
        }
        lastEncBits = encBits;
    }
}

void lcd_lib_buttons_update()
{
    manage_encoder_position(lcd_lib_encoder_pos_interrupt);

    uint8_t buttonState = !READ(BTN_ENC);
    lcd_lib_button_pressed = (buttonState && !lcd_lib_button_down);
    lcd_lib_button_down = buttonState;

	if (lcd_lib_button_down || lcd_lib_encoder_pos_interrupt!=0 ) last_user_interaction=millis();

    lcd_lib_encoder_pos_interrupt = 0;
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
    uint16_t hours = constrain(i / 60 / 60, 0, 999);
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

char* float_to_string2(float f, char* temp_buffer, const char* p_postfix, const bool bForceSign)
{
    int32_t i = f * 100.0 + 0.5;
    char* c = temp_buffer;
    if (bForceSign)
    {
        if (i > 0)
        {
            *c++ = '+';
        }
        else if (i == 0)
        {
            *c++ = ' ';
        }
    }
    if (i < 0)
    {
        *c++ = '-';
        i = -i;
    }
    if (i >= 100000)
        *c++ = ((i/100000)%10)+'0';
    if (i >= 10000)
        *c++ = ((i/10000)%10)+'0';
    if (i >= 1000)
        *c++ = ((i/1000)%10)+'0';
    *c++ = ((i/100)%10)+'0';
    *c++ = '.';
    *c++ = ((i/10)%10)+'0';
    *c++ = ((i)%10)+'0';
    if (p_postfix)
    {
        strcpy_P(c, p_postfix);
        c += strlen_P(p_postfix);
    }
    *c = '\0';
    return c;
}

#endif//ENABLE_ULTILCD2
