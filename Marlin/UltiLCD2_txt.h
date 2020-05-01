#ifndef ULTILCD2_TXT_H
#define ULTILCD2_TXT_H

// const char *lcd_str(uint8_t n);
extern const char* const lcd_text[] PROGMEM;

//#define PSTR_BACK               lcd_str(0)
//#define PSTR_CLICKRETURN        lcd_str(1)

#define PSTR_BACK               (const char*)(pgm_read_word(&(lcd_text[0])))
#define PSTR_CLICKRETURN        (const char*)(pgm_read_word(&(lcd_text[1])))

//extern const char* const PSTR_BACK PROGMEM;
//extern const char* const PSTR_CLICKRETURN PROGMEM;

#endif //ULTILCD2_TXT_H
