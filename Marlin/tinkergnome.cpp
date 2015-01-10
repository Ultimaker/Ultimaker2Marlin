#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "Marlin.h"
#include "cardreader.h"
#include "temperature.h"
//#include "lifetime_stats.h"
#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_maintenance.h"

#include "tinkergnome.h"

// #define LCD_TIMEOUT_TO_STATUS 30000
#define LCD_TIMEOUT_TO_STATUS (MILLISECONDS_PER_SECOND*30UL)		// 30 Sec.

// low pass filter constant, from 0.0 to 1.0 -- Higher numbers mean more smoothing, less responsiveness.
// 0.0 would be completely disabled, 1.0 would ignore any changes
#define LOW_PASS_SMOOTHING 0.95
#define DEFAULT_FILAMENT_AREA 6.3793966

#define LCD_DETAIL_CACHE_START ((LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))+1)
// #define LCD_DETAIL_CACHE_ID() lcd_cache[LCD_DETAIL_CACHE_START]
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
// #define LCD_DETAIL_CACHE_MATERIAL(n) (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*n])
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])

//#define LED_NORMAL() lcd_lib_led_color(48,48,60)
//#define LED_GLOW() lcd_lib_led_color(8 + led_glow, 8 + led_glow, 32 + led_glow)
//#define LED_GLOW_ERROR() lcd_lib_led_color(8+min(245,led_glow<<3),0,0);

// #define LED_FLASH() lcd_lib_led_color(8 + (led_glow<<3), 8 + min(255-8,(led_glow<<3)), 32 + min(255-32,led_glow<<3))
// #define LED_HEAT() lcd_lib_led_color(192 + led_glow/4, 8 + led_glow/4, 0)
#define LED_INPUT() lcd_lib_led_color(192+led_glow/4, 8+led_glow/4, 0)
// #define LED_DONE() lcd_lib_led_color(0, 8+led_glow, 8)
// #define LED_COOL() lcd_lib_led_color(0, 4, 16+led_glow)

#define DETAIL_LEN 20

// norpchen font symbols
#define DEGREE_SYMBOL "\x1F"
#define SQUARED_SYMBOL "\x1E"
#define CUBED_SYMBOL "\x1D"
#define PER_SECOND_SYMBOL "/s"

// static unsigned long timeout = 0;
static int16_t lastEncoderPos = 0;
static int16_t prevEncoderPos = 0;
static menuoption_t *selectedOption = NULL;
static char detailBuffer[DETAIL_LEN+1] = "";

// these are used to maintain a simple low-pass filter on the speeds - thanks norpchen
static float e_smoothed_speed[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0.0, 0.0, 0.0);
static float xy_speed = 0.0;


const uint8_t thermometerGfx[] PROGMEM = {
    5, 8, //size
    0x60, 0x9E, 0x81, 0x9E, 0x60
};

const uint8_t bedTempGfx[] PROGMEM = {
    7, 8, //size
    0x40, 0x4A, 0x55, 0x40, 0x4A, 0x55, 0x40
};

const uint8_t flowGfx[] PROGMEM = {
    7, 8, //size
    0x07, 0x08, 0x13, 0x7F, 0x13, 0x08, 0x07
};

const uint8_t clockInverseGfx[] PROGMEM = {
    7, 8, //size
    0x1C, 0x3E, 0x7F, 0x71, 0x77, 0x3E, 0x1C
};

const uint8_t pauseGfx[] PROGMEM = {
    7, 8, //size
    0x7F, 0x5D, 0x49, 0x41, 0x49, 0x5D, 0x7F
};

const uint8_t degreeGfx[] PROGMEM = {
    5, 8, //size
    0x06, 0x09, 0x09, 0x06, 0x00
};

const uint8_t fan1Gfx[] PROGMEM = {
    5, 8, //size
    0x26, 0x34, 0x08, 0x16, 0x32
};

const uint8_t speedGfx[] PROGMEM = {
    6, 8, //size
    0x22, 0x14, 0x08, 0x22, 0x14, 0x08
};

static void lcd_print_tune(menuoption_t &opt, char *detail, uint8_t n);
static void lcd_print_abort(menuoption_t &opt, char *detail, uint8_t n);
static void lcd_print_tune_speed(menuoption_t &opt, char *detail, uint8_t n);
static void lcd_print_tune_fan(menuoption_t &opt, char *detail, uint8_t n);
static void lcd_print_tune_nozzle0(menuoption_t &opt, char *detail, uint8_t n);
static void lcd_print_flow_nozzle0(menuoption_t &opt, char *detail, uint8_t n);
#if EXTRUDERS > 1
static void lcd_print_tune_nozzle1(menuoption_t &opt, char *detail, uint8_t n);
static void lcd_print_flow_nozzle1(menuoption_t &opt, char *detail, uint8_t n);
#endif
#if TEMP_SENSOR_BED != 0
static void lcd_print_tune_bed(menuoption_t &opt, char *detail, uint8_t n);
#endif

// print menu options
//This code uses the lcd_cache as buffer to store the filename, to save memory.

menuoption_t printOptions[] = {
                                  {(char *)"TUNE", LCD_CHAR_MARGIN_LEFT+2, BOTTOM_MENU_YPOS, 52, LCD_CHAR_HEIGHT, lcd_print_tune, STATE_NONE, ALIGN_CENTER}
                                 ,{(char *)"ABORT", LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+2, BOTTOM_MENU_YPOS, 52, LCD_CHAR_HEIGHT, lcd_print_abort, STATE_NONE, ALIGN_CENTER}
                                 ,{LCD_CACHE_FILENAME(0), LCD_CHAR_MARGIN_LEFT+12, 24, 24, LCD_CHAR_HEIGHT, lcd_print_flow_nozzle0, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
#if EXTRUDERS > 1
                                 ,{LCD_CACHE_FILENAME(1), LCD_CHAR_MARGIN_LEFT+42, 24, 24, LCD_CHAR_HEIGHT, lcd_print_flow_nozzle1, STATE_DISABLED, ALIGN_RIGHT | ALIGN_VCENTER}
#endif
                                 ,{LCD_CACHE_FILENAME(4), LCD_CHAR_MARGIN_LEFT+12, 33, 24, LCD_CHAR_HEIGHT, lcd_print_tune_nozzle0, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
#if EXTRUDERS > 1
                                 ,{LCD_CACHE_FILENAME(5), LCD_CHAR_MARGIN_LEFT+42, 33, 24, LCD_CHAR_HEIGHT, lcd_print_tune_nozzle1, STATE_DISABLED, ALIGN_RIGHT | ALIGN_VCENTER}
#endif
#if TEMP_SENSOR_BED != 0
                                 ,{LCD_CACHE_FILENAME(6), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 33, 24, LCD_CHAR_HEIGHT, lcd_print_tune_bed, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
#endif
                                 ,{LCD_CACHE_FILENAME(2), LCD_CHAR_MARGIN_LEFT+12, 42, 24, LCD_CHAR_HEIGHT, lcd_print_tune_speed, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
                                 ,{LCD_CACHE_FILENAME(3), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 42, 24, LCD_CHAR_HEIGHT, lcd_print_tune_fan, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
                              };

menuoption_t heatingOptions[] = {
                                  printOptions[0]
                                 ,printOptions[1]
                                 ,{LCD_CACHE_FILENAME(4), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 51-(EXTRUDERS*9)-(BED_MENU_OFFSET*9), 24, LCD_CHAR_HEIGHT, lcd_print_tune_nozzle0, STATE_DISABLED, ALIGN_RIGHT | ALIGN_VCENTER}
#if EXTRUDERS > 1
                                 ,{LCD_CACHE_FILENAME(5), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 42-(BED_MENU_OFFSET*9), 24, LCD_CHAR_HEIGHT, lcd_print_tune_nozzle1, STATE_DISABLED, ALIGN_RIGHT | ALIGN_VCENTER}
#endif
#if TEMP_SENSOR_BED != 0
                                 ,{LCD_CACHE_FILENAME(6), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 42, 24, LCD_CHAR_HEIGHT, lcd_print_tune_bed, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
#endif
                              };

uint8_t ui_mode = UI_MODE_STANDARD;

void tinkergnome_init()
{
    if (GET_UI_MODE() == UI_MODE_TINKERGNOME)
        ui_mode = UI_MODE_TINKERGNOME;
    else
        ui_mode = UI_MODE_STANDARD;
}

// draws a bargraph
void lcd_lib_draw_bargraph( uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, float value )
{
	lcd_lib_draw_box(x0, y0, x1, y1);
	value = constrain(value, 0.0, 1.0);
    // draw scale
    float segment = float(abs(x1-x0))/10;
	uint8_t ymin = y0+1;
	uint8_t ymax = y0+(y1-y0)/2-1;
    for (uint8_t i=1; i<10; ++i)
    {
        lcd_lib_draw_vline(x0 + i*segment, ymin, ymax);
    }

    // draw graph
	if (value<0.01) return;
	uint8_t xmax = value*abs(x1-x0)+x0+0.5;
	ymin = ymax+2;
	ymax = y1-1;
    for (uint8_t xpos=x0+1; xpos<xmax; xpos+=3)
    {
        lcd_lib_set (xpos, ymin, xpos+1, ymax);
    }
}


void lcd_lib_draw_heater(uint8_t x, uint8_t y, uint8_t heaterPower)
{
    // draw frame
    lcd_lib_draw_gfx(x, y, thermometerGfx);

//    lcd_lib_draw_hline(x+1, x+3, y);
//    lcd_lib_draw_hline(x+1, x+3, y+LCD_CHAR_HEIGHT);
//    lcd_lib_draw_vline(x+1, y+1, y+LCD_CHAR_HEIGHT-1);
//    lcd_lib_draw_vline(x+3, y+1, y+LCD_CHAR_HEIGHT-1);
//    lcd_lib_draw_vline(x,   y+LCD_CHAR_HEIGHT-2, y+LCD_CHAR_HEIGHT-1);
//    lcd_lib_draw_vline(x+4, y+LCD_CHAR_HEIGHT-2, y+LCD_CHAR_HEIGHT-1);

    if (heaterPower)
    {
        // draw power beam
        uint8_t beamHeight = min(LCD_CHAR_HEIGHT-2, (heaterPower*(LCD_CHAR_HEIGHT-2)/128)+1);
        lcd_lib_draw_vline(x+2, y+LCD_CHAR_HEIGHT-beamHeight-1, y+LCD_CHAR_HEIGHT-1);

        beamHeight = constrain(beamHeight, 0, 2);
        if (beamHeight>1)
        {
            lcd_lib_draw_vline(x+1, y+LCD_CHAR_HEIGHT-beamHeight-1, y+LCD_CHAR_HEIGHT-1);
            lcd_lib_draw_vline(x+3, y+LCD_CHAR_HEIGHT-beamHeight-1, y+LCD_CHAR_HEIGHT-1);
        }
    }
}

char* float_to_string2(float f, char* temp_buffer, const char* p_postfix)
{
    int32_t i = (f*10.0) + 0.5;
    char* c = temp_buffer;
    if (i < 0)
    {
        *c++ = '-';
        i = -i;
    }
    if (i >= 1000)
        *c++ = ((i/1000)%10)+'0';
    if (i >= 100)
        *c++ = ((i/100)%10)+'0';
    *c++ = ((i/10)%10)+'0';
    *c++ = '.';
    *c++ = (i%10)+'0';
    *c = '\0';
    if (p_postfix)
    {
        strcpy_P(c, p_postfix);
        c += strlen_P(p_postfix);
    }
    return c;
}

static char* int_to_time_string_tg(unsigned long i, char* temp_buffer)
{
    char* c = temp_buffer;
    uint8_t hours = i / 60 / 60;
    uint8_t mins = (i / 60) % 60;
    uint8_t secs = i % 60;

    if (!hours & !mins) {
        *c++ = '0';
        *c++ = '0';
        *c++ = ':';
        *c++ = '0' + secs / 10;
        *c++ = '0' + secs % 10;
    }else{
        if (hours > 99)
            *c++ = '0' + hours / 100;
        *c++ = '0' + (hours / 10) % 10;
        *c++ = '0' + hours % 10;
        *c++ = ':';
        *c++ = '0' + mins / 10;
        *c++ = '0' + mins % 10;
//        *c++ = 'h';
    }

    *c = '\0';
    return c;
}

static void lcd_progressline(uint8_t progress)
{
    progress = constrain(progress, 0, 124);
    if (progress)
    {
        lcd_lib_set(LCD_GFX_WIDTH-2, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), LCD_GFX_WIDTH-1, LCD_GFX_HEIGHT-1);
        lcd_lib_set(0, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), 1, LCD_GFX_HEIGHT-1);
    }
}

static void lcd_draw_menu_option(const menuoption_t &option)
{
    if (option.state & STATE_ACTIVE)
    {
        // draw frame
        lcd_lib_draw_box(option.left-2, option.top-2,
                         option.left+option.width, option.top+option.height+1);
    } else if (option.state & STATE_SELECTED)
    {
        // draw box
        lcd_lib_draw_box(option.left-2, option.top-1,
                         option.left+option.width, option.top+option.height);
        lcd_lib_set(option.left-1, option.top,
                    option.left+option.width-1, option.top+option.height-1);
    }

    const char* split = strchr(option.title, '|');

    if (split)
    {
        char buf[10];
        strncpy(buf, option.title, split - option.title);
        buf[split - option.title] = '\0';
        ++split;

        uint8_t textX1;
        uint8_t textY1;
        uint8_t textX2;
        uint8_t textY2;

        // calculate text position
        if (option.textalign & ALIGN_LEFT)
        {
            textX1 = textX2 = option.left;
        }
        else if (option.textalign & ALIGN_RIGHT)
        {
            textX1 = option.left + option.width - (LCD_CHAR_SPACING * strlen(buf));
            textX2 = option.left + option.width - (LCD_CHAR_SPACING * strlen(split));
        }
        else // if (option.textalign & ALIGN_HCENTER)
        {
            textX1 = option.left + option.width/2 - (LCD_CHAR_SPACING/2 * strlen(buf));
            textX2 = option.left + option.width/2 - (LCD_CHAR_SPACING/2 * strlen(split));
        }

        if (option.textalign & ALIGN_TOP)
        {
            textY1 = option.top;
            textY2 = option.top + LCD_LINE_HEIGHT;
        }
        else if (option.textalign & ALIGN_BOTTOM)
        {
            textY2 = option.top + option.height - LCD_CHAR_HEIGHT;
            textY1 = textY2 - LCD_LINE_HEIGHT;
        }
        else // if (option.textalign & ALIGN_VCENTER)
        {
            textY1 = option.top + option.height/2 - LCD_LINE_HEIGHT/2 - LCD_CHAR_HEIGHT/2;
            textY2 = textY1 + LCD_LINE_HEIGHT;
        }

        if (option.state & STATE_SELECTED)
        {
            lcd_lib_clear_string(textX1, textY1, buf);
            lcd_lib_clear_string(textX2, textY2, split);
        } else {
            lcd_lib_draw_string(textX1, textY1, buf);
            lcd_lib_draw_string(textX2, textY2, split);
        }
    }else{
        // calculate text position
        uint8_t textX;
        uint8_t textY;

        if (option.textalign & ALIGN_LEFT)
            textX = option.left;
        else if (option.textalign & ALIGN_RIGHT)
            textX = option.left + option.width - (LCD_CHAR_SPACING * strlen(option.title));
        else // if (option.textalign & ALIGN_HCENTER)
            textX = option.left + option.width/2 - (LCD_CHAR_SPACING/2 * strlen(option.title));

        if (option.textalign & ALIGN_TOP)
            textY = option.top;
        else if (option.textalign & ALIGN_BOTTOM)
            textY = option.top + option.height - LCD_CHAR_HEIGHT;
        else // if (option.textalign & ALIGN_VCENTER)
            textY = option.top + option.height/2 - LCD_CHAR_HEIGHT/2;


        if (option.state & STATE_SELECTED)
        {
            lcd_lib_clear_string(textX, textY, option.title);
        } else {
            lcd_lib_draw_string(textX, textY, option.title);
        }
    }
}

static void lcd_tune_value(char *title, int &value, int _min, int _max, const char *p_postfix)
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        lcd_lib_tick();
        value = constrain(int(value) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
        lcd_lib_encoder_pos = 0;
    }
    int_to_string(value, title, p_postfix);
}

//static void lcd_tune_value(char *title, uint16_t &value, uint16_t _min, uint16_t _max, const char *p_postfix)
//{
//    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
//    {
//        value = constrain(int(value) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
//        lcd_lib_encoder_pos = 0;
//    }
//        int_to_string(value, title+strlen(title), p_postfix);
//}

static void lcd_tune_byte(char *title, uint8_t &value, uint8_t _min, uint8_t _max, const char *p_postfix)
{
    int temp_value = int((float(value)*_max/255)+0.5);
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        lcd_lib_tick();
        temp_value += lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM;
        temp_value = constrain(temp_value, _min, _max);
        value = uint8_t((float(temp_value)*255/_max)+0.5);
        lcd_lib_encoder_pos = 0;
    }
    int_to_string(temp_value, title, p_postfix);
}

static void lcd_tune_temperature(char *title, int &value, int _min, int _max, const char *p_postfix)
{
    if ((value > 0) && (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0))
    {
        lcd_lib_tick();
        value = constrain(int(value) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
        lcd_lib_encoder_pos = 0;
    }
    int_to_string(value, title, p_postfix);
}

static void lcd_print_tune(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.state & STATE_ACTIVE) {
        // switch to tune menu
        opt.state = STATE_NONE;
        lcd_change_to_menu(lcd_menu_print_tune, 0);
    }
    if (detail) {
        *detail = 0;
    }
}

static void lcd_print_abort(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.state & STATE_ACTIVE) {
        // switch to abort menu
        opt.state = STATE_NONE;
        lcd_change_to_menu(lcd_menu_print_abort, MAIN_MENU_ITEM_POS(1));
    }
    if (detail) {
        *detail = 0;
    }
}

static void lcd_print_tune_speed(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.state & STATE_ACTIVE) {
        lcd_tune_value(opt.title, feedmultiply, 0, 999, PSTR("%"));
    }
    if (detail) {
        strncpy_P(detail, PSTR("Speed "), n);
        if (n>6)
        {
            char buffer[10];
            int_to_string(xy_speed+0.5, buffer, PSTR("mm" PER_SECOND_SYMBOL));
            strncpy(detail+6, buffer, n-6);
        }
    }
}

static void lcd_print_tune_fan(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.state & STATE_ACTIVE) {
        lcd_tune_byte(opt.title, fanSpeed, 0, 100, PSTR("%"));
    }
    if (detail) {
        strncpy_P(detail, PSTR("Fan"), n);
        lcd_lib_draw_bargraph(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING, 5, LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT, 11, float(fanSpeed)/255);
    }
}

static void lcd_print_tune_nozzle0(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.isActive()) {
        lcd_tune_temperature(opt.title, target_temperature[0], 0, HEATER_0_MAXTEMP - 15, PSTR(DEGREE_SYMBOL));
    }
    if (detail) {
        strncpy_P(detail, PSTR("Nozzle temperature"), n);
    }
}

static void lcd_print_flow_nozzle0(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.isActive() && target_temperature[0]>0) {
        lcd_tune_value(opt.title, extrudemultiply[0], 0, 999, PSTR("%"));
    }
    if (detail) {
//        strncpy_P(detail, PSTR("Material flow"), n);
        strncpy_P(detail, PSTR("Flow "), n);
        if (n>5)
        {
            char buffer[12];
            float_to_string2(e_smoothed_speed[0], buffer, PSTR("mm" CUBED_SYMBOL PER_SECOND_SYMBOL));
            strncpy(detail+5, buffer, n-5);
        }
    }
}

#if EXTRUDERS > 1
static void lcd_print_tune_nozzle1(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.isActive())
    {
        lcd_tune_temperature(opt.title, target_temperature[1], 0, HEATER_1_MAXTEMP - 15, PSTR(DEGREE_SYMBOL));
    }
    if (detail) {
        strncpy_P(detail, PSTR("Nozzle2 temperature"), n);
    }
}

static void lcd_print_flow_nozzle1(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.isActive() && target_temperature[1]>0)
    {
        lcd_tune_value(opt.title, extrudemultiply[1], 0, 999, PSTR("%"));
    }
    if (detail) {
        strncpy_P(detail, PSTR("Flow2 "), n);
        if (n>6)
        {
            char buffer[12];
            float_to_string2(e_smoothed_speed[1], buffer, PSTR("mm" CUBED_SYMBOL PER_SECOND_SYMBOL));
            strncpy(detail+6, buffer, n-6);
        }
    }
}
#endif

#if TEMP_SENSOR_BED != 0
static void lcd_print_tune_bed(menuoption_t &opt, char *detail, uint8_t n)
{
    if (opt.state & STATE_ACTIVE) {
        lcd_tune_temperature(opt.title, target_temperature_bed, 0, BED_MAXTEMP - 15, PSTR(DEGREE_SYMBOL));
    }
    if (detail) {
        strncpy_P(detail, PSTR("Buildplate temp."), n);
    }
}
#endif

static void lcd_reset_menu_options()
{
    // reset selection
    // timeout = ULONG_MAX;
    *detailBuffer = 0;
    lastEncoderPos = lcd_lib_encoder_pos = prevEncoderPos;
    if (selectedOption)
    {
        selectedOption->state = STATE_NONE;
        selectedOption = NULL;
        LED_NORMAL();
    }
}

static void lcd_process_menu_options(menuoption_t options[], uint8_t len)
{

    if ((lcd_lib_encoder_pos != lastEncoderPos) || lcd_lib_button_pressed)
    {
        // reset timeout value
        lastEncoderPos = lcd_lib_encoder_pos;

        // process active item
        if (selectedOption && selectedOption->isActive())
        {
            if (lcd_lib_button_pressed)
            {
                lcd_lib_beep();
                lcd_reset_menu_options();
            }else if (selectedOption->callbackFunc)
            {
                selectedOption->callbackFunc(*selectedOption, detailBuffer, DETAIL_LEN);
                lastEncoderPos = lcd_lib_encoder_pos;
            }
        }else if (lcd_lib_encoder_pos != ENCODER_NO_SELECTION)
        {
            // adjust encoder position
            if (lcd_lib_encoder_pos < 0)
                lcd_lib_encoder_pos += len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
            if (lcd_lib_encoder_pos >= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
                lcd_lib_encoder_pos -= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;

            if (selectedOption)
                selectedOption->state = STATE_NONE;

            // determine new selection
            int16_t index = (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM);

            if ((index >= 0) && (index < len))
            {
                selectedOption = &options[index];
                prevEncoderPos = lcd_lib_encoder_pos;
                // get detail text
                selectedOption->callbackFunc(*selectedOption, detailBuffer, DETAIL_LEN);
                if (lcd_lib_button_pressed && options[index].isEnabled())
                {
                    selectedOption->state = STATE_ACTIVE;
                    if (index>1)
                    {
                        // "instant tuning"
                        lcd_lib_encoder_pos = 0;
                        LED_INPUT();
                        lcd_lib_beep();
                    }else{
                        // process standard question menu
                        if (selectedOption->callbackFunc)
                        {
                            selectedOption->callbackFunc(*selectedOption, 0, 0);
                        }
                        selectedOption = 0;
                    }
                } else {
                    selectedOption->state = STATE_SELECTED;
                    LED_NORMAL();
                }
            }
            else{
                selectedOption = 0;
            }
        }
    }
    else if (selectedOption)
    {
        if (millis() - last_user_interaction > LCD_TIMEOUT_TO_STATUS)
        {
            // timeout occurred - reset selection
            lcd_reset_menu_options();
        }
        else
        {
            // refresh detail text
            selectedOption->callbackFunc(*selectedOption, detailBuffer, DETAIL_LEN);
        }
    }
}

void lcd_menu_print_heatup_tg()
{
    // lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));
    lcd_basic_screen();
    lcd_lib_draw_string_left(5, card.longFilename);
    lcd_lib_draw_hline(3, 124, 13);

#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > target_temperature_bed - 10)
    {
#endif
        for(uint8_t e=0; e<EXTRUDERS; e++)
        {
            if (target_temperature[e] > 0)
                continue;
            target_temperature[e] = material[e].temperature;
        }

        if (current_temperature_bed >= target_temperature_bed - TEMP_WINDOW * 2 && !is_command_queued())
        {
            bool ready = true;
            for(uint8_t e=0; e<EXTRUDERS; e++)
                if (current_temperature[e] < target_temperature[e] - TEMP_WINDOW)
                    ready = false;

            if (ready)
            {
                lcd_reset_menu_options();
                doStartPrint();
                lcd_remove_menu();
                lcd_add_menu(lcd_menu_printing_tg, ENCODER_NO_SELECTION);
            }
        }
#if TEMP_SENSOR_BED != 0
    }
#endif

    uint8_t progress = 125;
    for(uint8_t e=0; e<EXTRUDERS; e++)
    {
        if (target_temperature[e] < 1)
            continue;
        if (current_temperature[e] > 20)
            progress = min(progress, (current_temperature[e] - 20) * 125 / (target_temperature[e] - 20 - TEMP_WINDOW));
        else
            progress = 0;
    }
#if TEMP_SENSOR_BED != 0
    if ((current_temperature_bed > 20) && (target_temperature_bed > 20+TEMP_WINDOW))
        progress = min(progress, (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW));
    else if (target_temperature_bed > current_temperature_bed - 20)
        progress = 0;
#endif

    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;

    lcd_lib_draw_string_leftP(15, PSTR("Heating up... "));

    char buffer[8];
    int_to_string(progress*100/124, buffer, PSTR("%"));
    lcd_lib_draw_string_right(15, buffer);

    lcd_progressline(progress);

    // bed temperature
    uint8_t ypos = 41;
#if TEMP_SENSOR_BED != 0
    // bed temperature
//    int_to_string(target_temperature_bed, buffer, PSTR("C"));
//    lcd_lib_draw_string_right(ypos, buffer);
    uint8_t index = 1+EXTRUDERS+BED_MENU_OFFSET;
    if (!heatingOptions[index].isActive())
    {
        int_to_string(target_temperature_bed, heatingOptions[index].title, PSTR(DEGREE_SYMBOL));
    }
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature_bed, buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(-1));
    ypos -= 9;
#endif // TEMP_SENSOR_BED
#if EXTRUDERS > 1
    // temperature second extruder
//    int_to_string(target_temperature[1], buffer, PSTR("C"));
//    lcd_lib_draw_string_right(ypos, buffer);
    if (!heatingOptions[3].isActive())
    {
        if (target_temperature[1] > 0)
        {
            heatingOptions[3].state &= ~STATE_DISABLED;
        }
        else{
            heatingOptions[3].state |= STATE_DISABLED;
        }
        int_to_string(target_temperature[1], heatingOptions[3].title, PSTR(DEGREE_SYMBOL));
    }
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[1], buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(1));
    ypos -= 9;
#endif // EXTRUDERS
    // temperature first extruder
    if (!heatingOptions[2].isActive())
    {
        if (target_temperature[0] > 0)
        {
            heatingOptions[2].state &= ~STATE_DISABLED;
        }
        else{
            heatingOptions[2].state |= STATE_DISABLED;
        }
        int_to_string(target_temperature[0], heatingOptions[2].title, PSTR(DEGREE_SYMBOL));
    }
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[0], buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(0));

    lcd_process_menu_options(heatingOptions, sizeof(heatingOptions)/sizeof(heatingOptions[0]));

    for (uint8_t index=0; index<sizeof(heatingOptions)/sizeof(heatingOptions[0]); ++index) {
        lcd_draw_menu_option(heatingOptions[index]);
    }

    lcd_lib_update_screen();
}


unsigned long predictTimeLeft()
{
    float printTimeMs = (millis() - starttime);
    float printTimeSec = printTimeMs / 1000L;
    float totalTimeMs = float(printTimeMs) * float(card.getFileSize()) / float(card.getFilePos());
    static float totalTimeSmoothSec;
    totalTimeSmoothSec = (totalTimeSmoothSec * 999L + totalTimeMs / 1000L) / 1000L;
    if (isinf(totalTimeSmoothSec))
        totalTimeSmoothSec = totalTimeMs;

    if (LCD_DETAIL_CACHE_TIME() == 0 && printTimeSec < 60)
    {
        totalTimeSmoothSec = totalTimeMs / 1000;
        return 0;
    }else{
        unsigned long totalTimeSec;
        if (printTimeSec < LCD_DETAIL_CACHE_TIME() / 2)
        {
            float f = float(printTimeSec) / float(LCD_DETAIL_CACHE_TIME() / 2);
            if (f > 1.0)
                f = 1.0;
            totalTimeSec = float(totalTimeSmoothSec) * f + float(LCD_DETAIL_CACHE_TIME()) * (1 - f);
        }else{
            totalTimeSec = totalTimeSmoothSec;
        }

        return (printTimeSec > totalTimeSec) ? 1 : totalTimeSec - printTimeSec;
    }
}

void lcd_menu_printing_tg()
{
    static block_t *lastBlock = 0;

    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    unsigned long timeLeftSec;
    char buffer[16];

    if (isPauseRequested()) {
        lcd_menu_print_pause();
    }

    switch(printing_state)
    {
    default:
        if (current_block != lastBlock)
        {
            lastBlock = current_block;
            // calculate speeds - thanks norpchen
            if (current_block!=NULL)
            {

                if (current_block->steps_e > 0)
                {
                    xy_speed = current_block->nominal_speed;

    //                float block_time = current_block->millimeters / current_block->nominal_speed;
    //                float mm_e = current_block->steps_e / axis_steps_per_unit[E_AXIS];

                    // calculate live extrusion rate from e speed and filament area
                    float speed_e = current_block->steps_e * current_block->nominal_rate / axis_steps_per_unit[E_AXIS] / current_block->step_event_count;
                    float volume = (volume_to_filament_length[current_block->active_extruder] < 0.99) ? speed_e / volume_to_filament_length[current_block->active_extruder] : speed_e*DEFAULT_FILAMENT_AREA;

                    if (speed_e>0 && speed_e<15.0)
                    {
                        e_smoothed_speed[current_block->active_extruder] = (e_smoothed_speed[current_block->active_extruder]*LOW_PASS_SMOOTHING) + ( volume *(1.0-LOW_PASS_SMOOTHING));
                    }
                }
            }
        }

        if (selectedOption && *detailBuffer)
        {
            lcd_lib_draw_string_left(5, detailBuffer);
        }
        else
        {
            lcd_lib_draw_string_left(5, card.longFilename);
        }

        if (card.pause)
        {
            lcd_lib_draw_gfx(54, 15, pauseGfx);
            lcd_lib_draw_stringP(64, 15, (movesplanned() < 1) ? PSTR("Paused...") : PSTR("Pausing..."));
        }
        else
        {
            // time left
            timeLeftSec = predictTimeLeft();
            if (timeLeftSec > 0)
            {
                lcd_lib_draw_gfx(54, 15, clockInverseGfx);
                int_to_time_string_tg(timeLeftSec, buffer);
                lcd_lib_draw_string(64, 15, buffer);

                // draw progress string right aligned
                int_to_string(progress*100/124, buffer, PSTR("%"));
                lcd_lib_draw_string_right(15, buffer);
                lcd_progressline(progress);
            }
        }

        break;
    case PRINT_STATE_WAIT_USER:
        lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
        lcd_reset_menu_options();
        // lcd_lib_draw_string_left(5, PSTR("Paused..."));
        lcd_lib_draw_string_left(5, card.longFilename);
        lcd_lib_draw_gfx(54, 15, pauseGfx);
        if (movesplanned() < 1)
        {
            lcd_lib_draw_stringP(64, 15, PSTR("Paused..."));
            lcd_lib_draw_string_leftP(BOTTOM_MENU_YPOS, PSTR("Click to continue..."));
        }
        else
        {
            lcd_lib_draw_stringP(64, 15, PSTR("Pausing..."));
        }
        if (!led_glow)
        {
            lcd_lib_tick();
        }
        break;
    case PRINT_STATE_HEATING:
        if (selectedOption && *detailBuffer)
        {
            lcd_lib_draw_string_left(5, detailBuffer);
        }
        else
        {
            lcd_lib_draw_string_leftP(5, PSTR("Heating"));
        }
        break;
    case PRINT_STATE_HEATING_BED:
        if (selectedOption && *detailBuffer)
        {
            lcd_lib_draw_string_left(5, detailBuffer);
        }
        else
        {
            lcd_lib_draw_string_leftP(5, PSTR("Heating buildplate"));
        }
        break;
    }

    // all printing states
    // z position
    lcd_lib_draw_string_leftP(15, PSTR("Z"));
    float_to_string(current_position[Z_AXIS], buffer, NULL);
    lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT+12, 15, buffer);

    uint8_t index=2;
    // flow
    // lcd_lib_draw_string_leftP(printOptions[index].top, PSTR("*"));
    lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT-1, printOptions[index].top, flowGfx);
    if (!printOptions[index].isActive())
    {
        int_to_string(extrudemultiply[0], printOptions[index].title, PSTR("%"));
    }
#if EXTRUDERS > 1
    if (!printOptions[++index].isActive())
    {
        int_to_string(extrudemultiply[1], printOptions[index].title, PSTR("%"));
         if (target_temperature[1] > 0)
            printOptions[index].state &= ~STATE_DISABLED;
        else {
            printOptions[index].state |= STATE_DISABLED;
        }
    }
#endif
    // temperature first extruder
    lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, printOptions[++index].top, thermometerGfx);
    // lcd_lib_draw_string_leftP(printOptions[++index].top, PSTR("Temp"));
    if (!printOptions[index].isActive())
    {
        int_to_string(dsp_temperature[0], printOptions[index].title, PSTR(DEGREE_SYMBOL));
    }
#if EXTRUDERS > 1
    // temperature second extruder
    if (!printOptions[++index].isActive())
    {
        int_to_string(dsp_temperature[1], printOptions[index].title, PSTR(DEGREE_SYMBOL));
        if (target_temperature[1] > 0)
            printOptions[index].state &= ~STATE_DISABLED;
        else {
            printOptions[index].state |= STATE_DISABLED;
        }
    }
#endif
#if TEMP_SENSOR_BED != 0
    // temperature build-plate
    ++index;
    lcd_lib_draw_gfx(printOptions[index].left-12, printOptions[index].top, bedTempGfx);
    if (!printOptions[index].isActive())
    {
        int_to_string(dsp_temperature_bed, printOptions[index].title, PSTR(DEGREE_SYMBOL));
    }
#endif

    // speed
    ++index;
    lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, printOptions[index].top, speedGfx);
    // lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+1, printOptions[index].top, PSTR("\x7E"));

    if (!printOptions[index].isActive())
    {
        int_to_string(feedmultiply, printOptions[index].title, PSTR("%"));
    }

    // fan speed
    ++index;
    static uint8_t fanAnimate = 0;
    static uint8_t prevFanSpeed = 0;

    // start animation
    if (!fanAnimate && fanSpeed!=prevFanSpeed)
        fanAnimate = 32;
    if ((fanSpeed == 0) || (!fanAnimate) || (fanAnimate%4))
    {
        lcd_lib_draw_gfx(printOptions[index].left-11, printOptions[index].top, fan1Gfx);
    }
    if (fanAnimate && !(led_glow%16))
    {
        --fanAnimate;
    }
    prevFanSpeed = fanSpeed;
    if (!printOptions[index].isActive())
    {
        int_to_string(int(fanSpeed*100/255 + 0.5f), printOptions[index].title, PSTR("%"));
    }

    index = 0;
    if (printing_state == PRINT_STATE_WAIT_USER)
    {
        index += 2;
    }
    else
    {
        lcd_process_menu_options(printOptions, sizeof(printOptions)/sizeof(printOptions[0]));
        if (!card.pause)
        {
            const char *message = lcd_getstatus();
            if (!selectedOption && message && *message)
            {
                lcd_lib_draw_string_left(BOTTOM_MENU_YPOS, message);
                index += 2;
            }
        }
    }

    for (; index<sizeof(printOptions)/sizeof(printOptions[0]); ++index) {
        lcd_draw_menu_option(printOptions[index]);
    }

    lcd_lib_update_screen();
}

static char* lcd_uimode_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR(" Standard Mode"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR(" Geek Mode"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    if (nr-1 == ui_mode)
        card.longFilename[0] = '>';

    return card.longFilename;
}

static void lcd_uimode_details(uint8_t nr)
{
    // nothing displayed
}

void lcd_menu_maintenance_uimode()
{
    lcd_scroll_menu(PSTR("Expert settings"), 3, lcd_uimode_item, lcd_uimode_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            if (ui_mode != UI_MODE_STANDARD)
            {
                ui_mode = UI_MODE_STANDARD;
                SET_UI_MODE(ui_mode);
                lcd_change_to_previous_menu();
                lcd_lib_encoder_pos -= ENCODER_TICKS_PER_SCROLL_MENU_ITEM;
                return;
            }
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            if (!(ui_mode & UI_MODE_TINKERGNOME))
            {
                ui_mode = UI_MODE_TINKERGNOME;
                SET_UI_MODE(ui_mode);
                // adjust menu stack
                lcd_change_to_previous_menu();
                uint16_t encoderPos = lcd_lib_encoder_pos + ENCODER_TICKS_PER_SCROLL_MENU_ITEM;
                lcd_change_to_previous_menu();
                lcd_replace_menu(lcd_menu_maintenance_advanced, encoderPos);
                return;
            }
        }
        lcd_change_to_previous_menu();
    }
}

#endif//ENABLE_ULTILCD2
