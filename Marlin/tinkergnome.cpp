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
#include "UltiLCD2_menu_utils.h"

#include "tinkergnome.h"

// low pass filter constant, from 0.0 to 1.0 -- Higher numbers mean more smoothing, less responsiveness.
// 0.0 would be completely disabled, 1.0 would ignore any changes
#define LOW_PASS_SMOOTHING 0.95
#define DEFAULT_FILAMENT_AREA 6.3793966

#define LED_DIM_TIME 0		    // 0 min -> off
#define LED_DIM_MAXTIME 240		// 240 min
#define DIM_LEVEL 0

//#define LED_NORMAL() lcd_lib_led_color(48,48,60)
//#define LED_GLOW() lcd_lib_led_color(8 + led_glow, 8 + led_glow, 32 + led_glow)
//#define LED_GLOW_ERROR() lcd_lib_led_color(8+min(245,led_glow<<3),0,0);

// #define LED_FLASH() lcd_lib_led_color(8 + (led_glow<<3), 8 + min(255-8,(led_glow<<3)), 32 + min(255-32,led_glow<<3))
// #define LED_HEAT() lcd_lib_led_color(192 + led_glow/4, 8 + led_glow/4, 0)
// #define LED_DONE() lcd_lib_led_color(0, 8+led_glow, 8)
// #define LED_COOL() lcd_lib_led_color(0, 4, 16+led_glow)

// norpchen font symbols
#define DEGREE_SYMBOL "\x1F"
#define SQUARED_SYMBOL "\x1E"
#define CUBED_SYMBOL "\x1D"
#define HOME_SYMBOL "\x1C"
#define PER_SECOND_SYMBOL "/s"

// #define AXIS_MOVE_DELAY 10      // 10ms
#define MOVE_DELAY 500

// these are used to maintain a simple low-pass filter on the speeds - thanks norpchen
static float e_smoothed_speed[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0.0, 0.0, 0.0);
static float xy_speed = 0.0;

static float target_position[NUM_AXIS];
static bool bResetTargetPos = false;
static int8_t movingSpeed = 0;
static bool delayMove = false;

const uint8_t thermometerGfx[] PROGMEM = {
    5, 8, //size
    0x60, 0x9E, 0x81, 0x9E, 0x60
};

//const uint8_t bedTempGfx[] PROGMEM = {
//    5, 8, //size
//    0x3E, 0x22, 0x54, 0x22, 0x3E
//};

const uint8_t bedTempGfx[] PROGMEM = {
    7, 8, //size
    0x40, 0x4A, 0x55, 0x40, 0x4A, 0x55, 0x40
};

const uint8_t flowGfx[] PROGMEM = {
    7, 8, //size
    0x07, 0x08, 0x13, 0x7F, 0x13, 0x08, 0x07
};

//const uint8_t clockGfx[] PROGMEM = {
//    5, 8, //size
//    0x1C, 0x22, 0x2A, 0x26, 0x1C
//};

const uint8_t clockGfx[] PROGMEM = {
    7, 8, //size
    0x1C, 0x22, 0x41, 0x4F, 0x49, 0x22, 0x1C
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

//const uint8_t fanGfx[] PROGMEM = {
//    7, 8, //size
//    0xE0, 0x77, 0x0f, 0x08, 0x18, 0x18, 0x10
//};

//const uint8_t fanGfx[] PROGMEM = {
//    6, 8, //size
//    0x63, 0x77, 0x00, 0x08, 0x18, 0x18
//};

const uint8_t fan1Gfx[] PROGMEM = {
    5, 8, //size
    0x26, 0x34, 0x08, 0x16, 0x32
};

const uint8_t speedGfx[] PROGMEM = {
    6, 8, //size
    0x22, 0x14, 0x08, 0x22, 0x14, 0x08
};

const uint8_t revSpeedGfx[] PROGMEM = {
    6, 8, //size
    0x08, 0x14, 0x22, 0x08, 0x14, 0x22
};

const uint8_t homeGfx[] PROGMEM = {
    7, 8, //size
    0x08, 0x7C, 0x46, 0x6F, 0x46, 0x7C, 0x08
};

static void lcd_print_tune();
static void lcd_print_abort();
static void lcd_print_tune_speed();
static void lcd_print_tune_fan();
static void lcd_print_tune_nozzle0();
static void lcd_print_flow_nozzle0();
#if EXTRUDERS > 1
static void lcd_print_tune_nozzle1();
static void lcd_print_flow_nozzle1();
#endif
#if TEMP_SENSOR_BED != 0
static void lcd_print_tune_bed();
#endif
static void lcd_lib_draw_bargraph( uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, float value );
static char* float_to_string2(float f, char* temp_buffer, const char* p_postfix);

uint8_t ui_mode = UI_MODE_STANDARD;
uint16_t led_timeout = LED_DIM_TIME;

void tinkergnome_init()
{
    if (GET_UI_MODE() == UI_MODE_TINKERGNOME)
        ui_mode = UI_MODE_TINKERGNOME;
    else
        ui_mode = UI_MODE_STANDARD;

    led_timeout = constrain(GET_LED_TIMEOUT(), 0, LED_DIM_MAXTIME);
}


// return standard menu option
static void get_standard_menuoption(uint8_t nr, uint8_t &index, menu_t &opt)
{
    if (nr == index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_tune);
    }
    else if (nr == index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_abort);
    }
}

// return heatup menu option
static const menu_t & get_heatup_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    get_standard_menuoption(nr, index, opt);
    if (nr == index++)
    {
        // temp nozzle 1
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // temp nozzle 2
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle1);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // temp buildplate
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_bed);
    }
#endif
    return opt;
}

// return print menu option
static const menu_t & get_print_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    get_standard_menuoption(nr, index, opt);

    if (nr == index++)
    {
        // flow nozzle 1
        opt.setData(MENU_INPLACE_EDIT, lcd_print_flow_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // flow nozzle 2
        opt.setData(MENU_INPLACE_EDIT, lcd_print_flow_nozzle1);
    }
#endif
    else if (nr == index++)
    {
        // temp nozzle 1
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // temp nozzle 2
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle1);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // temp buildplate
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_bed);
    }
#endif
    else if (nr == index++)
    {
        // speed
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_speed);
    }
    else if (nr == index++)
    {
        // fan
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_fan);
    }
    return opt;
}

// draws a bargraph
static void lcd_lib_draw_bargraph( uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, float value )
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

static char* float_to_string2(float f, char* temp_buffer, const char* p_postfix)
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

static void lcd_tune_value(int &value, int _min, int _max)
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        lcd_lib_tick();
        value = constrain(int(value) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
        lcd_lib_encoder_pos = 0;
    }
}

static void lcd_tune_value(float &value, float _min, float _max, float _step)
{
    if (lcd_lib_encoder_pos != 0)
    {
        lcd_lib_tick();
        value = constrain(value + (lcd_lib_encoder_pos * _step), _min, _max);
        lcd_lib_encoder_pos = 0;
    }
}

static void lcd_tune_byte(uint8_t &value, uint8_t _min, uint8_t _max)
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
}

static void lcd_tune_temperature(int &value, int _min, int _max)
{
//    if ((value > 0) && (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0))
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        lcd_lib_tick();
        value = constrain(value + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
        lcd_lib_encoder_pos = 0;
    }
}

static void lcd_print_tune()
{
    // switch to tune menu
    menu.add_menu(menu_t(lcd_menu_print_tune));
}

static void lcd_print_abort()
{
    // switch to abort menu
    menu.add_menu(menu_t(lcd_menu_print_abort, MAIN_MENU_ITEM_POS(1)));
}

static void lcd_print_tune_speed()
{
    lcd_tune_value(feedmultiply, 0, 999);
}

static void lcd_print_tune_fan()
{
    lcd_tune_byte(fanSpeed, 0, 100);
}

static void lcd_print_tune_nozzle0()
{
    lcd_tune_temperature(target_temperature[0], 0, HEATER_0_MAXTEMP - 15);
}

static void lcd_print_flow_nozzle0()
{
    if (target_temperature[0]>0) {
        lcd_tune_value(extrudemultiply[0], 0, 999);
    }
}

#if EXTRUDERS > 1
static void lcd_print_tune_nozzle1()
{
    lcd_tune_temperature(target_temperature[1], 0, HEATER_1_MAXTEMP - 15);
}

static void lcd_print_flow_nozzle1()
{
    if (target_temperature[1]>0)
    {
        lcd_tune_value(extrudemultiply[1], 0, 999);
    }
}
#endif

#if TEMP_SENSOR_BED != 0
static void lcd_print_tune_bed()
{
    lcd_tune_temperature(target_temperature_bed, 0, BED_MAXTEMP - 15);
}
#endif

void drawStandardSubmenu (uint8_t nr, uint8_t flags, bool &bStatusline)
{
    if (nr == 0)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+2
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("TUNE")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == 1)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+2
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("ABORT")
                                , ALIGN_CENTER
                                , flags);
    }
}

void drawHeatupSubmenu (uint8_t nr, uint8_t flags, bool &bStatusline)
{
    uint8_t index(2);
    if (nr < index)
    {
        drawStandardSubmenu(nr, flags, bStatusline);
    }
    else if (nr == index++)
    {
        // temp nozzle 1
        if (flags & MENU_SELECTED)
        {
#if EXTRUDERS < 2
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Nozzle "));
#else
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Nozzle(1) "));
#endif
            int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR(DEGREE_SYMBOL"/")), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string(target_temperature[0], LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                          , 51-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                          , 24
                          , LCD_CHAR_HEIGHT
                          , LCD_CACHE_FILENAME(0)
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // temp nozzle 2
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Nozzle(2) "));
            int_to_string(target_temperature[1], int_to_string(dsp_temperature[1], LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR(DEGREE_SYMBOL"/")), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string(target_temperature[1], LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // temp buildplate
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Buildplate "));
            int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR(DEGREE_SYMBOL"/")), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string(target_temperature_bed, LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
}

void drawPrintSubmenu (uint8_t nr, uint8_t flags, bool &bStatusline)
{
    uint8_t index(2);
    if (nr < index)
    {
        drawStandardSubmenu(nr, flags, bStatusline);
    }
    else if (nr == index++)
    {
        // flow nozzle 1
        if (flags & MENU_SELECTED)
        {
#if EXTRUDERS < 2
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Flow "));
#else
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Flow(1) "));
#endif
            float_to_string2(e_smoothed_speed[0], LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR("mm" CUBED_SYMBOL PER_SECOND_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string(extrudemultiply[0], LCD_CACHE_FILENAME(0), PSTR("%"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                              , 24
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // flow nozzle 2
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Flow(2) "));
            float_to_string2(e_smoothed_speed[1], LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR("mm" CUBED_SYMBOL PER_SECOND_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string(extrudemultiply[1], LCD_CACHE_FILENAME(0), PSTR("%"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+42
                              , 24
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
    else if (nr == index++)
    {
        // temp nozzle 1
        if (flags & MENU_SELECTED)
        {
#if EXTRUDERS < 2
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Nozzle "));
#else
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Nozzle(1) "));
#endif
            int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR(DEGREE_SYMBOL"/")), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string((flags & MENU_ACTIVE) ? target_temperature[0] : dsp_temperature[0], LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                              , 33
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // temp nozzle 2
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Nozzle(2) "));
            int_to_string(target_temperature[1], int_to_string(dsp_temperature[1], LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR(DEGREE_SYMBOL"/")), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string((flags & MENU_ACTIVE) ? target_temperature[1] : dsp_temperature[1], LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+42
                              , 33
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // temp buildplate
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Buildplate "));
            int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR(DEGREE_SYMBOL"/")), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string((flags & MENU_ACTIVE) ? target_temperature_bed : dsp_temperature_bed, LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 33
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
    else if (nr == index++)
    {
        // speed
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Speed "));
            int_to_string(xy_speed+0.5, LCD_CACHE_FILENAME(1)+strlen(LCD_CACHE_FILENAME(1)), PSTR("mm" PER_SECOND_SYMBOL));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            bStatusline = true;
        }
        int_to_string(feedmultiply, LCD_CACHE_FILENAME(0), PSTR("%"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                              , 42
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // fan
        if (flags & MENU_SELECTED)
        {
            strcpy_P(LCD_CACHE_FILENAME(1), PSTR("Fan"));
            lcd_lib_draw_string_left(5, LCD_CACHE_FILENAME(1));
            lcd_lib_draw_bargraph(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING, 5, LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT, 11, float(fanSpeed)/255);
            bStatusline = true;
        }
        int_to_string(float(fanSpeed)*100/255 + 0.5f, LCD_CACHE_FILENAME(0), PSTR("%"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42
                              , 24
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
}

void lcd_menu_print_heatup_tg()
{
    lcd_basic_screen();
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
                menu.reset_submenu();
                doStartPrint();
                menu.replace_menu(menu_t(lcd_menu_printing_tg), false);
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

    int_to_string(progress*100/124, LCD_CACHE_FILENAME(0), PSTR("%"));
    lcd_lib_draw_string_right(15, LCD_CACHE_FILENAME(0));

    lcd_progressline(progress);

    // bed temperature
    uint8_t ypos = 42;
#if TEMP_SENSOR_BED != 0
    // bed temperature
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature_bed, LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, LCD_CACHE_FILENAME(0));
    lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-71, ypos, bedTempGfx);
    // lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(-1));
    ypos -= LCD_LINE_HEIGHT;
#endif // TEMP_SENSOR_BED
#if EXTRUDERS > 1
    // temperature second extruder
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[1], LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, LCD_CACHE_FILENAME(0));
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(1));
    ypos -= LCD_LINE_HEIGHT;
#endif // EXTRUDERS
    // temperature first extruder
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[0], LCD_CACHE_FILENAME(0), PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, LCD_CACHE_FILENAME(0));
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(0));

    menu.process_submenu(get_heatup_menuoption, EXTRUDERS + BED_MENU_OFFSET + 2);

    bool bStatusline(false);
    for (uint8_t index=0; index<EXTRUDERS + BED_MENU_OFFSET + 2; ++index)
    {
        menu.drawSubMenu(drawHeatupSubmenu, index, bStatusline);
    }
    if (!bStatusline)
    {
        lcd_lib_draw_string_left(5, card.longFilename);
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
    if (card.pause)
    {
        lcd_tripple_menu(PSTR("RESUME|PRINT"), PSTR("CHANGE|MATERIAL"), PSTR("TUNE"));
        if (lcd_lib_button_pressed)
        {
            if (IS_SELECTED_MAIN(0) && movesplanned() < 1)
            {
                card.pause = false;
                lcd_lib_beep();
            }else if (IS_SELECTED_MAIN(1) && movesplanned() < 1)
            {
                menu.add_menu(menu_t(lcd_change_to_menu_change_material_return), false);
                menu.add_menu(menu_t(lcd_menu_change_material_preheat));
            }
            else if (IS_SELECTED_MAIN(2))
                menu.add_menu(menu_t(lcd_menu_print_tune));
        }
    }
    else
    {

        static block_t *lastBlock = 0;

        lcd_basic_screen();
        lcd_lib_draw_hline(3, 124, 13);

        uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
        unsigned long timeLeftSec;

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
                    int_to_time_string_tg(timeLeftSec, LCD_CACHE_FILENAME(0));
                    lcd_lib_draw_string(64, 15, LCD_CACHE_FILENAME(0));

                    // draw progress string right aligned
                    int_to_string(progress*100/124, LCD_CACHE_FILENAME(0), PSTR("%"));
                    lcd_lib_draw_string_right(15, LCD_CACHE_FILENAME(0));
                    lcd_progressline(progress);
                }
            }

            break;
        case PRINT_STATE_WAIT_USER:
            lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
            menu.reset_submenu();
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
        }

        // all printing states
        // z position
        lcd_lib_draw_string_leftP(15, PSTR("Z"));

        // float_to_string(current_position[Z_AXIS], LCD_CACHE_FILENAME(0), NULL);
        // calculate current z position
        float_to_string(st_get_position(Z_AXIS) / axis_steps_per_unit[Z_AXIS], LCD_CACHE_FILENAME(0), NULL);
        lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT+12, 15, LCD_CACHE_FILENAME(0));

        // flow
        lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT-1, 24, flowGfx);
        // temperature first extruder
        lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 33, thermometerGfx);

    #if TEMP_SENSOR_BED != 0
        // temperature build-plate
        lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING-12, 33, bedTempGfx);
    #endif

        // speed
        lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 42, speedGfx);
        // fan speed
        static uint8_t fanAnimate = 0;
        static uint8_t prevFanSpeed = 0;

        // start animation
        if (!fanAnimate && fanSpeed!=prevFanSpeed)
            fanAnimate = 32;
        if ((fanSpeed == 0) || (!fanAnimate) || (fanAnimate%4))
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING-11, 42, fan1Gfx);
        }
        if (fanAnimate && !(led_glow%16))
        {
            --fanAnimate;
        }
        prevFanSpeed = fanSpeed;

        uint8_t index = 0;
        if (printing_state == PRINT_STATE_WAIT_USER)
        {
            index += 2;
        }
        else
        {
            menu.process_submenu(get_print_menuoption, EXTRUDERS*2 + BED_MENU_OFFSET + 4);
            if (!card.pause)
            {
                const char *message = lcd_getstatus();
                if (!menu.isSubmenuSelected() && message && *message)
                {
                    lcd_lib_draw_string_left(BOTTOM_MENU_YPOS, message);
                    index += 2;
                }
            }
        }

        bool bStatusline(false);
        for (; index < EXTRUDERS*2 + BED_MENU_OFFSET + 4; ++index) {
            menu.drawSubMenu(drawPrintSubmenu, index, bStatusline);
        }
        if (!bStatusline)
        {
            if (printing_state == PRINT_STATE_HEATING)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Heating"));
            }
            else if (printing_state == PRINT_STATE_HEATING_BED)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Heating buildplate"));
            }
            else
            {
                lcd_lib_draw_string_left(5, card.longFilename);
            }
        }
    }
    lcd_lib_update_screen();
}

static char* lcd_expert_item(uint8_t nr)
{
    if (nr == 0)
    {
        strcpy_P(LCD_CACHE_FILENAME(0), PSTR("< RETURN"));
    }
    else if (nr == 1)
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("User interface"));
    }
    else if (nr == 2)
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("LED timeout"));
    }
    else if (nr == 3)
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("Move axis"));
    }
    else if (nr == 4)
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("Disable steppers"));
    }
    else
    {
        strcpy_P(LCD_CACHE_FILENAME(0), PSTR("???"));
    }

    return LCD_CACHE_FILENAME(0);
}

static char* lcd_uimode_item(uint8_t nr)
{
    if (nr == 0)
    {
        strcpy_P(LCD_CACHE_FILENAME(0), PSTR("< RETURN"));
    }
    else if (nr == 1)
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("Standard Mode"));
    }
    else if (nr == 2)
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("Geek Mode"));
    }
    else
    {
        LCD_CACHE_FILENAME(0)[0] = ' ';
        strcpy_P(LCD_CACHE_FILENAME(0)+1, PSTR("???"));
    }

    if (nr-1 == ui_mode)
        LCD_CACHE_FILENAME(0)[0] = '>';

    return LCD_CACHE_FILENAME(0);
}

static void lcd_expert_details(uint8_t nr)
{
    if (nr == 0)
        return;
    else if(nr == 1)
    {
        if (ui_mode & UI_MODE_TINKERGNOME)
        {
            strcpy_P(LCD_CACHE_FILENAME(0), PSTR("Geek Mode"));
        }
        else
        {
            strcpy_P(LCD_CACHE_FILENAME(0), PSTR("Standard Mode"));
        }
    }
    else if(nr == 2)
    {
        if (led_timeout > 0)
        {
            int_to_string(led_timeout, LCD_CACHE_FILENAME(0), PSTR(" min"));
        }
        else
        {
            strcpy_P(LCD_CACHE_FILENAME(0), PSTR("off"));
        }
    }
    else
    {
        LCD_CACHE_FILENAME(0)[0] = '\0';
    }
    lcd_lib_draw_string_left(BOTTOM_MENU_YPOS, LCD_CACHE_FILENAME(0));
}

static void lcd_uimode_details(uint8_t nr)
{
    // nothing displayed
}

void lcd_menu_uimode()
{
    lcd_scroll_menu(PSTR("User interface"), 3, lcd_uimode_item, lcd_uimode_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            if (ui_mode != UI_MODE_STANDARD)
            {
                ui_mode = UI_MODE_STANDARD;
                SET_UI_MODE(ui_mode);
                menu.return_to_previous();
                return;
            }
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            if (!(ui_mode & UI_MODE_TINKERGNOME))
            {
                ui_mode = UI_MODE_TINKERGNOME;
                SET_UI_MODE(ui_mode);
                menu.return_to_previous();
                return;
            }
        }
        lcd_change_to_previous_menu();
    }
}

static void lcd_menu_led_timeout()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        led_timeout = constrain(int(led_timeout + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM)), 0, LED_DIM_MAXTIME);
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
    {
        SET_LED_TIMEOUT(led_timeout);
        lcd_change_to_previous_menu();
    }

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("LED timeout"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    if (led_timeout > 0)
    {
        int_to_string(int(led_timeout), LCD_CACHE_FILENAME(0), PSTR(" min"));
    }
    else
    {
        strcpy_P(LCD_CACHE_FILENAME(0), PSTR("off"));
    }
    lcd_lib_draw_string_center(30, LCD_CACHE_FILENAME(0));
    lcd_lib_update_screen();
}

void lcd_menu_maintenance_expert()
{
    lcd_scroll_menu(PSTR("Expert settings"), 5, lcd_expert_item, lcd_expert_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            menu.add_menu(menu_t(lcd_menu_uimode));
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            menu.add_menu(menu_t(lcd_menu_led_timeout, ENCODER_NO_SELECTION, 4));
        }
        else if (IS_SELECTED_SCROLL(3))
        {
            menu.add_menu(menu_t(lcd_menu_move_axes, MAIN_MENU_ITEM_POS(0)));
        }
        else if (IS_SELECTED_SCROLL(4))
        {
            // disable steppers
            enquecommand_P(PSTR("M84"));
            lcd_lib_beep();
        }
        else
        {
            menu.return_to_previous();
        }
    }
}

static void init_target_positions()
{
    lcd_lib_encoder_pos = 0;
    movingSpeed = 0;
    delayMove = false;
    for (uint8_t i=0; i<NUM_AXIS; ++i)
    {
        target_position[i] = current_position[i];
    }
}

static bool endstop_reached(AxisEnum axis, int8_t direction)
{
    if (axis == X_AXIS)
    {
        #if defined(X_MIN_PIN) && X_MIN_PIN > -1
        if ((direction < 0) && (READ(X_MIN_PIN) != X_ENDSTOPS_INVERTING))
        {
            return true;
        }
        #endif
        #if defined(X_MAX_PIN) && X_MAX_PIN > -1
        if ((direction > 0) && (READ(X_MAX_PIN) != X_ENDSTOPS_INVERTING))
        {
            return true;
        }
        #endif
    }
    else if (axis == Y_AXIS)
    {
        #if defined(Y_MIN_PIN) && Y_MIN_PIN > -1
        if ((direction < 0) && (READ(Y_MIN_PIN) != Y_ENDSTOPS_INVERTING))
        {
            return true;
        }
        #endif
        // check max endstop
        #if defined(Y_MAX_PIN) && Y_MAX_PIN > -1
        if ((direction > 0) && (READ(Y_MAX_PIN) != Y_ENDSTOPS_INVERTING))
        {
            return true;
        }
        #endif
    }
    else if (axis == Z_AXIS)
    {
        #if defined(Z_MIN_PIN) && Z_MIN_PIN > -1
        if ((direction < 0) && (READ(Z_MIN_PIN) != Z_ENDSTOPS_INVERTING))
        {
            return true;
        }
        #endif
        // check max endstop
        #if defined(Z_MAX_PIN) && Z_MAX_PIN > -1
        if ((direction > 0) && (READ(Z_MAX_PIN) != Z_ENDSTOPS_INVERTING))
        {
            return true;
        }
        #endif
    }
    return false;
}

static void plan_move(AxisEnum axis)
{
    if (!is_command_queued() && !blocks_queued())
    {
        if (bResetTargetPos)
        {
            init_target_positions();
            bResetTargetPos = false;
        }
        else
        {
            // enque next move
            if ((abs(target_position[axis] - current_position[axis])>0.005) && !endstop_reached(axis, (target_position[axis]>current_position[axis]) ? 1 : -1))
            {
                current_position[axis] = target_position[axis];
                plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], homing_feedrate[axis]/800, active_extruder);
            }
        }
    }
}

static void stopMove()
{
    // stop moving
    lcd_lib_encoder_pos = 0;
    movingSpeed = 0;
    delayMove = true;
}

static void lcd_move_axis(AxisEnum axis)
{
    if (endstop_reached(axis, movingSpeed) || lcd_lib_button_pressed || ((lcd_lib_encoder_pos < 0) && (movingSpeed > 0)) || ((lcd_lib_encoder_pos > 0) && (movingSpeed < 0)))
    {
        // stop moving
        stopMove();
    }
    else if (delayMove)
    {
        // prevent too fast consecutively inputs
        lcd_lib_encoder_pos = 0;
        delayMove = (millis() - last_user_interaction) < MOVE_DELAY;
    }
    else
    {
        if (lcd_lib_encoder_pos/ENCODER_TICKS_PER_SCROLL_MENU_ITEM < 0)
        {
            --movingSpeed;
            lcd_lib_encoder_pos = 0;
        }
        else if (lcd_lib_encoder_pos/ENCODER_TICKS_PER_SCROLL_MENU_ITEM > 0)
        {
            ++movingSpeed;
            lcd_lib_encoder_pos = 0;
        }
        movingSpeed = constrain(movingSpeed, -15, 15);
        if (movingSpeed != 0)
        {
            uint8_t steps = BLOCK_BUFFER_SIZE - movesplanned() - 2;
            for (uint8_t i = 0; i < steps; ++i)
            {
                target_position[axis] = current_position[axis]+float(movingSpeed)/(BLOCK_BUFFER_SIZE << 2);
                if ((movingSpeed < 0 && (target_position[axis] < min_pos[axis])) || ((movingSpeed > 0) && (target_position[axis] > max_pos[axis])) || endstop_reached(axis, movingSpeed))
                {
                    stopMove();
                }
                else
                {
                    current_position[axis] = target_position[axis];
                    plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], abs(movingSpeed), active_extruder);
                }
            }
        }
    }
}

static void lcd_move_x_axis()
{
    lcd_move_axis(X_AXIS);
}

static void lcd_move_y_axis()
{
    lcd_move_axis(Y_AXIS);
}

static void lcd_move_z_axis()
{
    lcd_move_axis(Z_AXIS);
}

static void lcd_position_x_axis()
{
    lcd_tune_value(target_position[X_AXIS], min_pos[X_AXIS], max_pos[X_AXIS], 0.05f);
    plan_move(X_AXIS);
}

static void lcd_position_y_axis()
{
    lcd_tune_value(target_position[Y_AXIS], min_pos[Y_AXIS], max_pos[Y_AXIS], 0.05f);
    plan_move(Y_AXIS);
}

static void lcd_position_z_axis()
{
    lcd_tune_value(target_position[Z_AXIS], min_pos[Z_AXIS], max_pos[Z_AXIS], 0.01f);
    plan_move(Z_AXIS);
}

FORCE_INLINE void lcd_home_x_axis() { bResetTargetPos = true; enquecommand_P(PSTR("G28 X0")); }
FORCE_INLINE void lcd_home_y_axis() { bResetTargetPos = true; enquecommand_P(PSTR("G28 Y0")); }
FORCE_INLINE void lcd_home_z_axis() { bResetTargetPos = true; enquecommand_P(PSTR("G28 Z0")); }
FORCE_INLINE void lcd_home_all()    { bResetTargetPos = true; enquecommand_P(PSTR("G28")); }

static void drawMoveDetails()
{
    int_to_string(abs(movingSpeed), LCD_CACHE_FILENAME(1), PSTR("mm" PER_SECOND_SYMBOL));
    lcd_lib_draw_string_center(5, LCD_CACHE_FILENAME(1));
    if (movingSpeed <= 0)
    {
        lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, revSpeedGfx);
    }
    if (movingSpeed >= 0)
    {
        lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6, 5, speedGfx);
    }
}

// create menu options for "move axes"
static const menu_t & get_move_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Home all axis
        opt.setData(MENU_NORMAL, lcd_home_all);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // x move
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_move_x_axis, NULL, 2);
    }
    else if (nr == index++)
    {
        // x position
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_position_x_axis, NULL, 4);
    }
    else if (nr == index++)
    {
        // x home
        opt.setData(MENU_NORMAL, lcd_home_x_axis);
    }
    else if (nr == index++)
    {
        // y move
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_move_y_axis, NULL, 2);
    }
    else if (nr == index++)
    {
        // y position
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_position_y_axis, NULL, 4);
    }
    else if (nr == index++)
    {
        // y home
        opt.setData(MENU_NORMAL, lcd_home_y_axis);
    }
    else if (nr == index++)
    {
        // z move
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_move_z_axis, NULL, 2);
    }
    else if (nr == index++)
    {
        // z position
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_position_z_axis, NULL, 5);
    }
    else if (nr == index++)
    {
        // z home
        opt.setData(MENU_NORMAL, lcd_home_z_axis);
    }
    return opt;
}

static void drawMoveSubmenu(uint8_t nr, uint8_t flags, bool &bStatusline)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Home all axis
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home all axes"));
            bStatusline = true;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+2
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR(" ALL")
                                , ALIGN_CENTER
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+10, 54, homeGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+10, 54, homeGfx);
        }
    }
    else if (nr == index++)
    {
        // RETURN
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Return"));
            bStatusline = true;
        }
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+2
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("RETURN")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // x move
        if (flags & MENU_ACTIVE)
        {
            drawMoveDetails();
            bStatusline = true;
        }
        else if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Move X axis"));
            bStatusline = true;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING
                                , 17
                                , LCD_CHAR_SPACING*3
                                , LCD_CHAR_HEIGHT
                                , PSTR("X")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // x position
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("X axis position"));
            bStatusline = true;
        }
        float_to_string(st_get_position(X_AXIS) / axis_steps_per_unit[X_AXIS], LCD_CACHE_FILENAME(0), PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING
                              , 17
                              , 48
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // x home
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home X axis"));
            bStatusline = true;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING
                           , 17
                           , 10
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING + 1, 17, homeGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING + 1, 17, homeGfx);
        }
    }
    else if (nr == index++)
    {
        // y move
        if (flags & MENU_ACTIVE)
        {
            drawMoveDetails();
            bStatusline = true;
        }
        else if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Move Y axis"));
            bStatusline = true;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING
                                , 29
                                , LCD_CHAR_SPACING*3
                                , LCD_CHAR_HEIGHT
                                , PSTR("Y")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // y position
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Y axis position"));
            bStatusline = true;
        }
        float_to_string(st_get_position(Y_AXIS) / axis_steps_per_unit[Y_AXIS], LCD_CACHE_FILENAME(0), PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING
                              , 29
                              , 48
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // y home
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home Y axis"));
            bStatusline = true;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING
                           , 29
                           , 10
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING + 1, 29, homeGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING + 1, 29, homeGfx);
        }
    }
    else if (nr == index++)
    {
        // z move
        if (flags & MENU_ACTIVE)
        {
            drawMoveDetails();
            bStatusline = true;
        }
        else if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Move Z axis"));
            bStatusline = true;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING
                                , 41
                                , LCD_CHAR_SPACING*3
                                , LCD_CHAR_HEIGHT
                                , PSTR("Z")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // z position
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Z axis position"));
            bStatusline = true;
        }
        float_to_string(st_get_position(Z_AXIS) / axis_steps_per_unit[Z_AXIS], LCD_CACHE_FILENAME(0), PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING
                              , 41
                              , 48
                              , LCD_CHAR_HEIGHT
                              , LCD_CACHE_FILENAME(0)
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // z home
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home Z axis"));
            bStatusline = true;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING
                           , 41
                           , 10
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING + 1, 41, homeGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 3*LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING + 1, 41, homeGfx);
        }
    }
}

void lcd_menu_move_axes()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_move_menuoption, 11);

    bool bStatusline(false);
    for (uint8_t index=0; index<11; ++index) {
        menu.drawSubMenu(drawMoveSubmenu, index, bStatusline);
    }
    if (!bStatusline)
    {
        lcd_lib_draw_string_leftP(5, PSTR("Move axis"));
    }

    lcd_lib_update_screen();
}

void manage_led_timeout()
{
    if (led_timeout > 0)
    {
        static uint8_t ledDimmed = 0;
        if ((millis() - last_user_interaction) > (led_timeout*MILLISECONDS_PER_MINUTE))
        {
            if (!ledDimmed)
            {
                analogWrite(LED_PIN, 255 * min(int(DIM_LEVEL), led_brightness_level) / 100);
                ledDimmed ^= 1;
            }
        }
        else if (ledDimmed)
        {
            analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
            ledDimmed ^= 1;
        }
    }
}

void manage_encoder_position(int8_t encoder_pos_interrupt)
{
// Added encoder acceleration (Lars Jun 2014)
// if we detect we're moving the encoder the same direction for repeated frames, we increase our step size (up to a maximum)
// if we stop, or change direction, set the step size back to +/- 1
// we only want this for SOME things, like changing a value, and not for other things, like a menu.
// so we have an enable bit
    static int8_t accelFactor = 0;
    if (encoder_pos_interrupt != 0)
    {
        if ((ui_mode & UI_MODE_TINKERGNOME) && (menu.encoder_acceleration_factor() > 1))
        {
            if (encoder_pos_interrupt > 0)		// positive -- were we already going positive last time?  If so, increase our accel
            {
                // increase positive acceleration
                if (accelFactor >= 0)
                    ++accelFactor;
                else
                    accelFactor = 1;
                // beep with a pitch changing tone based on the acceleration factor
                // lcd_lib_beep_ext (500+accelFactor*25, 10);
            }
            else //if (lcd_lib_encoder_pos_interrupt <0)
            {
                // decrease negative acceleration
                if (accelFactor <= 0 )
                    --accelFactor;
                else
                    accelFactor = -1;
                // lcd_lib_beep_ext (600+accelFactor*25, 10);
            }
            lcd_lib_tick();
            lcd_lib_encoder_pos += encoder_pos_interrupt * constrain(abs(accelFactor), 1, menu.encoder_acceleration_factor());
        }
        else
        {
            lcd_lib_encoder_pos += encoder_pos_interrupt;
        }
    }
    else
    {
        // no movement -> no acceleration
        accelFactor=0;
    }
}

#endif//ENABLE_ULTILCD2
