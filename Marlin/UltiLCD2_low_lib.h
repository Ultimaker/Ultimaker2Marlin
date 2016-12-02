#ifndef ULTI_LCD2_LOW_LIB_H
#define ULTI_LCD2_LOW_LIB_H

#include <stdint.h>
#include <stddef.h>

#define LCD_GFX_WIDTH 128
#define LCD_GFX_HEIGHT 64
#define LCD_FONT_WIDTH  5
#define LCD_FONT_WITH_SPACE_WIDTH   (LCD_FONT_WIDTH + 1)

#define LCD_MAX_TEXT_LINE_LENGTH    (LCD_GFX_WIDTH / (LCD_FONT_WITH_SPACE_WIDTH))   // Rounded down this is 21 characters per line

void lcd_lib_init();
void lcd_lib_update_screen();   /* Start sending out the display buffer to the screen. Wait till lcd_lib_update_ready before issuing any draw functions */
bool lcd_lib_update_ready();

void lcd_lib_draw_string(uint8_t x, uint8_t y, const char* str);
void lcd_lib_clear_string(uint8_t x, uint8_t y, const char* str);
void lcd_lib_draw_string_center(uint8_t y, const char* str);
void lcd_lib_clear_string_center(uint8_t y, const char* str);
void lcd_lib_draw_stringP(uint8_t x, uint8_t y, const char* pstr);
void lcd_lib_clear_stringP(uint8_t x, uint8_t y, const char* pstr);
void lcd_lib_draw_string_centerP(uint8_t y, const char* pstr);
void lcd_lib_clear_string_centerP(uint8_t y, const char* pstr);
void lcd_lib_draw_string_center_atP(uint8_t x, uint8_t y, const char* pstr);
void lcd_lib_clear_string_center_atP(uint8_t x, uint8_t y, const char* pstr);
void lcd_lib_draw_vline(uint8_t x, uint8_t y0, uint8_t y1);
void lcd_lib_draw_hline(uint8_t x0, uint8_t x1, uint8_t y);
void lcd_lib_draw_box(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_lib_draw_shade(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_lib_clear();
void lcd_lib_clear(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_lib_invert(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_lib_set();
void lcd_lib_set(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void lcd_lib_draw_gfx(uint8_t x, uint8_t y, const uint8_t* gfx);
void lcd_lib_clear_gfx(uint8_t x, uint8_t y, const uint8_t* gfx);

void lcd_lib_beep();
void lcd_lib_buttons_update();
void lcd_lib_buttons_update_interrupt();
void lcd_lib_led_color(uint8_t r, uint8_t g, uint8_t b);

extern int16_t lcd_lib_encoder_pos;
extern bool lcd_lib_button_pressed;
extern bool lcd_lib_button_down;

char* int_to_string(int i, char* temp_buffer, const char* p_postfix = NULL);
char* int_to_time_string(unsigned long i, char* temp_buffer);
char* float_to_string(float f, char* temp_buffer, const char* p_postfix = NULL);

#endif//ULTI_LCD2_LOW_LIB_H
