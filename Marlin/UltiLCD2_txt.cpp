#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2_txt.h"


static const char* const PSTR_000 PROGMEM = "BACK";
static const char* const PSTR_001 PROGMEM = "Click to return";

static const char* const PSTR_002 PROGMEM = "Test";

const char* const lcd_text[] PROGMEM =
{
  PSTR_000,
  PSTR_001,
};

const char *lcd_str(uint8_t n)
{
    return (const char*)(pgm_read_word(&(lcd_text[n])));
}


#endif //ENABLE_ULTILCD2
