#ifndef ULTI_LCD2_LOW_LIB_H
#define ULTI_LCD2_LOW_LIB_H

#include <stdint.h>
#include <stddef.h>

void lcd_lib_init();
void lcd_lib_update_screen();   /* Start sending out the display buffer to the screen. Wait till lcd_lib_update_ready before issuing any draw functions */
bool lcd_lib_update_ready();
void led_update();

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
void lcd_lib_tick();
void lcd_lib_keyclick();
void lcd_lib_buttons_update();
void lcd_lib_buttons_update_interrupt();
void lcd_lib_led_color(uint8_t r, uint8_t g, uint8_t b);
uint8_t lcd_lib_led_brightness();
void lcd_lib_contrast(uint8_t data);

extern int16_t lcd_lib_encoder_pos;
extern bool lcd_lib_button_pressed;
extern bool lcd_lib_button_down;
extern unsigned long last_user_interaction;
extern uint8_t led_glow;
extern uint8_t led_glow_dir;

char* int_to_string(int i, char* temp_buffer, const char* p_postfix = NULL);
char* int_to_time_string(unsigned long i, char* temp_buffer);
char* float_to_string2(float f, char* temp_buffer, const char* p_postfix = NULL, const bool bForceSign = false);

// display constants
#define LCD_GFX_WIDTH 128
#define LCD_GFX_HEIGHT 64

// text position constants
#define LCD_LINE_HEIGHT 9
#define LCD_CHAR_MARGIN_LEFT 4
#define LCD_CHAR_MARGIN_RIGHT 4
#define LCD_CHAR_SPACING 6
#define LCD_CHAR_HEIGHT 7

#define LINE_ENTRY_STEP      2
#define LINE_ENTRY_WAIT_END 24
#define LINE_ENTRY_GFX_LENGTH  (LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-LCD_CHAR_MARGIN_LEFT)
#define LINE_ENTRY_TEXT_LENGTH (LINE_ENTRY_GFX_LENGTH / LCD_CHAR_SPACING)
#define LINE_ENTRY_TEXT_OFFSET ((lineEntryPos%LCD_CHAR_SPACING == 0) ? 0 : -1)
#define LINE_ENTRY_TEXT_BEGIN  ((lineEntryPos + LCD_CHAR_SPACING-1) / LCD_CHAR_SPACING)
#define LINE_ENTRY_GFX_BEGIN   (LCD_CHAR_SPACING-1 - (lineEntryPos + LCD_CHAR_SPACING-1) % LCD_CHAR_SPACING)
#define LINE_ENTRY_MAX_STEP(text_length) ((text_length) * LCD_CHAR_SPACING)

FORCE_INLINE void lcd_lib_draw_string_left(uint8_t y, const char* str) { lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT, y, str); }
FORCE_INLINE void lcd_lib_draw_string_leftP(uint8_t y, const char* pstr) { lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, y, pstr); }
FORCE_INLINE void lcd_lib_draw_string_right(uint8_t x, uint8_t y, const char* str) { lcd_lib_draw_string(x - (strlen(str) * LCD_CHAR_SPACING), y, str); }
FORCE_INLINE void lcd_lib_draw_string_right(uint8_t y, const char* str) { lcd_lib_draw_string_right(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT, y, str); }
FORCE_INLINE void lcd_lib_draw_string_rightP(uint8_t y, const char* pstr) { lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (strlen_P(pstr) * LCD_CHAR_SPACING), y, pstr); }
FORCE_INLINE void lcd_lib_draw_string_rightP(uint8_t x, uint8_t y, const char* pstr) { lcd_lib_draw_stringP(x - (strlen_P(pstr) * LCD_CHAR_SPACING), y, pstr); }

#ifndef HAVE_STRLCAT
size_t strlcat(char* dst, const char* src, size_t n);
#endif // HAVE_STRLCAT
#ifndef HAVE_STRLCPY
size_t strlcpy(char* dst, const char* src, size_t n);
#endif // HAVE_STRLCPY

// norpchen font symbols
#define DEGREE_SYMBOL "\x1F"
#define DEGREE_SLASH "\x1F/"

// #define SQUARED_SYMBOL "\x1E"
// #define CUBED_SYMBOL "\x1D"
#define UNIT_ACCELERATION "mm/s\x1E"

#define MILLISECONDS_PER_SECOND 1000U
#define MILLISECONDS_PER_MINUTE (MILLISECONDS_PER_SECOND*60UL)

#define LED_NORMAL if (sleep_state & SLEEP_LCD_DIMMED) { lcd_lib_led_color(0,0,0); } else { lcd_lib_led_color(40,40,54); } led_update();
#define LED_GLOW   lcd_lib_led_color(8 + led_glow, 8 + led_glow, 32 + led_glow); led_update();
#define LED_GLOW_ERROR lcd_lib_led_color(led_glow,128-led_glow,led_glow); led_update();

#endif//ULTI_LCD2_LOW_LIB_H
