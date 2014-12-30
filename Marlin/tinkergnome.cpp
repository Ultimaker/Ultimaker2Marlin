#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include <limits.h>
//#include "Marlin.h"
#include "cardreader.h"
#include "temperature.h"
//#include "lifetime_stats.h"
#include "UltiLCD2_low_lib.h"
#include "UltiLCD2.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_maintenance.h"

#include "tinkergnome.h"

#define LCD_TIMEOUT_TO_STATUS 15000

// UI Mode
#define INFO_MODE_FILNAME 0
#define INFO_MODE_COMMENT 1


#define LCD_DETAIL_CACHE_START ((LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))+1)
// #define LCD_DETAIL_CACHE_ID() lcd_cache[LCD_DETAIL_CACHE_START]
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
#define LCD_DETAIL_CACHE_MATERIAL(n) (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*n])
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])

//#define LED_NORMAL() lcd_lib_led_color(48,48,60)
//#define LED_GLOW() lcd_lib_led_color(8 + led_glow, 8 + led_glow, 32 + led_glow)
//#define LED_GLOW_ERROR() lcd_lib_led_color(8+min(245,led_glow<<3),0,0);

#define LED_FLASH() lcd_lib_led_color(8 + (led_glow<<3), 8 + min(255-8,(led_glow<<3)), 32 + min(255-32,led_glow<<3))
#define LED_HEAT() lcd_lib_led_color(192 + led_glow/4, 8 + led_glow/4, 0)
#define LED_INPUT() lcd_lib_led_color(192 + led_glow/4, 8 + led_glow/4, 0)
#define LED_DONE() lcd_lib_led_color(0, 8 + led_glow, 8)
#define LED_COOL() lcd_lib_led_color(0, 4,16 + led_glow)

static void lcd_print_tune(menuoption_t **pOption);
static void lcd_print_abort(menuoption_t **pOption);
static void lcd_print_tune_speed(menuoption_t **pOption);
static void lcd_print_tune_fan(menuoption_t **pOption);
static void lcd_print_tune_nozzle0(menuoption_t **pOption);
#if EXTRUDERS > 1
static void lcd_print_tune_nozzle1(menuoption_t **pOption);
#endif
#if TEMP_SENSOR_BED != 0
static void lcd_print_tune_bed(menuoption_t **pOption);
#endif

// print menu options
//This code uses the lcd_cache as buffer to store the filename, to save memory.

menuoption_t printOptions[] = {
                                  {(char *)"TUNE", LCD_CHAR_MARGIN_LEFT+2, BOTTOM_MENU_YPOS, 52, LCD_CHAR_HEIGHT, lcd_print_tune, STATE_NONE, ALIGN_CENTER}
                                 ,{(char *)"ABORT", LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+2, BOTTOM_MENU_YPOS, 52, LCD_CHAR_HEIGHT, lcd_print_abort, STATE_NONE, ALIGN_CENTER}
                                 ,{LCD_CACHE_FILENAME(0), LCD_CHAR_MARGIN_LEFT+33, 32, 24, LCD_CHAR_HEIGHT, lcd_print_tune_speed, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
                                 ,{LCD_CACHE_FILENAME(1), LCD_CHAR_MARGIN_LEFT+33, 41, 24, LCD_CHAR_HEIGHT, lcd_print_tune_fan, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
                                 ,{LCD_CACHE_FILENAME(2), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_WIDTH, 50-(EXTRUDERS*9)-(BED_MENU_OFFSET*9), 24, LCD_CHAR_HEIGHT, lcd_print_tune_nozzle0, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
#if EXTRUDERS > 1
                                 ,{LCD_CACHE_FILENAME(3), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_WIDTH, 41-(BED_MENU_OFFSET*9), 24, LCD_CHAR_HEIGHT, lcd_print_tune_nozzle1, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
#endif
#if TEMP_SENSOR_BED != 0
                                 ,{LCD_CACHE_FILENAME(4), LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_WIDTH, 41, 24, LCD_CHAR_HEIGHT, lcd_print_tune_bed, STATE_NONE, ALIGN_RIGHT | ALIGN_VCENTER}
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


void lcd_lib_draw_heater(uint8_t x, uint8_t y, uint8_t heaterPower)
{
    // draw frame
    lcd_lib_draw_hline(x+1, x+3, y);
    lcd_lib_draw_hline(x+1, x+3, y+LCD_CHAR_HEIGHT);
    lcd_lib_draw_vline(x+1, y+1, y+LCD_CHAR_HEIGHT-1);
    lcd_lib_draw_vline(x+3, y+1, y+LCD_CHAR_HEIGHT-1);
    lcd_lib_draw_vline(x,   y+LCD_CHAR_HEIGHT-2, y+LCD_CHAR_HEIGHT-1);
    lcd_lib_draw_vline(x+4, y+LCD_CHAR_HEIGHT-2, y+LCD_CHAR_HEIGHT-1);

    if (heaterPower)
    {
        // draw power beam
        uint8_t beamHeight = min(LCD_CHAR_HEIGHT-2, (heaterPower*(LCD_CHAR_HEIGHT-2)/128)+1);
//        lcd_lib_set(x+2, y+LCD_CHAR_HEIGHT - beamHeight, x+3, y+LCD_CHAR_HEIGHT-1);
        lcd_lib_draw_vline(x+2, y+LCD_CHAR_HEIGHT-beamHeight-1, y+LCD_CHAR_HEIGHT-1);

//        lcd_lib_draw_vline(x+3, y+LCD_CHAR_HEIGHT - max(3, beamHeight-1), y+LCD_CHAR_HEIGHT-1);

        // lcd_lib_set(0, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), 1, LCD_GFX_HEIGHT-1);

    }
}

static void lcd_lib_draw_string_left(uint8_t y, const char* str)
{
    lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT, y, str);
}

static void lcd_lib_draw_string_right(uint8_t x, uint8_t y, const char* str)
{
    lcd_lib_draw_string(x - (strlen(str) * LCD_CHAR_WIDTH), y, str);
}

static void lcd_lib_draw_string_right(uint8_t y, const char* str)
{
    lcd_lib_draw_string_right(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT, y, str);
}

static void lcd_lib_draw_string_leftP(uint8_t y, const char* pstr)
{
    lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, y, pstr);
}

//static void lcd_lib_draw_string_rightP(uint8_t y, const char* pstr)
//{
//    lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (strlen_P(pstr) * LCD_CHAR_WIDTH), y, pstr);
//}

static void lcd_lib_draw_string_rightP(uint8_t x, uint8_t y, const char* pstr)
{
    lcd_lib_draw_stringP(x - (strlen_P(pstr) * LCD_CHAR_WIDTH), y, pstr);
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
        *c++ = 'h';
    }

    *c = '\0';
    return c;
}

static void lcd_progressline(uint8_t progress)
{
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
            textX1 = option.left + option.width - (LCD_CHAR_WIDTH * strlen(buf));
            textX2 = option.left + option.width - (LCD_CHAR_WIDTH * strlen(split));
        }
        else // if (option.textalign & ALIGN_HCENTER)
        {
            textX1 = option.left + option.width/2 - (LCD_CHAR_WIDTH/2 * strlen(buf));
            textX2 = option.left + option.width/2 - (LCD_CHAR_WIDTH/2 * strlen(split));
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
            textX = option.left + option.width - (LCD_CHAR_WIDTH * strlen(option.title));
        else // if (option.textalign & ALIGN_HCENTER)
            textX = option.left + option.width/2 - (LCD_CHAR_WIDTH/2 * strlen(option.title));

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
        temp_value += lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM;
        temp_value = constrain(temp_value, _min, _max);
        value = uint8_t((float(temp_value)*255/_max)+0.5);
        lcd_lib_encoder_pos = 0;
    }
    int_to_string(temp_value, title, p_postfix);
}

static void lcd_tune_temperature(char *title, int &value, int _min, int _max, const char *p_postfix)
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        value = constrain(int(value) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
        lcd_lib_encoder_pos = 0;
    }
    int_to_string(value, title, p_postfix);
}

static void lcd_print_tune(menuoption_t **pOption)
{
    // reset menu option
//    if (pOption && *pOption)
//    {
//        (*pOption)->state = STATE_NONE;
//        *pOption = 0;
//    }
    // switch to tune menu
    lcd_change_to_menu(lcd_menu_print_tune, 0);
}

static void lcd_print_abort(menuoption_t **pOption)
{
    // reset menu option
//    if (pOption && *pOption)
//    {
//        (*pOption)->state = STATE_NONE;
//        *pOption = 0;
//    }
    // switch to abort menu
    lcd_change_to_menu(lcd_menu_print_abort, SCROLL_MENU_ITEM_POS(1));
}

static void lcd_print_tune_speed(menuoption_t **pOption)
{
    if (pOption && *pOption) {
        lcd_tune_value((*pOption)->title, feedmultiply, 0, 999, PSTR("%"));
    }
}

static void lcd_print_tune_fan(menuoption_t **pOption)
{
    if (pOption && *pOption) {
        lcd_tune_byte((*pOption)->title, fanSpeed, 0, 100, PSTR("%"));
    }
}

static void lcd_print_tune_nozzle0(menuoption_t **pOption)
{
    if (pOption && *pOption) {
        lcd_tune_temperature((*pOption)->title, target_temperature[0], 0, HEATER_0_MAXTEMP - 15, PSTR("C"));
    }
}

#if EXTRUDERS > 1
static void lcd_print_tune_nozzle1(menuoption_t **pOption)
{
    if (pOption && *pOption) {
        lcd_tune_temperature((*pOption)->title, target_temperature[1], 0, HEATER_1_MAXTEMP - 15, PSTR("C"));
    }
}
#endif

#if TEMP_SENSOR_BED != 0
static void lcd_print_tune_bed(menuoption_t **pOption)
{
    if (pOption && *pOption) {
        lcd_tune_temperature((*pOption)->title, target_temperature_bed, 0, BED_MAXTEMP - 15, PSTR("C"));
    }
}
#endif

static void lcd_process_menu_options(menuoption_t options[], uint8_t len)
{

    static unsigned long timeout = 0;
    static int16_t lastEncoderPos = 0;
    static int16_t prevEncoderPos = 0;
    static menuoption_t *selectedOption = NULL;

    if ((lcd_lib_encoder_pos != lastEncoderPos) || lcd_lib_button_pressed)
    {
        // reset timeout value
        timeout = millis() + LCD_TIMEOUT_TO_STATUS;
        lastEncoderPos = lcd_lib_encoder_pos;

        // process active item
        if (selectedOption && selectedOption->isActive())
        {
            if (lcd_lib_button_pressed)
            {
                selectedOption->state = STATE_NONE;
                selectedOption = NULL;
                LED_NORMAL();
                lcd_lib_beep();
                lastEncoderPos = lcd_lib_encoder_pos = prevEncoderPos;
            }else if (selectedOption->callbackFunc)
            {
                selectedOption->callbackFunc(&selectedOption);
                lastEncoderPos = lcd_lib_encoder_pos;
            }
        }else if (lcd_lib_encoder_pos != ENCODER_NO_SELECTION)
        {
            // adjust encoder position
            if (lcd_lib_encoder_pos < 0)
                lcd_lib_encoder_pos += len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
            else if (lcd_lib_encoder_pos >= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
                lcd_lib_encoder_pos -= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;

            // detect selection
            if (selectedOption)
                selectedOption->state = STATE_NONE;
            int index = (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM);
            if ((index >= 0) && (index < len))
            {
                selectedOption = &options[index];
                if (lcd_lib_button_pressed)
                {
                    if (index>1)
                    {
                        // "instant tuning"
                        prevEncoderPos = lcd_lib_encoder_pos;
                        lcd_lib_encoder_pos = 0;
                        selectedOption->state = STATE_ACTIVE;
                        LED_INPUT();
                        lcd_lib_beep();
                    }else{
                        // process standard question menu
                        if (selectedOption->callbackFunc)
                        {
                            selectedOption->callbackFunc(&selectedOption);
                        }
                        selectedOption = 0;
                    }
                } else {
                    selectedOption->state = STATE_SELECTED;
                    LED_NORMAL();
                }
            }else{
                selectedOption = 0;
            }
        }
    } else if (timeout < millis())
    {
        // timeout occurred - reset selection
        timeout = ULONG_MAX;
        lastEncoderPos = lcd_lib_encoder_pos = prevEncoderPos;
        if (selectedOption)
        {
            selectedOption->state = STATE_NONE;
            selectedOption = NULL;
            LED_NORMAL();
        }
    }
}

void lcd_menu_print_heatup_tg()
{
    lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));

#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > target_temperature_bed - 10)
    {
#endif
        for(uint8_t e=0; e<EXTRUDERS; e++)
        {
            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] > 0)
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
                doStartPrint();
                lcd_replace_menu(lcd_menu_printing_tg);
            }
        }
#if TEMP_SENSOR_BED != 0
    }
#endif

    uint8_t progress = 125;
    for(uint8_t e=0; e<EXTRUDERS; e++)
    {
        if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] < 1)
            continue;
        if (current_temperature[e] > 20)
            progress = min(progress, (current_temperature[e] - 20) * 125 / (target_temperature[e] - 20 - TEMP_WINDOW));
        else
            progress = 0;
    }
#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > 20)
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

    // temperature extruder 1
    uint8_t ypos = 41;
#if TEMP_SENSOR_BED != 0
    int_to_string(target_temperature_bed, buffer, PSTR("C"));
    lcd_lib_draw_string_right(ypos, buffer);
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature_bed, buffer, PSTR("C"));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(-1));
    ypos -= 9;
#endif // TEMP_SENSOR_BED
#if EXTRUDERS > 1
    int_to_string(target_temperature[1], buffer, PSTR("C"));
    lcd_lib_draw_string_right(ypos, buffer);
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[1], buffer, PSTR("C"));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(1));
    ypos -= 9;
#endif // EXTRUDERS
    int_to_string(target_temperature[0], buffer, PSTR("C"));
    lcd_lib_draw_string_right(ypos, buffer);
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[0], buffer, PSTR("C"));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(0));

    lcd_lib_draw_string_left(5, card.longFilename);
    lcd_lib_draw_hline(3, 124, 13);

//    static int maxPWM = 0;
//
//    int_to_string(getHeaterPower(0), buffer);
//    lcd_lib_draw_string_left(24, buffer);
//
//    maxPWM = max(maxPWM, getHeaterPower(0));
//    int_to_string(maxPWM, buffer);
//    lcd_lib_draw_string(64, 24, buffer);

    lcd_lib_update_screen();
}

void lcd_menu_printing_tg()
{
    lcd_process_menu_options(printOptions, sizeof(printOptions)/sizeof(printOptions[0]));

    lcd_basic_screen();

    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    char buffer[16];
    char* c;
    uint8_t index;

    switch(printing_state)
    {
    default:
        // z position
        lcd_lib_draw_string_leftP(15, PSTR("Z"));
        float_to_string(current_position[Z_AXIS], buffer, NULL);
        lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_WIDTH+LCD_CHAR_WIDTH/2, 15, buffer);

        // speed
        lcd_lib_draw_string_leftP(printOptions[2].top, PSTR("Speed"));
        if (!printOptions[2].isActive())
        {
            int_to_string(feedmultiply, printOptions[2].title, PSTR("%"));
        }

        // fan speed
        lcd_lib_draw_string_leftP(printOptions[3].top, PSTR("Fan"));
        if (!printOptions[3].isActive())
        {
            int_to_string(int(fanSpeed) * 100 / 255, printOptions[3].title, PSTR("%"));
        }

        // temperature first extruder
        if (!printOptions[4].isActive())
        {
            // lcd_lib_draw_heater(printOptions[4].left-8, printOptions[4].top, getHeaterPower(0));
            int_to_string(dsp_temperature[0], printOptions[4].title, PSTR("C"));
        }
#if EXTRUDERS > 1
        // temperature second extruder
        if (!printOptions[5].isActive())
        {
            // lcd_lib_draw_heater(printOptions[5].left-8, printOptions[5].top, getHeaterPower(1));
            int_to_string(dsp_temperature[1], printOptions[5].title, PSTR("C"));
        }
#endif
#if TEMP_SENSOR_BED != 0
        // temperature build-plate
        index = 3+EXTRUDERS+BED_MENU_OFFSET;
        if (!printOptions[index].isActive())
        {
            // lcd_lib_draw_heater(printOptions[index].left-8, printOptions[index].top, getHeaterPower(-1));
            int_to_string(dsp_temperature_bed, printOptions[index].title, PSTR("C"));
        }
#endif

        for (uint8_t index=2; index<sizeof(printOptions)/sizeof(printOptions[0]); ++index) {
            lcd_draw_menu_option(printOptions[index]);
        }

        lcd_lib_draw_string_left(5, card.longFilename);
        lcd_lib_draw_hline(3, 124, 13);
        break;
    case PRINT_STATE_WAIT_USER:
        lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
        lcd_lib_draw_string_centerP(20, PSTR("Press button"));
        lcd_lib_draw_string_centerP(30, PSTR("to continue"));
        break;
    case PRINT_STATE_HEATING:
        lcd_lib_draw_string_centerP(20, PSTR("Heating"));
        c = int_to_string(dsp_temperature[0], buffer, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature[0], c, PSTR("C"));
        lcd_lib_draw_string_center(30, buffer);
        break;
    case PRINT_STATE_HEATING_BED:
        lcd_lib_draw_string_centerP(20, PSTR("Heating buildplate"));
        c = int_to_string(dsp_temperature_bed, buffer, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature_bed, c, PSTR("C"));
        lcd_lib_draw_string_center(30, buffer);
        break;
    }
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
        // lcd_lib_draw_stringP(5, 10, PSTR("Time left unknown"));

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
        unsigned long timeLeftSec;
        if (printTimeSec > totalTimeSec)
            timeLeftSec = 1;
        else
            timeLeftSec = totalTimeSec - printTimeSec;


        // lcd_lib_draw_stringP(5, 10, PSTR("Time left"));

        int_to_time_string_tg(timeLeftSec, buffer);
        lcd_lib_draw_string(58, 15, buffer);

    }

    int_to_string(progress*100/124, buffer, PSTR("%"));
    // draw string right aligned
    lcd_lib_draw_string_right(15, buffer);

    lcd_progressline(progress);

    lcd_draw_menu_option(printOptions[0]);
    lcd_draw_menu_option(printOptions[1]);

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
