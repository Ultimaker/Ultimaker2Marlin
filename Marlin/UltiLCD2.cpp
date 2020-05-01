#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_main.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_maintenance.h"
#include "UltiLCD2_menu_utils.h"
#include "cardreader.h"
#include "ConfigurationStore.h"
#include "temperature.h"
#include "pins.h"
#include "preferences.h"
#include "tinkergnome.h"

// coefficient for the exponential moving average
// K1 defined in Configuration.h in the PID settings
#define K2 (1.0-K1)

#define LCD_CHARS_PER_LINE 20

uint8_t led_brightness_level = 100;
uint8_t led_mode = LED_MODE_ALWAYS_ON;
float dsp_temperature[EXTRUDERS] = { 20.0 };
float dsp_temperature_bed = 20.0;
char lcd_status_message[LCD_CHARS_PER_LINE+1] = {0};

//#define SPECIAL_STARTUP
#define MILLIS_GLOW  (1000L / 40L)
// #define MILLIS_GLOW  (25L)
static unsigned long glow_millis;

static void lcd_menu_startup();
#ifdef SPECIAL_STARTUP
static void lcd_menu_special_startup();
#endif//SPECIAL_STARTUP


void lcd_init()
{
    tinkergnome_init();
    lcd_lib_init();
    if (!lcd_material_verify_material_settings())
    {
        SERIAL_ECHO_START;
        SERIAL_ECHOLNPGM("Invalid material settings found, resetting to defaults");
        lcd_material_reset_defaults();
        for(uint8_t e=0; e<EXTRUDERS; ++e)
        {
            lcd_material_set_material(0, e);
        }
    }
    lcd_material_read_current_material();

    glow_millis = millis() + 750;
    led_glow = led_glow_dir = 0;

#if EXTRUDERS > 1
    active_extruder = (swapExtruders() ? 1 : 0);
#endif // EXTRUDERS

    // initialize menu stack and show start animation
    lcd_clearstatus();
    menu.init_menu(menu_t(lcd_menu_main, MAIN_MENU_ITEM_POS(0)), false);
    menu.add_menu(menu_t(lcd_menu_startup), false);
    analogWrite(LED_PIN, 0);
}

void lcd_update()
{
    unsigned long m = millis();

    //increase/decrease the glow counters
    if (glow_millis < m)
    {
        glow_millis += MILLIS_GLOW;
        if (led_glow_dir)
        {
            led_glow-=2;
            if (led_glow == 0) led_glow_dir = 0;
        }
        else
        {
            led_glow+=2;
            if (led_glow >= 126) led_glow_dir = 1;
        }
    }

    if (!lcd_lib_update_ready()) return;
    lcd_lib_buttons_update();
    card.updateSDInserted();

    if (IsStopped())
    {
		char buffer[24] = {0};
		strcpy_P(buffer, PSTR("ultimaker.com/"));
        lcd_lib_clear();
        lcd_lib_draw_string_centerP(10, PSTR("ERROR - STOPPED"));
        switch(StoppedReason())
        {
        case STOP_REASON_MAXTEMP:
        case STOP_REASON_MINTEMP:
            lcd_lib_draw_string_centerP(20, PSTR("Temp sensor"));
			strcat_P(buffer, PSTR("ER01"));
            break;
        case STOP_REASON_MAXTEMP_BED:
            lcd_lib_draw_string_centerP(20, PSTR("Temp sensor BED"));
			strcat_P(buffer, PSTR("ER02"));
            break;
        case STOP_REASON_HEATER_ERROR:
            lcd_lib_draw_string_centerP(20, PSTR("Heater error"));
			strcat_P(buffer, PSTR("ER03"));
            break;
        case STOP_REASON_SAFETY_TRIGGER:
            lcd_lib_draw_string_centerP(20, PSTR("Safety circuit"));
			strcat_P(buffer, PSTR("ER04"));
            break;
        case STOP_REASON_Z_ENDSTOP_BROKEN_ERROR:
            lcd_lib_draw_string_centerP(20, PSTR("Z switch broken"));
			strcat_P(buffer, PSTR("ER05"));
            break;
        case STOP_REASON_Z_ENDSTOP_STUCK_ERROR:
            lcd_lib_draw_string_centerP(20, PSTR("Z switch stuck"));
			strcat_P(buffer, PSTR("ER06"));
            break;
        case STOP_REASON_XY_ENDSTOP_BROKEN_ERROR:
            lcd_lib_draw_string_centerP(20, PSTR("X or Y switch broken"));
			strcat_P(buffer, PSTR("ER07"));
            break;
        case STOP_REASON_XY_ENDSTOP_STUCK_ERROR:
            lcd_lib_draw_string_centerP(20, PSTR("X or Y switch stuck"));
			strcat_P(buffer, PSTR("ER07"));
            break;
		default:
			strcat_P(buffer, PSTR("support"));
        }
        lcd_lib_draw_string_centerP(40, PSTR("Go to:"));
        lcd_lib_draw_string_center(50, buffer);
        LED_GLOW_ERROR
        lcd_lib_update_screen();
    }
    else
    {
        if (!card.sdprinting())
        {
            if (HAS_SERIAL_CMD)
            {
                if (!(sleep_state & SLEEP_SERIAL_SCREEN))
                {
                    // show usb printing screen during incoming serial communication
                    menu.add_menu(menu_t(lcd_menu_printing_tg, MAIN_MENU_ITEM_POS(1)), false);
                    sleep_state |= SLEEP_SERIAL_SCREEN;
                }
            }
            else if (sleep_state & SLEEP_SERIAL_SCREEN)
            {
                // hide usb printing screen
                sleep_state &= ~SLEEP_SERIAL_SCREEN;
                menu.removeMenu(lcd_menu_printing_tg);
            }
        }
        menu.processEvents();
    }

    if (postMenuCheck && (printing_state != PRINT_STATE_ABORT))
    {
        postMenuCheck();
    }

    // refresh the displayed temperatures
    for(uint8_t e=0; e<EXTRUDERS; ++e)
    {
        dsp_temperature[e] = (K2 * current_temperature[e]) + (K1 * dsp_temperature[e]);
    }
#if TEMP_SENSOR_BED != 0
    dsp_temperature_bed = (K2 * current_temperature_bed) + (K1 * dsp_temperature_bed);
#endif
}

void lcd_menu_startup()
{
    lcd_lib_encoder_pos = ENCODER_NO_SELECTION;

    if (!lcd_lib_update_ready())
        led_glow = 0;

    LED_GLOW
    lcd_lib_clear();

    lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR(STRING_CONFIG_H_AUTHOR));

    if ((led_glow > 0) && (led_glow < 84))
    {
        for(uint8_t n=0;n<10;n++)
        {
            if (led_glow*2 >= n + 20)
                lcd_lib_clear(0, 22+n*2, led_glow*2-n-20, 23+n*2);
            if (led_glow*2 >= n)
                lcd_lib_clear(led_glow*2 - n, 22+n*2, 127, 23+n*2);
            else
                lcd_lib_clear(0, 22+n*2, 127, 23+n*2);
        }
    }
    lcd_lib_update_screen();

    if (led_mode == LED_MODE_ALWAYS_ON)
        analogWrite(LED_PIN, int(led_glow << 1) * led_brightness_level / 100);
    if (led_glow_dir || lcd_lib_button_pressed)
    {
        if (led_mode == LED_MODE_ALWAYS_ON)
            analogWrite(LED_PIN, 255 * led_brightness_level / 100);
        led_glow = led_glow_dir = 0;
        LED_NORMAL

#ifdef SPECIAL_STARTUP
        menu.replace_menu(menu_t(lcd_menu_special_startup), lcd_lib_button_pressed);
#else
        if (IS_FIRST_RUN_DONE())
        {
            menu.return_to_previous(false);
        }
        else
        {
            menu.replace_menu(menu_t(lcd_menu_first_run_init), lcd_lib_button_pressed);
        }
#endif//SPECIAL_STARTUP
    }
}

#ifdef SPECIAL_STARTUP
static void lcd_menu_special_startup()
{
    LED_GLOW

    lcd_lib_clear();
    lcd_lib_draw_gfx(7, 12, specialStartupGfx);
    lcd_lib_draw_stringP(3, 2, PSTR("Welcome"));
    lcd_lib_draw_string_centerP(47, PSTR("To the Ultimaker2"));
    lcd_lib_draw_string_centerP(55, PSTR("experience!"));
    lcd_lib_update_screen();

    if (lcd_lib_button_pressed)
    {
        lcd_remove_menu();
        if (!IS_FIRST_RUN_DONE())
        {
            lcd_add_menu(lcd_menu_first_run_init, ENCODER_NO_SELECTION);
        }
    }
}
#endif//SPECIAL_STARTUP

void doCooldown()
{
    for(uint8_t n=0; n<EXTRUDERS; ++n)
        cooldownHotend(n);
#if TEMP_SENSOR_BED != 0
    setTargetBed(0);
#endif
    fanSpeed = 0;
}

/* Warning: This function is called from interrupt context */
void lcd_buttons_update()
{
    lcd_lib_buttons_update_interrupt();
}

void lcd_setstatus(const char* message)
{
    if (message)
    {
        strncpy(lcd_status_message, message, LCD_CHARS_PER_LINE);
    }
    else
    {
        *lcd_status_message = '\0';
    }
}

void lcd_clearstatus()
{
    *lcd_status_message = '\0';
}

const char * lcd_getstatus()
{
    return lcd_status_message;
}

#endif//ENABLE_ULTILCD2
