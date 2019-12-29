#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include <math.h>
#include "Marlin.h"
#include "cardreader.h"
#include "temperature.h"
#include "lifetime_stats.h"
#include "ConfigurationStore.h"
#include "machinesettings.h"
#include "filament_sensor.h"
#include "preferences.h"
#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2.h"
#include "UltiLCD2_gfx.h"
#include "UltiLCD2_menu_first_run.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_maintenance.h"

#include "tinkergnome.h"

// #define LED_FLASH() lcd_lib_led_color(8 + (led_glow<<3), 8 + min(255-8,(led_glow<<3)), 32 + min(255-32,led_glow<<3))
// #define LED_HEAT() lcd_lib_led_color(192 + led_glow/4, 8 + led_glow/4, 0)
// #define LED_DONE() lcd_lib_led_color(0, 8+led_glow, 8)
// #define LED_COOL() lcd_lib_led_color(0, 4, 16+led_glow)

#define MOVE_DELAY 500  // 500ms

// Use the lcd_cache memory to store manual moving positions
#define TARGET_POS(n)   (cache._float[n])
#define TARGET_MIN(n)   (cache._float[n])
#define TARGET_MAX(n)   (cache._float[3+n])
#define OLD_FEEDRATE    (cache._float[NUM_AXIS])
#define OLD_ACCEL       (cache._float[NUM_AXIS+1])
#define OLD_JERK        (cache._float[NUM_AXIS+2])

uint8_t sleep_state = 0x0;

float recover_height = 0.0f;
float recover_position[NUM_AXIS] = { 0.0f, 0.0f, 0.0f, 0.0f };
int recover_temperature[EXTRUDERS] = { 0 };

// these are used to maintain a simple low-pass filter on the speeds - thanks norpchen
float e_smoothed_speed[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0.0f, 0.0f, 0.0f);
float current_nominal_speed = 0.0f;
// static float target_position[NUM_AXIS];
static int8_t movingSpeed = 0;
static bool delayMove = false;
static uint8_t printing_page = 0;

static void lcd_menu_print_page_inc() { lcd_lib_keyclick(); lcd_basic_screen(); menu.set_selection(0); ++printing_page; }
static void lcd_menu_print_page_dec() { lcd_lib_keyclick(); lcd_basic_screen(); menu.set_selection(1); --printing_page; }

#if defined(BABYSTEPPING)
static void init_babystepping();
static void lcd_menu_babystepping();
#endif // BABYSTEPPING

static void lcd_print_tune_speed();
static void lcd_print_tune_fan();
static void lcd_print_flow_nozzle0();
#if EXTRUDERS > 1
static void lcd_print_flow_nozzle1();
static void lcd_tune_swap_length();
#endif // EXTRUDERS
static void lcd_tune_retract_length();
static void lcd_tune_retract_speed();
static void lcd_print_tune_accel();
static void lcd_print_tune_xyjerk();
static void lcd_position_z_axis();

#define EXPERT_VERSION 7

void tinkergnome_init()
{
    sleep_state = 0x0;

    uint16_t version = GET_EXPERT_VERSION()+1;

    if (version > 7)
    {
        axis_direction = GET_AXIS_DIRECTION();
    }
    else
    {
        // init direction flags
        axis_direction = DEFAULT_AXIS_DIR;
        SET_AXIS_DIRECTION(axis_direction);
    }
    if (version > 6)
    {
#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
        // read buildplate pid coefficients
        float pidBed[3];
        eeprom_read_block(pidBed, (uint8_t*)EEPROM_PID_BED, sizeof(pidBed));
        bedKp = pidBed[0];
        bedKi = pidBed[1];
        bedKd = pidBed[2];
#endif
#if EXTRUDERS > 1
        e2_steps_per_unit = GET_STEPS_E2();
#endif
    }
    else
    {
        // write buildplate pid coefficients
        float pidBed[3];
#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
        pidBed[0] = bedKp;
        pidBed[1] = bedKi;
        pidBed[2] = bedKd;
#else
        pidBed[0] = DEFAULT_bedKp;
        pidBed[1] = (DEFAULT_bedKi*PID_dT);
        pidBed[2] = (DEFAULT_bedKd/PID_dT);
#endif
        eeprom_write_block(pidBed, (uint8_t*)EEPROM_PID_BED, sizeof(pidBed));

        SET_STEPS_E2(axis_steps_per_unit[E_AXIS]);
    }
    if (version > 5)
    {
        // read heater check variables
        heater_check_temp = GET_HEATER_CHECK_TEMP();
        heater_check_time = GET_HEATER_CHECK_TIME();
#if EXTRUDERS > 1
        eeprom_read_block(pid2, (uint8_t*)EEPROM_PID_2, sizeof(pid2));
#endif
#if EXTRUDERS > 1 && defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
        motor_current_e2 = GET_MOTOR_CURRENT_E2();
#endif
    }
    else
    {
        heater_check_temp = MAX_HEATING_TEMPERATURE_INCREASE;
        heater_check_time = MAX_HEATING_CHECK_MILLIS / 1000;

        SET_HEATER_CHECK_TEMP(heater_check_temp);
        SET_HEATER_CHECK_TIME(heater_check_time);
#if EXTRUDERS < 2
        float pid2[3];
#endif // EXTRUDERS
        pid2[0] = Kp;
        pid2[1] = Ki;
        pid2[2] = Kd;
        eeprom_write_block(pid2, (uint8_t*)EEPROM_PID_2, sizeof(pid2));

#if EXTRUDERS > 1 && defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
        motor_current_e2 = motor_current_setting[2];
        SET_MOTOR_CURRENT_E2(motor_current_e2);
#else
        SET_MOTOR_CURRENT_E2(motor_current_setting[2]);
#endif // EXTRUDERS
    }
    if (version > 4)
    {
        // read end of print retraction length from eeprom
        end_of_print_retraction = GET_END_RETRACT();
    }
    else
    {
        end_of_print_retraction = END_OF_PRINT_RETRACTION;
        SET_END_RETRACT(end_of_print_retraction);
    }
    if (version > 3)
    {
        // read axis limits from eeprom
        eeprom_read_block(min_pos, (uint8_t*)EEPROM_AXIS_LIMITS, sizeof(min_pos));
        eeprom_read_block(max_pos, (uint8_t*)(EEPROM_AXIS_LIMITS+sizeof(min_pos)), sizeof(max_pos));
    }
    else
    {
        eeprom_write_block(min_pos, (uint8_t *)EEPROM_AXIS_LIMITS, sizeof(min_pos));
        eeprom_write_block(max_pos, (uint8_t *)(EEPROM_AXIS_LIMITS+sizeof(min_pos)), sizeof(max_pos));
    }
    if (version > 2)
    {
        heater_timeout = GET_HEATER_TIMEOUT();
    }
    else
    {
        heater_timeout = 3;
        SET_HEATER_TIMEOUT(heater_timeout);
    }
    if (version > 1)
    {
        control_flags = GET_CONTROL_FLAGS();
    }
    else
    {
        control_flags = FLAG_PID_NOZZLE;
        SET_CONTROL_FLAGS(control_flags);
    }
    if (version > 0)
    {
        ui_mode = GET_UI_MODE();
        led_sleep_brightness = GET_SLEEP_BRIGHTNESS();
        lcd_sleep_contrast = GET_SLEEP_CONTRAST();
        led_sleep_glow = GET_SLEEP_GLOW();
        led_timeout = constrain(GET_LED_TIMEOUT(), 0, LED_DIM_MAXTIME);
        lcd_timeout = constrain(GET_LCD_TIMEOUT(), 0, LED_DIM_MAXTIME);
        lcd_contrast = GET_LCD_CONTRAST();
        if (lcd_contrast == 0)
            lcd_contrast = 0xDF;
    } else
    {
        ui_mode = UI_MODE_EXPERT;
        led_sleep_brightness = 0;
        lcd_sleep_contrast = 0;
        led_sleep_glow = 0;
        led_timeout = 0;
        lcd_timeout = 0;
        lcd_contrast = 0xDF;
        SET_UI_MODE(ui_mode);
        SET_SLEEP_BRIGHTNESS(led_sleep_brightness);
        SET_SLEEP_CONTRAST(lcd_sleep_contrast);
        SET_SLEEP_GLOW(led_sleep_glow);
        SET_LED_TIMEOUT(led_timeout);
        SET_LCD_TIMEOUT(lcd_timeout);
        SET_LCD_CONTRAST(lcd_contrast);
    }

    if (version < EXPERT_VERSION+1)
    {
        SET_EXPERT_VERSION(EXPERT_VERSION);
    }
}

void menu_printing_init()
{
    printing_page = 0;
}

// return heatup menu option
static const menu_t & get_heatup_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_tune);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_abort);
    }
    else if (nr == menu_index++)
    {
        // temp nozzle 1
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle0);
    }
#if EXTRUDERS > 1
    else if (nr == menu_index++)
    {
        // temp nozzle 2
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle1);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == menu_index++)
    {
        // temp buildplate
        opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_bed);
    }
#endif
    return opt;
}

static void lcd_print_ask_pause()
{
    menu.add_menu(menu_t(lcd_select_first_submenu, lcd_menu_print_pause, NULL));
}

#if defined(BABYSTEPPING)
static void lcd_start_babystepping()
{
    menu.add_menu(menu_t(init_babystepping, lcd_menu_babystepping, NULL));
}
#endif // ENABLED

static void lcd_toggle_led()
{
    // toggle led status
    sleep_state ^= SLEEP_LED_OFF;

    analogWrite(LED_PIN, (sleep_state & SLEEP_LED_OFF) ? 0 : 255 * int(led_brightness_level) / 100);
    LED_NORMAL
    menu.reset_submenu();
}

// return print menu option
static const menu_t & get_print_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (printing_page == 1)
    {
        if (nr == menu_index++)
        {
            opt.setData(MENU_NORMAL, lcd_menu_print_page_dec);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_NORMAL, lcd_print_ask_pause);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_NORMAL, lcd_toggle_led);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_INPLACE_EDIT, lcd_tune_retract_length, 2);
        }
#if EXTRUDERS > 1
        else if (nr == menu_index++)
        {
            opt.setData(MENU_INPLACE_EDIT, lcd_tune_swap_length, 2);
        }
#endif
        else if (nr == menu_index++)
        {
            opt.setData(MENU_INPLACE_EDIT, lcd_tune_retract_speed);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_accel);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_xyjerk);
        }
    }
    else
    {
        if (nr == menu_index++)
        {
            opt.setData(MENU_NORMAL, lcd_toggle_led);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_NORMAL, lcd_print_ask_pause);
        }
        else if (nr == menu_index++)
        {
            opt.setData(MENU_NORMAL, lcd_menu_print_page_inc);
        }
#if defined(BABYSTEPPING)
        else if (nr == menu_index++)
        {
            // babystepping menu
            opt.setData(MENU_NORMAL, lcd_start_babystepping);
        }
#endif
        else if (nr == menu_index++)
        {
            // flow nozzle 1
            opt.setData(MENU_INPLACE_EDIT, lcd_print_flow_nozzle0);
        }
    #if EXTRUDERS > 1
        else if (nr == menu_index++)
        {
            // flow nozzle 2
            opt.setData(MENU_INPLACE_EDIT, lcd_print_flow_nozzle1);
        }
    #endif
        else if (nr == menu_index++)
        {
            // speed
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_speed);
        }
        else if (nr == menu_index++)
        {
            // temp nozzle 1
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle0);
        }
    #if EXTRUDERS > 1
        else if (nr == menu_index++)
        {
            // temp nozzle 2
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_nozzle1);
        }
    #endif
        else if (nr == menu_index++)
        {
            // fan
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_fan);
        }
    #if TEMP_SENSOR_BED != 0
        else if (nr == menu_index++)
        {
            // temp buildplate
            opt.setData(MENU_INPLACE_EDIT, lcd_print_tune_bed);
        }
    #endif
    }
    return opt;
}

static void lcd_print_tune_speed()
{
    lcd_tune_value(feedmultiply, 0, 999);
}

static void lcd_print_tune_fan()
{
    lcd_tune_byte(fanSpeed, 0, 100);
}

static void lcd_print_flow_nozzle0()
{
    if (target_temperature[0]>0) {
        lcd_tune_value(extrudemultiply[0], 0, 999);
    }
}

#if EXTRUDERS > 1
static void lcd_print_flow_nozzle1()
{
    if (target_temperature[1]>0)
    {
        lcd_tune_value(extrudemultiply[1], 0, 999);
    }
}
#endif

static void lcd_tune_retract_length()
{
    lcd_tune_value(retract_length, 0, 50, 0.01);
}

#if EXTRUDERS > 1
static void lcd_tune_swap_length()
{
    lcd_tune_value(extruder_swap_retract_length, 0, 50, 0.01);
}
#endif // EXTRUDERS

static void lcd_tune_retract_speed()
{
    lcd_tune_speed(retract_feedrate, 0, max_feedrate[E_AXIS]*60);
}

static void lcd_print_tune_accel()
{
    lcd_tune_value(acceleration, 0, 20000, 100.0);
}

static void lcd_print_tune_xyjerk()
{
    lcd_tune_value(max_xy_jerk, 0, 100, 1.0);
}

#if defined(BABYSTEPPING)

static void init_babystepping()
{
    cache._float[X_AXIS] = 0.0f;
    cache._float[Y_AXIS] = 0.0f;
    cache._float[Z_AXIS] = 0.0f;
}

static void _lcd_babystep(const uint8_t axis)
{
    int diff = lcd_lib_encoder_pos*axis_steps_per_unit[axis]/200;
    if (diff)
    {
        cache._float[axis] += (float)diff/axis_steps_per_unit[axis];
        babystepsTodo[axis] += diff;
        lcd_lib_encoder_pos = 0;
    }
}

static void lcd_babystep_x() { _lcd_babystep(X_AXIS); }
static void lcd_babystep_y() { _lcd_babystep(Y_AXIS); }
static void lcd_babystep_z() { _lcd_babystep(Z_AXIS); }

static void lcd_store_babystep_z()
{
    lcd_lib_keyclick();
    if (fabs(cache._float[Z_AXIS]) > 0.001)
    {
        add_homing[Z_AXIS] -= cache._float[Z_AXIS];
        Config_StoreSettings();
        cache._float[Z_AXIS] = 0;
    }
}

// create menu options for "move axes"
static const menu_t & get_babystep_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // babystep x
        opt.setData(MENU_INPLACE_EDIT, lcd_babystep_x, (uint8_t)0, 0);
    }
    else if (nr == index++)
    {
        // babystep y
        opt.setData(MENU_INPLACE_EDIT, lcd_babystep_y, (uint8_t)0, 0);
    }
    else if (nr == index++)
    {
        // babystep z
        opt.setData(MENU_INPLACE_EDIT, lcd_babystep_z, (uint8_t)0, 0);
    }
    else if (nr == index++)
    {
        // store z offset
        opt.setData(MENU_NORMAL, lcd_store_babystep_z);
    }
    return opt;
}

static void drawBabystepSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // RETURN
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_MARGIN_LEFT
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // babystep x
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Babystep X axis"));
            flags |= MENU_STATUSLINE;
        }
        if (flags & MENU_ACTIVE)
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH-32-LCD_CHAR_MARGIN_RIGHT, 17, dangerGfx);
        }

        lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT, 17, PSTR("X"));
        float_to_string2(cache._float[X_AXIS], buffer, PSTR("mm"), true);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+3*LCD_CHAR_SPACING
                              , 17
                              , 9*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_VCENTER | ALIGN_RIGHT
                              , flags);

    }
    else if (nr == index++)
    {
        // babystep y
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Babystep Y axis"));
            flags |= MENU_STATUSLINE;
        }
        if (flags & MENU_ACTIVE)
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH-32-LCD_CHAR_MARGIN_RIGHT, 17, dangerGfx);
        }
        lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT, 28, PSTR("Y"));
        float_to_string2(cache._float[Y_AXIS], buffer, PSTR("mm"), true);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+3*LCD_CHAR_SPACING
                              , 28
                              , 9*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_VCENTER | ALIGN_RIGHT
                              , flags);
    }
    else if (nr == index++)
    {
        // babystep z
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Babystep Z axis"));
            flags |= MENU_STATUSLINE;
        }
        if (flags & MENU_ACTIVE)
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH-32-LCD_CHAR_MARGIN_RIGHT, 17, dangerGfx);
        }
        lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT, 39, PSTR("Z"));
        float_to_string2(cache._float[Z_AXIS], buffer, PSTR("mm"), true);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+3*LCD_CHAR_SPACING
                              , 39
                              , 9*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_VCENTER | ALIGN_RIGHT
                              , flags);

    }
    else if (nr == index++)
    {
        // store z offset
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 2*LCD_CHAR_SPACING
               , 39
               , 2*LCD_CHAR_SPACING
               , LCD_CHAR_HEIGHT+1
               , flags);

        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store Z offset"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 2*LCD_CHAR_SPACING + 1, 39, storeGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 2*LCD_CHAR_SPACING + 1, 39, storeGfx);
        }
    }
}

static void lcd_menu_babystepping()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    uint8_t iCount(4);
    // show z store option only if no other submenu is active
    if ((fabs(cache._float[Z_AXIS]) > 0.001) && (!menu.isSubmenuActive() || menu.isSelected(4)))
    {
        ++iCount;
    }

    menu.process_submenu(get_babystep_menuoption, iCount);

    uint8_t flags = 0;
    for (uint8_t index=0; index<iCount; ++index)
    {
        menu.drawSubMenu(drawBabystepSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Babystepping"));
    }

    lcd_lib_update_screen();
}

#endif // BABYSTEPPING

static void drawHeatupSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};

    if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT*2
                           , BOTTOM_MENU_YPOS
                           , 48
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Tune menu"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_clear_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_draw_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
    }
    else if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, standbyGfx);
            lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 5, PSTR("Abort print"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT*2
                                , BOTTOM_MENU_YPOS
                                , 48
                                , LCD_CHAR_HEIGHT
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
    }
    else if (nr == index++)
    {
        // temp nozzle 1
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
#if EXTRUDERS < 2
            strcpy_P(buffer, PSTR("Nozzle "));
            int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer+7, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
#else
            strcpy_P(buffer, PSTR("Nozzle(1) "));
            int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer+10, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
#endif
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        int_to_string(target_temperature[0], buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                          , 51-(EXTRUDERS*LCD_LINE_HEIGHT)-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                          , 24
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // temp nozzle 2
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            strcpy_P(buffer, PSTR("Nozzle(2) "));
            int_to_string(target_temperature[1], int_to_string(dsp_temperature[1], buffer+10, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        int_to_string(target_temperature[1], buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42-(BED_MENU_OFFSET*LCD_LINE_HEIGHT)
                              , 24
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == index++)
    {
        // temp buildplate
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            strcpy_P(buffer, PSTR("Buildplate "));
            int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, buffer+11, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }
        int_to_string(target_temperature_bed, buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 42
                              , 24
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif
}

static void drawPrintSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (printing_page == 1) // second page
    {
        if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Goto page 1"));
                flags |= MENU_STATUSLINE;
            }
            LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+3
                               , BOTTOM_MENU_YPOS
                               , 3*LCD_CHAR_SPACING
                               , LCD_CHAR_HEIGHT
                               , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, previousGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, previousGfx);
            }
        }
        else if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, toolGfx);
                lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 4, PSTR("Control options"));
                flags |= MENU_STATUSLINE;
            }
            LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 - LCD_CHAR_SPACING
                               , BOTTOM_MENU_YPOS
                               , 2*LCD_CHAR_SPACING
                               , LCD_CHAR_HEIGHT
                               , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 - 4, BOTTOM_MENU_YPOS, toolGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 - 4, BOTTOM_MENU_YPOS, toolGfx);
            }
        }
        else if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Toggle LED"));
                flags |= MENU_STATUSLINE;
            }
            LCDMenu::drawMenuBox(LCD_GFX_WIDTH - 2*LCD_CHAR_SPACING - LCD_CHAR_MARGIN_RIGHT
                               , BOTTOM_MENU_YPOS
                               , 2*LCD_CHAR_SPACING
                               , LCD_CHAR_HEIGHT
                               , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING - 5, BOTTOM_MENU_YPOS-1, ledswitchGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - LCD_CHAR_SPACING - 5, BOTTOM_MENU_YPOS-1, ledswitchGfx);
            }
        }
        else if (nr == index++)
        {
            // retract length
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                lcd_lib_draw_string_leftP(5, PSTR("Retract length"));
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_string_leftP(15, PSTR("Retract"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 15, retractLenGfx);
            // lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 15, PSTR("L"));
            float_to_string2(retract_length, buffer, PSTR("mm"));
            LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                                  , 15
                                  , 7*LCD_CHAR_SPACING
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
#if EXTRUDERS > 1
        else if (nr == index++)
        {
            // extruder swap length
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                lcd_lib_draw_string_leftP(5, PSTR("Extruder change"));
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_string_leftP(24, PSTR("E"));
            float_to_string2(extruder_swap_retract_length, buffer, PSTR("mm"));
            LCDMenu::drawMenuString(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING
                                  , 24
                                  , 7*LCD_CHAR_SPACING
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
#endif
        else if (nr == index++)
        {
            // retract speed
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                lcd_lib_draw_string_leftP(5, PSTR("Retract speed"));
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 24, retractSpeedGfx);
            // lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 24, PSTR("S"));
            int_to_string(retract_feedrate / 60 + 0.5, buffer, PSTR("mm/s"));
            LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                                  , 24
                                  , 7*LCD_CHAR_SPACING
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
        else if (nr == index++)
        {
            // acceleration
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                lcd_lib_draw_string_leftP(5, PSTR("Acceleration"));
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_string_leftP(33, PSTR("Accel"));
            int_to_string(acceleration, buffer, PSTR(UNIT_ACCELERATION));
            LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-10*LCD_CHAR_SPACING // 2*LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING //
                                  , 33
                                  , 10*LCD_CHAR_SPACING
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
        else if (nr == index++)
        {
            // jerk
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                lcd_lib_draw_string_leftP(5, PSTR("Max. X/Y Jerk"));
                flags |= MENU_STATUSLINE;
            }

            lcd_lib_draw_string_leftP(42, PSTR("X/Y Jerk"));
            int_to_string(max_xy_jerk, buffer, PSTR("mm/s"));
            LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                                  , 42
                                  , 7*LCD_CHAR_SPACING
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
    }
    else // first page
    {
        if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Toggle LED"));
                flags |= MENU_STATUSLINE;
            }
            LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+2
                               , BOTTOM_MENU_YPOS
                               , 2*LCD_CHAR_SPACING
                               , LCD_CHAR_HEIGHT
                               , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT + 4, BOTTOM_MENU_YPOS-1, ledswitchGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT + 4, BOTTOM_MENU_YPOS-1, ledswitchGfx);
            }
        }
        else if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, toolGfx);
                lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 4, PSTR("Control options"));
                flags |= MENU_STATUSLINE;
            }
            LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 - LCD_CHAR_SPACING
                               , BOTTOM_MENU_YPOS
                               , 2*LCD_CHAR_SPACING
                               , LCD_CHAR_HEIGHT
                               , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 - 4, BOTTOM_MENU_YPOS, toolGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 - 4, BOTTOM_MENU_YPOS, toolGfx);
            }
        }
        else if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Goto page 2"));
                flags |= MENU_STATUSLINE;
            }
            LCDMenu::drawMenuBox(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 3*LCD_CHAR_SPACING
                                    , BOTTOM_MENU_YPOS
                                    , 3*LCD_CHAR_SPACING-1
                                    , LCD_CHAR_HEIGHT
                                    , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, nextGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, nextGfx);
            }
        }
#if defined(BABYSTEPPING)
        else if (nr == index++)
        {
            if (flags & MENU_SELECTED)
            {
                lcd_lib_draw_string_leftP(5, PSTR("Babystepping"));
                flags |= MENU_STATUSLINE;
            }

            lcd_lib_draw_string_leftP(15, PSTR("Z"));

            // calculate current z position
            float_to_string2(st_get_position(Z_AXIS) / axis_steps_per_unit[Z_AXIS], buffer, 0);
            LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                                    , 15
                                    , LCD_CHAR_SPACING*strlen(buffer)
                                    , LCD_CHAR_HEIGHT
                                    , buffer
                                    , ALIGN_LEFT | ALIGN_VCENTER
                                    , flags);
        }
#endif
        else if (nr == index++)
        {
            // flow nozzle 1
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
    #if EXTRUDERS < 2
                strcpy_P(buffer, PSTR("Flow "));
                float_to_string1(e_smoothed_speed[0], buffer+5, PSTR(UNIT_FLOW));
    #else
                strcpy_P(buffer, PSTR("Flow(1) "));
                float_to_string1(e_smoothed_speed[0], buffer+8, PSTR(UNIT_FLOW));
    #endif
                lcd_lib_draw_string_left(5, buffer);
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT-1, 24, flowGfx);
            int_to_string(extrudemultiply[0], buffer, PSTR("%"));
            LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                                  , 24
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
    #if EXTRUDERS > 1
        else if (nr == index++)
        {
            // flow nozzle 2
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                strcpy_P(buffer, PSTR("Flow(2) "));
                float_to_string1(e_smoothed_speed[1], buffer+8, PSTR(UNIT_FLOW));
                lcd_lib_draw_string_left(5, buffer);
                flags |= MENU_STATUSLINE;
            }
            int_to_string(extrudemultiply[1], buffer, PSTR("%"));
            LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+42
                                  , 24
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
    #endif
        else if (nr == index++)
        {
            // speed
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                strcpy_P(buffer, PSTR("Speed "));
                int_to_string(current_nominal_speed+0.5, buffer+6, PSTR(UNIT_SPEED));
                lcd_lib_draw_string_left(5, buffer);
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 33, speedGfx);
            int_to_string(feedmultiply, buffer, PSTR("%"));
            LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                                  , 33
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
        else if (nr == index++)
        {
            // temp nozzle 1
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
    #if EXTRUDERS < 2
                strcpy_P(buffer, PSTR("Nozzle "));
                int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer+7, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
    #else
                strcpy_P(buffer, PSTR("Nozzle(1) "));
                int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer+10, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
    #endif
                lcd_lib_draw_string_left(5, buffer);
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 42, thermometerGfx);
            int_to_string((flags & MENU_ACTIVE) ? target_temperature[0] : dsp_temperature[0], buffer, PSTR(DEGREE_SYMBOL));
            LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+12
                                  , 42
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
    #if EXTRUDERS > 1
        else if (nr == index++)
        {
            // temp nozzle 2
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                strcpy_P(buffer, PSTR("Nozzle(2) "));
                int_to_string(target_temperature[1], int_to_string(dsp_temperature[1], buffer+10, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
                lcd_lib_draw_string_left(5, buffer);
                flags |= MENU_STATUSLINE;
            }
            int_to_string((flags & MENU_ACTIVE) ? target_temperature[1] : dsp_temperature[1], buffer, PSTR(DEGREE_SYMBOL));
            LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+42
                                  , 42
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
    #endif
        else if (nr == index++)
        {
            // fan
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                lcd_lib_draw_string_leftP(5, PSTR("Fan"));
                lcd_lib_draw_bargraph(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING, 5, LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT, 11, float(fanSpeed)/255);
                flags |= MENU_STATUSLINE;
            }
            int_to_string(float(fanSpeed)*100/255 + 0.5f, buffer, PSTR("%"));
            LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
    #if TEMP_SENSOR_BED != 0
                                  , 33
    #else
                                  , 42
    #endif
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);

            // fan speed
            static uint8_t fanAnimate = 0;
            static uint8_t prevFanSpeed = 0;

            // start animation
            if (!fanAnimate && fanSpeed!=prevFanSpeed)
                fanAnimate = 32;
            if ((fanSpeed == 0) || (!fanAnimate) || (fanAnimate%4))
            {
    #if TEMP_SENSOR_BED != 0
                lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING-11, 33, fan1Gfx);
    #else
                lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING-11, 42, fan1Gfx);
    #endif
            }
            if (fanAnimate && !(led_glow%16))
            {
                --fanAnimate;
            }
            prevFanSpeed = fanSpeed;

        }
    #if TEMP_SENSOR_BED != 0
        else if (nr == index++)
        {
            // temp buildplate
            if (flags & (MENU_SELECTED | MENU_ACTIVE))
            {
                strcpy_P(buffer, PSTR("Buildplate "));
                int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, buffer+11, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
                lcd_lib_draw_string_left(5, buffer);
                flags |= MENU_STATUSLINE;
            }
            lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING-12, 42, bedTempGfx);
            int_to_string((flags & MENU_ACTIVE) ? target_temperature_bed : dsp_temperature_bed, buffer, PSTR(DEGREE_SYMBOL));
            LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                                  , 42
                                  , 24
                                  , LCD_CHAR_HEIGHT
                                  , buffer
                                  , ALIGN_RIGHT | ALIGN_VCENTER
                                  , flags);
        }
    #endif
    }
}

void lcd_menu_print_heatup_tg()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    char buffer[32] = {0};
#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed >= degTargetBed() - TEMP_WINDOW * 2)
    {
#endif
        for(uint8_t e=0; e<EXTRUDERS; ++e)
        {
#if EXTRUDERS == 2
            uint8_t index = (swapExtruders() ? e^0x01 : e);
            if (LCD_DETAIL_CACHE_MATERIAL(index) < 1 || target_temperature[e] > 0)
                continue;
#else
            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] > 0)
                continue;
#endif
            target_temperature[e] = material[e].temperature[nozzleSizeToTemperatureIndex(LCD_DETAIL_CACHE_NOZZLE_DIAMETER(e))];
        }

#if TEMP_SENSOR_BED != 0
        if (current_temperature_bed >= degTargetBed() - TEMP_WINDOW * 2 && !commands_queued() && !blocks_queued())
#else
        if (!commands_queued() && !blocks_queued())
#endif // TEMP_SENSOR_BED
        {
            bool ready = true;
            for(uint8_t e=0; (e<EXTRUDERS) && ready; ++e)
            {
                if (current_temperature[e] < degTargetHotend(e) - TEMP_WINDOW)
                {
                    ready = false;
                }
            }

            if (ready)
            {
                menu.reset_submenu();
                doStartPrint();
                printing_page = 0;
                menu.replace_menu(menu_t(lcd_menu_printing_tg, MAIN_MENU_ITEM_POS(1)), false);
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
            progress = min(progress, (current_temperature[e] - 20) * 125 / (degTargetHotend(e) - 20 - TEMP_WINDOW));
        else
            progress = 0;
    }
#if TEMP_SENSOR_BED != 0
    if ((current_temperature_bed > 20) && (degTargetBed() > 20+TEMP_WINDOW))
        progress = min(progress, (current_temperature_bed - 20) * 125 / (degTargetBed() - 20 - TEMP_WINDOW));
    else if (degTargetBed() > current_temperature_bed - 20)
        progress = 0;
#endif

    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;

    lcd_lib_draw_string_leftP(15, PSTR("Heating up... "));

    int_to_string(progress*100/124, buffer, PSTR("%"));
    lcd_lib_draw_string_right(15, buffer);

    lcd_progressline(progress);

    // bed temperature
    uint8_t ypos = 42;
#if TEMP_SENSOR_BED != 0
    // bed temperature
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature_bed, buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-71, ypos, bedTempGfx);
    // lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(-1));
    ypos -= LCD_LINE_HEIGHT;
#endif // TEMP_SENSOR_BED
#if EXTRUDERS > 1
    // temperature second extruder
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[1], buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(1));
    ypos -= LCD_LINE_HEIGHT;
#endif // EXTRUDERS
    // temperature first extruder
    lcd_lib_draw_string_rightP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-26, ypos, PSTR("/"));
    int_to_string(dsp_temperature[0], buffer, PSTR(DEGREE_SYMBOL));
    lcd_lib_draw_string_right(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-34, ypos, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-70, ypos, getHeaterPower(0));

    menu.process_submenu(get_heatup_menuoption, EXTRUDERS + BED_MENU_OFFSET + 2);

    uint8_t flags = 0;
    for (uint8_t index=0; index<EXTRUDERS + BED_MENU_OFFSET + 2; ++index)
    {
        menu.drawSubMenu(drawHeatupSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_left(5, card.currentLongFileName());
    }

    lcd_lib_update_screen();
}

static unsigned long predictTimeLeft()
{
    if ((printing_state == PRINT_STATE_HEATING) || (printing_state == PRINT_STATE_HEATING_BED) || (!card.getFilePos()) || (!card.getFileSize()))
    {
        return 0;
    }

    unsigned long printTime;
    {
        unsigned long m(millis());
        if (starttime >= m)
        {
            return 0;
        }
        printTime = m/1000L - starttime/1000L;
    }

    float progress = float(card.getFilePos()) / float(card.getFileSize());

    if ((printTime < 60) || (progress < 0.01f))
    {
        return LCD_DETAIL_CACHE_TIME;
    }
    else if ((LCD_DETAIL_CACHE_TIME == 0) && (printTime < 600) && (progress < 0.5f))
    {
        return 0;
    }

    unsigned long totalTime = constrain(float(printTime) / progress, 0, 1000L * 60L * 60L);
    if (predictedTime > 0)
    {
        // low pass filter
        predictedTime = (predictedTime * 999L + totalTime) / 1000L;
    }
    else
    {
        predictedTime = totalTime;
    }

    if (LCD_DETAIL_CACHE_TIME && (printTime < LCD_DETAIL_CACHE_TIME / 2))
    {
        float f = float(printTime) / float(LCD_DETAIL_CACHE_TIME / 2);
        if (f > 1.0)
            f = 1.0;
        totalTime = float(predictedTime) * f + float(LCD_DETAIL_CACHE_TIME) * (1 - f);
    }
    else
    {
        totalTime = predictedTime;
    }

    return (printTime >= totalTime) ? 0 : totalTime - printTime;
}

void lcd_menu_printing_tg()
{
    if (card.pause())
    {
        menu.add_menu(menu_t(lcd_select_first_submenu, lcd_menu_print_resume, NULL, MAIN_MENU_ITEM_POS(0)), true);
        if (!checkFilamentSensor())
        {
            menu.add_menu(menu_t(lcd_menu_filament_outage, MAIN_MENU_ITEM_POS(0)));
        }
    }
    else
    {
        lcd_basic_screen();
        lcd_lib_draw_hline(3, 124, 13);

        // calculate speeds
        if (current_block!=NULL)
        {

            if ((current_block->steps_e > 0) && (current_block->step_event_count > 0) && (current_block->steps_x || current_block->steps_y))
            {
    //                float block_time = current_block->millimeters / current_block->nominal_speed;
    //                float mm_e = current_block->steps_e / axis_steps_per_unit[E_AXIS];

                // calculate live extrusion rate from e speed and filament area
                float speed_e = current_block->steps_e * current_block->nominal_rate / e_steps_per_unit(current_block->active_extruder) / current_block->step_event_count;
                float volume = (volume_to_filament_length[current_block->active_extruder] < 0.99) ? speed_e / volume_to_filament_length[current_block->active_extruder] : speed_e*DEFAULT_FILAMENT_AREA;

                e_smoothed_speed[current_block->active_extruder] = (e_smoothed_speed[current_block->active_extruder]*LOW_PASS_SMOOTHING) + ( volume *(1.0-LOW_PASS_SMOOTHING));
                current_nominal_speed = current_block->nominal_speed;
            }
        }

#if EXTRUDERS > 1
        char buffer[32] = {0};
#endif // EXTRUDERS
        if (printing_page == 0)
        {
#ifdef __AVR__
            uint8_t progress(0);
            if IS_SD_PRINTING
            {
                uint32_t divisor = (card.getFileSize() + 123) / 124;
                if (divisor)
                {
                    progress = card.getFilePos() / divisor;
                }
            }
#else
            uint8_t progress = 0;
#endif
#if EXTRUDERS < 2
            char buffer[32] = {0};
#endif // EXTRUDERS
            switch(printing_state)
            {
            default:

                if (card.pause())
                {
                    lcd_lib_draw_gfx(54, 15, hourglassGfx);
                    lcd_lib_draw_stringP(64, 15, (movesplanned() < 1) ? PSTR("Paused...") : PSTR("Pausing..."));
                }
                else if (IS_SD_PRINTING)
                {
                    if (progress)
                    {
                        // time left
                        unsigned long timeLeftSec = predictTimeLeft();
                        if (timeLeftSec > 0)
                        {
                            lcd_lib_draw_gfx(54, 15, clockInverseGfx);
                            int_to_time_min(timeLeftSec, buffer);
                            lcd_lib_draw_string(64, 15, buffer);
                        }
                        // draw progress string right aligned
                        int_to_string(progress*100/124, buffer, PSTR("%"));
                        lcd_lib_draw_string_right(15, buffer);
                        lcd_progressline(progress);
                    }
                }

                break;
            case PRINT_STATE_WAIT_USER:
                lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
                menu.reset_submenu();
                // lcd_lib_draw_string_left(5, PSTR("Paused..."));
                lcd_lib_draw_string_left(5, card.currentLongFileName());
                lcd_lib_draw_gfx(54, 15, hourglassGfx);
                if (!blocks_queued())
                {
                    lcd_lib_draw_stringP(64, 15, PSTR("Paused..."));
                    lcd_lib_draw_string_leftP(BOTTOM_MENU_YPOS, PSTR("Click to continue..."));
                }
                else
                {
                    lcd_lib_draw_stringP(64, 15, PSTR("Pausing..."));
                }
//                if (!led_glow)
//                {
//                    lcd_lib_tick();
//                }
                break;
            }

#ifndef BABYSTEPPING
            // z position
            lcd_lib_draw_string_leftP(15, PSTR("Z"));

            // calculate current z position
            float_to_string2(st_get_position(Z_AXIS) / axis_steps_per_unit[Z_AXIS], buffer, 0);
            lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT+12, 15, buffer);
#endif
        }

        uint8_t index = 0;
#ifdef BABYSTEPPING
        uint8_t len = (printing_page == 1) ? 6 + min(EXTRUDERS, 2) : EXTRUDERS*2 + BED_MENU_OFFSET + 6;
#else
        uint8_t len = (printing_page == 1) ? 6 + min(EXTRUDERS, 2) : EXTRUDERS*2 + BED_MENU_OFFSET + 5;
#endif // BABYSTEPPING

        menu.process_submenu(get_print_menuoption, len);
        const char *message = lcd_getstatus();
        if (!menu.isSubmenuSelected())
        {
            if (message && *message)
            {
                lcd_lib_draw_string_left(BOTTOM_MENU_YPOS, message);
                index += 3;
            }
            else if (printing_state == PRINT_STATE_HEATING)
            {
                lcd_lib_draw_string_leftP(BOTTOM_MENU_YPOS, PSTR("Heating nozzle"));
#if EXTRUDERS > 1
                int_to_string(tmp_extruder+1, buffer, NULL);
                lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT + 15*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, buffer);
#endif // EXTRUDERS
                index += 3;
            }
            else if (printing_state == PRINT_STATE_HEATING_BED)
            {
                lcd_lib_draw_string_leftP(BOTTOM_MENU_YPOS, PSTR("Heating buildplate"));
                index += 3;
            }
        }

        uint8_t flags = 0;
        for (; index < len; ++index) {
            menu.drawSubMenu(drawPrintSubmenu, index, flags);
        }
        if (!(flags & MENU_STATUSLINE))
        {
            if (HAS_SERIAL_CMD)
            {
                lcd_lib_draw_string_leftP(5, PSTR("USB communication..."));
            }
            else if (card.isFileOpen())
            {
                lcd_lib_draw_string_left(5, card.currentLongFileName());
            }
        }
        lcd_lib_update_screen();
    }
}

static void lcd_expert_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[LINE_ENTRY_TEXT_LENGTH] = {' '};
    if (nr == 0)
    {
        strcpy_P(buffer, PSTR("< RETURN"));
    }
    else if (nr == 1)
    {
        strcpy_P(buffer+1, PSTR("Move axis"));
    }
    else if (nr == 2)
    {
        strcpy_P(buffer+1, PSTR("Adjust Z position"));
    }
    else if (nr == 3)
    {
        strcpy_P(buffer+1, PSTR("Recover print"));
    }
    else if (nr == 4)
    {
        strcpy_P(buffer+1, PSTR("Disable steppers"));
    }
    else
    {
        strcpy_P(buffer+1, PSTR("???"));
    }

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void init_target_positions()
{
    lcd_lib_encoder_pos = 0;
    movingSpeed = 0;
    delayMove = false;
    for (uint8_t i=0; i<NUM_AXIS; ++i)
    {
        TARGET_POS(i) = current_position[i] = st_get_position(i)/axis_steps_per_unit[i];
    }
}

void lcd_simple_buildplate_cancel()
{
    // reload settings
    Config_RetrieveSettings();
    menu.return_to_previous();
}

void lcd_simple_buildplate_store()
{
    add_homing[Z_AXIS] -= current_position[Z_AXIS];
    current_position[Z_AXIS] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
    Config_StoreSettings();
    menu.return_to_previous();
}

void lcd_simple_buildplate_quit()
{
    // home z-axis
    homeBed();
    // home head
    homeHead();
    enquecommand_P(PSTR("M84 X0 Y0"));
}

// create menu options for "move axes"
static const menu_t & get_simple_buildplate_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Store new homeing offset
        opt.setData(MENU_NORMAL, lcd_simple_buildplate_store);
    }
    else if (nr == index++)
    {
        // Cancel
        opt.setData(MENU_NORMAL, lcd_simple_buildplate_cancel);
    }
    else if (nr == index++)
    {
        // z position
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_position_z_axis, NULL, 3);
    }
    return opt;
}

static void drawSimpleBuildplateSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Store
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Cancel
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_MARGIN_LEFT
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , PSTR("CANCEL")
                           , ALIGN_CENTER
                           , flags);
    }
    else if (nr == index++)
    {
        // z position
        char buffer[32] = {0};
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING, 40, PSTR("Z"));
        float_to_string2(st_get_position(Z_AXIS) / axis_steps_per_unit[Z_AXIS], buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+7*LCD_CHAR_SPACING
                              , 40
                              , 48
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
}

void lcd_menu_simple_buildplate()
{
    lcd_basic_screen();
    // lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_simple_buildplate_menuoption, 3);

    uint8_t flags = 0;
    for (uint8_t index=0; index<3; ++index) {
        menu.drawSubMenu(drawSimpleBuildplateSubmenu, index, flags);
    }

    lcd_lib_draw_string_centerP(8, PSTR("Move Z until the"));
    lcd_lib_draw_string_centerP(17, PSTR("nozzle touches"));
    lcd_lib_draw_string_centerP(26, PSTR("the buildplate"));

    lcd_lib_update_screen();
}

void lcd_prepare_buildplate_adjust()
{
    Config_RetrieveSettings();
    add_homing[Z_AXIS] = 0;
    enquecommand_P(PSTR("G28 Z0 X0 Y0"));
    char buffer[32] = {0};
    sprintf_P(buffer, PSTR("G1 F%i Z%i X%i Y%i"), int(homing_feedrate[0]), 35, AXIS_CENTER_POS(X_AXIS), AXIS_CENTER_POS(Y_AXIS));
    enquecommand(buffer);
    enquecommand_P(PSTR("M84 X0 Y0"));
}

void lcd_simple_buildplate_init()
{
    menu.set_active(get_simple_buildplate_menuoption, 2);
}

void lcd_menu_simple_buildplate_init()
{
    lcd_lib_clear();

    float zPos = st_get_position(Z_AXIS) / axis_steps_per_unit[Z_AXIS];
    if ((commands_queued() < 1) && (zPos < 35.01f))
    {
        menu.replace_menu(menu_t(lcd_simple_buildplate_init, lcd_menu_simple_buildplate, lcd_simple_buildplate_quit, 0), false);
    }

    lcd_lib_draw_string_centerP(8, PSTR("Move Z until the"));
    lcd_lib_draw_string_centerP(17, PSTR("nozzle touches"));
    lcd_lib_draw_string_centerP(26, PSTR("the buildplate"));

    char buffer[32] = {0};
    lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING, 40, PSTR("Z"));
    float_to_string2(zPos, buffer, PSTR("mm"));
    LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+7*LCD_CHAR_SPACING
                          , 40
                          , 48
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , 0);

    lcd_lib_update_screen();
}

static void drawRecoverFileSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);

    if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT*2
                           , BOTTOM_MENU_YPOS
                           , 48
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Tune menu"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_clear_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_draw_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
    }
    else if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, standbyGfx);
            lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING, 5, PSTR("Abort print"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT*2
                                , BOTTOM_MENU_YPOS
                                , 48
                                , LCD_CHAR_HEIGHT
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
    }
}

static void recover_abort()
{
    quickStop();
    clear_command_queue();

    for(uint8_t n=0; n<EXTRUDERS; ++n)
        cooldownHotend(n);
    fanSpeed = 0;
    reset_printing_state();
    doCooldown();

    menu.return_to_main();
}

static const menu_t & get_recoverfile_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_tune);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, recover_abort);
    }
    return opt;
}

static void lcd_menu_recover_file()
{
    char buffer[32] = {0};
    LED_GLOW
    // analogWrite(LED_PIN, (led_glow << 1) * int(led_brightness_level) / 100);

    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_recoverfile_menuoption, 2);

    lcd_lib_draw_string_centerP(20, PSTR("Reading file..."));
    lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+2*LCD_CHAR_SPACING, 35, PSTR("Z"));
    char *c = float_to_string2(current_position[Z_AXIS], buffer, PSTR(" / "));
    c = float_to_string2(recover_height, c, NULL);
    lcd_lib_draw_string_center(35, buffer);

    uint8_t flags = 0;
    for (uint8_t index=0; index<2; ++index)
    {
        menu.drawSubMenu(drawRecoverFileSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_left(5, card.currentLongFileName());
    }

    lcd_lib_update_screen();
}


static void lcd_recover_start()
{
    #if EXTRUDERS > 1
    active_extruder = (swapExtruders() ? 1 : 0);
    #else
    active_extruder = 0;
    #endif // EXTRUDERS
    current_position[E_AXIS] = 0.0f;
    plan_set_e_position(current_position[E_AXIS], active_extruder, true);
    menu.replace_menu(menu_t(lcd_menu_recover_file));
    card.startFileprint();
}

static void lcd_recover_zvalue()
{
    lcd_tune_value(recover_height, min_pos[Z_AXIS], max_pos[Z_AXIS], 0.01f);
}

void reset_printing_state()
{
    printing_state = PRINT_STATE_NORMAL;
    card.stopPrinting();
}

static const menu_t & get_recover_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_recover_start);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, reset_printing_state, lcd_change_to_previous_menu, NULL);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_recover_zvalue, 4);
    }
    return opt;
}

static void drawRecoverSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Start print"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 8*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("PRINT")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2+LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 8*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("CANCEL")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // recover z position
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Recover height"));
            flags |= MENU_STATUSLINE;
        }

        lcd_lib_draw_string_centerP(20, PSTR("Recover print from"));
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+4*LCD_CHAR_SPACING, 35, PSTR("Z"));
        float_to_string2(recover_height, buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+6*LCD_CHAR_SPACING
                          , 35
                          , 48
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
}

void lcd_menu_recover_init()
{
    menu.set_active(get_recover_menuoption, 2);
}

void lcd_menu_expert_recover()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_recover_menuoption, 3);

    uint8_t flags = 0;
    for (uint8_t index=0; index<3; ++index)
    {
        menu.drawSubMenu(drawRecoverSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Recover print"));
    }

    lcd_lib_update_screen();

}

void lcd_menu_maintenance_expert()
{
    lcd_scroll_menu(PSTR("Expert functions"), 5, lcd_expert_item, NULL);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(1))
        {
            menu.add_menu(menu_t(lcd_menu_move_axes, MAIN_MENU_ITEM_POS(0)));
        }
        else if (IS_SELECTED_SCROLL(2))
        {
            menu.add_menu(menu_t(lcd_prepare_buildplate_adjust, lcd_menu_simple_buildplate_init, NULL, ENCODER_NO_SELECTION));
        }
        else if (IS_SELECTED_SCROLL(3))
        {
            for (uint8_t i=0; i<EXTRUDERS; ++i)
            {
                recover_temperature[i] = target_temperature[i];
            }
            printing_state = PRINT_STATE_RECOVER;
            // select file
            lcd_clear_cache();
            card.release();
            card.setroot();
            menu.add_menu(menu_t(lcd_menu_print_select, SCROLL_MENU_ITEM_POS(0)));
        }
        else if (IS_SELECTED_SCROLL(4))
        {
            // disable steppers
            enquecommand_P(PSTR("M84"));
            lcd_lib_keyclick();
        }
        else
        {
            menu.return_to_previous();
        }
    }
    lcd_lib_update_screen();
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
    if (!commands_queued() && !blocks_queued())
    {
        // enque next move
        if ((abs(TARGET_POS(axis) - current_position[axis])>0.005) && !endstop_reached(axis, (TARGET_POS(axis)>current_position[axis]) ? 1 : -1))
        {
            current_position[axis] = TARGET_POS(axis);
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], homing_feedrate[axis]/800, active_extruder);
        }
    }
}

static void stopMove()
{
    // stop moving
    quickStop();
    lcd_lib_encoder_pos = 0;
    movingSpeed = 0;
    delayMove = true;
    for (uint8_t i=0; i<NUM_AXIS; ++i)
    {
        TARGET_POS(i) = current_position[i] = constrain(st_get_position(i)/axis_steps_per_unit[i], min_pos[i], max_pos[i]);
    }
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
}

static void lcd_move_axis(AxisEnum axis, float diff)
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
        if (movingSpeed != 0)
        {
            movingSpeed = constrain(movingSpeed, -20, 20);
            if (abs(movingSpeed) < 6)
            {
                movingSpeed = 6*((movingSpeed > 0) - (movingSpeed < 0));
            }

            uint8_t steps = min(abs(movingSpeed)*2, (BLOCK_BUFFER_SIZE - movesplanned()) >> 1);
            for (uint8_t i = 0; i < steps; ++i)
            {
                TARGET_POS(axis) = round((current_position[axis]+float(movingSpeed)*diff)/diff)*diff;
                current_position[axis] = constrain(TARGET_POS(axis), min_pos[axis], max_pos[axis]);
                plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], abs(movingSpeed), active_extruder);
                if (endstop_reached(axis, movingSpeed))
                {
                    // quickstop
                    stopMove();
                    break;
                }
                else if ((movingSpeed < 0 && (TARGET_POS(axis) < min_pos[axis])) || ((movingSpeed > 0) && (TARGET_POS(axis) > max_pos[axis])))
                {
                    // stop planner
                    lcd_lib_encoder_pos = 0;
                    movingSpeed = 0;
                    delayMove = true;
                    last_user_interaction = millis();
                    break;
                }
            }
        }
    }
}

static void lcd_move_x_axis()
{
    lcd_move_axis(X_AXIS, 0.1);
}

static void lcd_move_y_axis()
{
    lcd_move_axis(Y_AXIS, 0.1);
}

static void lcd_move_z_axis()
{
    lcd_move_axis(Z_AXIS, 0.05);
}

static void lcd_position_x_axis()
{
    lcd_tune_value(TARGET_POS(X_AXIS), min_pos[X_AXIS], max_pos[X_AXIS], 0.05f);
    plan_move(X_AXIS);
}

static void lcd_position_y_axis()
{
    lcd_tune_value(TARGET_POS(Y_AXIS), min_pos[Y_AXIS], max_pos[Y_AXIS], 0.05f);
    plan_move(Y_AXIS);
}

static void lcd_position_z_axis()
{
    lcd_tune_value(TARGET_POS(Z_AXIS), min_pos[Z_AXIS], max_pos[Z_AXIS], 0.01f);
    plan_move(Z_AXIS);
}

FORCE_INLINE void lcd_home_x_axis() { enquecommand_P(PSTR("G28 X0")); }
FORCE_INLINE void lcd_home_y_axis() { enquecommand_P(PSTR("G28 Y0")); }

static void drawMoveDetails()
{
    char buffer[32] = {0};
    int_to_string(abs(movingSpeed), buffer, PSTR(UNIT_SPEED));
    lcd_lib_draw_string_center(5, buffer);
    if (movingSpeed <= 0)
    {
        lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 5, revSpeedGfx);
    }
    if (movingSpeed >= 0)
    {
        lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6, 5, speedGfx);
    }
}

static void pos2string(uint8_t flags, uint8_t axis, char *buffer)
{
    float_to_string2((flags & MENU_ACTIVE) ? TARGET_POS(axis) : st_get_position(axis) / axis_steps_per_unit[axis], buffer, PSTR("mm"));
}

// create menu options for "move axes"
static const menu_t & get_move_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Home all axis
        opt.setData(MENU_NORMAL, homeAll);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // x move
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_move_x_axis, stopMove, 2);
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
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_move_y_axis, stopMove, 2);
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
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_move_z_axis, stopMove, 2);
    }
    else if (nr == index++)
    {
        // z position
        opt.setData(MENU_INPLACE_EDIT, init_target_positions, lcd_position_z_axis, NULL, 5);
    }
    else if (nr == index++)
    {
        // z home
        opt.setData(MENU_NORMAL, homeBed);
    }
    return opt;
}

static void drawMoveSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Home all axis
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home all axes"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
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
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_MARGIN_LEFT
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // x move
        if (flags & MENU_ACTIVE)
        {
            drawMoveDetails();
            flags |= MENU_STATUSLINE;
        }
        else if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Move X axis"));
            flags |= MENU_STATUSLINE;
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
            flags |= MENU_STATUSLINE;
        }
        pos2string(flags, X_AXIS, buffer);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING
                              , 17
                              , 48
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // x home
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home X axis"));
            flags |= MENU_STATUSLINE;
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
            flags |= MENU_STATUSLINE;
        }
        else if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Move Y axis"));
            flags |= MENU_STATUSLINE;
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
            flags |= MENU_STATUSLINE;
        }
        pos2string(flags, Y_AXIS, buffer);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING
                              , 29
                              , 48
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // y home
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home Y axis"));
            flags |= MENU_STATUSLINE;
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
            flags |= MENU_STATUSLINE;
        }
        else if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Move Z axis"));
            flags |= MENU_STATUSLINE;
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
            flags |= MENU_STATUSLINE;
        }
        pos2string(flags, Z_AXIS, buffer);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING
                              , 41
                              , 48
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // z home
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Home Z axis"));
            flags |= MENU_STATUSLINE;
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

    uint8_t flags = 0;
    for (uint8_t index=0; index<11; ++index) {
        menu.drawSubMenu(drawMoveSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Move axis"));
    }

    lcd_lib_update_screen();
}

void manage_led_timeout()
{
    if ((led_timeout > 0) && !(sleep_state & SLEEP_LED_OFF))
    {
        const unsigned long timeout=last_user_interaction + (led_timeout*MILLISECONDS_PER_MINUTE);
        if (timeout < millis())
        {
            if (!(sleep_state & SLEEP_LED_DIMMED))
            {
                // dim LED
                analogWrite(LED_PIN, 255 * min(led_sleep_brightness, led_brightness_level) / 100);
                sleep_state ^= SLEEP_LED_DIMMED;
            }
        }
        else if (sleep_state & SLEEP_LED_DIMMED)
        {
            analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
            sleep_state ^= SLEEP_LED_DIMMED;
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
        if ((ui_mode & UI_MODE_EXPERT) && (menu.encoder_acceleration_factor() > 1))
        {
            if (encoder_pos_interrupt > 0)
            {
                // increase positive acceleration
                if (accelFactor >= 0)
                    ++accelFactor;
                else
                    accelFactor = 1;
            }
            else //if (lcd_lib_encoder_pos_interrupt <0)
            {
                // decrease negative acceleration
                if (accelFactor <= 0 )
                    --accelFactor;
                else
                    accelFactor = -1;
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

static void lcd_extrude_homehead()
{
    lcd_lib_keyclick();
    homeHead();
    enquecommand_P(PSTR("M84 X0 Y0"));
}

static void lcd_extrude_headtofront()
{
    lcd_lib_keyclick();
    // move to center front
    char buffer[32] = {0};
    sprintf_P(buffer, PSTR("G1 F12000 X%i Y%i"), int(AXIS_CENTER_POS(X_AXIS)), int(min_pos[Y_AXIS])+5);

    homeHead();
    enquecommand(buffer);
    enquecommand_P(PSTR("M84 X0 Y0"));
}

static void lcd_extrude_disablexy()
{
    lcd_lib_keyclick();
    process_command_P(PSTR("M84 X0 Y0"));
}

static const menu_t & get_extrude_tune_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_SCROLL_ITEM, lcd_extrude_homehead);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_SCROLL_ITEM, lcd_extrude_headtofront);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_SCROLL_ITEM, lcd_extrude_disablexy);
    }
    return opt;
}

static void drawExtrudeTuneSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_MARGIN_LEFT
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT
                           , 20
                           , LCD_GFX_WIDTH-LCD_CHAR_MARGIN_LEFT-LCD_CHAR_MARGIN_RIGHT
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(2*LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, 20, PSTR("Home head"));
            lcd_lib_clear_gfx(2*LCD_CHAR_MARGIN_LEFT, 20, homeGfx);
        }
        else
        {
            lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, 20, PSTR("Home head"));
            lcd_lib_draw_gfx(2*LCD_CHAR_MARGIN_LEFT, 20, homeGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT
                           , 30
                           , LCD_GFX_WIDTH-LCD_CHAR_MARGIN_LEFT-LCD_CHAR_MARGIN_RIGHT
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(2*LCD_CHAR_MARGIN_LEFT, 30, PSTR("Move head to front"));
        }
        else
        {
            lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT, 30, PSTR("Move head to front"));
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT
                           , 40
                           , LCD_GFX_WIDTH-LCD_CHAR_MARGIN_LEFT-LCD_CHAR_MARGIN_RIGHT
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(2*LCD_CHAR_MARGIN_LEFT, 40, PSTR("Disable XY-steppers"));
        }
        else
        {
            lcd_lib_draw_stringP(2*LCD_CHAR_MARGIN_LEFT, 40, PSTR("Disable XY-steppers"));
        }
    }
}

static void lcd_menu_tune_extrude()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_extrude_tune_menuoption, 4);

    uint8_t flags = 0;
    for (uint8_t index=0; index<4; ++index)
    {
        menu.drawSubMenu(drawExtrudeTuneSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Select option"));
    }
    lcd_lib_update_screen();
}

static void lcd_extrude_return()
{
    set_extrude_min_temp(EXTRUDE_MINTEMP);
    menu.return_to_previous();
    if (!card.sdprinting())
    {
        target_temperature[active_extruder] = 0;
    }
}

static void lcd_extrude_toggle_heater()
{
    // second target temperature
    uint16_t temp2(material[active_extruder].temperature[0]*9/20);
    temp2 -= temp2 % 10;
    if (!target_temperature[active_extruder])
    {
        target_temperature[active_extruder] = temp2;
    }
    else if (target_temperature[active_extruder] > temp2)
    {
        target_temperature[active_extruder] = 0;
    }
    else
    {
        target_temperature[active_extruder] = material[active_extruder].temperature[0];
    }
}

static void lcd_extrude_temperature()
{
    lcd_tune_value(target_temperature[active_extruder], 0, get_maxtemp(active_extruder) - 15);
}

static void lcd_extrude_reset_pos()
{
    lcd_lib_keyclick();
    current_position[E_AXIS] = 0.0f;
    plan_set_e_position(current_position[E_AXIS], active_extruder, true);
    TARGET_POS(E_AXIS) = current_position[E_AXIS];
}

static void lcd_extrude_init_move()
{
    st_synchronize();
    plan_set_e_position(st_get_position(E_AXIS) / e_steps_per_unit(active_extruder) / volume_to_filament_length[active_extruder], active_extruder, true);
    TARGET_POS(E_AXIS) = st_get_position(E_AXIS) / e_steps_per_unit(active_extruder);
}

static void lcd_extrude_move()
{
    if (printing_state == PRINT_STATE_NORMAL && movesplanned() < 3)
    {
        if (lcd_tune_value(TARGET_POS(E_AXIS), -10000.0, +10000.0, 0.1))
        {
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], TARGET_POS(E_AXIS) / volume_to_filament_length[active_extruder], 10, active_extruder);
        }
    }
}

static void lcd_extrude_quit_move()
{
    // disable E-steppers
    st_synchronize();
    process_command_P(PSTR("M84 E0"));
}

static void lcd_fastmove_init()
{
    st_synchronize();
    plan_set_e_position(st_get_position(E_AXIS) / e_steps_per_unit(active_extruder) / volume_to_filament_length[active_extruder], active_extruder, true);
    TARGET_POS(E_AXIS) = st_get_position(E_AXIS) / e_steps_per_unit(active_extruder);
    //Set E motor power lower so the motor will skip instead of grind.
#if EXTRUDERS > 1 && defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
    digipot_current(2, active_extruder ? (motor_current_e2*2/3) : (motor_current_setting[2]*2/3));
#else
    digipot_current(2, motor_current_setting[2]*2/3);
#endif
    //increase max. feedrate and reduce acceleration
    OLD_FEEDRATE = max_feedrate[E_AXIS];
    OLD_ACCEL = retract_acceleration;
    OLD_JERK = max_e_jerk;
    max_feedrate[E_AXIS] = float(FILAMENT_FAST_STEPS) / e_steps_per_unit(active_extruder);
    retract_acceleration = float(FILAMENT_LONG_ACCELERATION_STEPS) / e_steps_per_unit(active_extruder);
    max_e_jerk = FILAMENT_LONG_MOVE_JERK;
}

static void lcd_fastmove_quit()
{
    // reset feeedrate and acceleration to default
    max_feedrate[E_AXIS] = OLD_FEEDRATE;
    retract_acceleration = OLD_ACCEL;
    max_e_jerk = OLD_JERK;
    //Set E motor power to default.
#if EXTRUDERS > 1 && defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
    digipot_current(2, active_extruder ? motor_current_e2 : motor_current_setting[2]);
#else
    digipot_current(2, motor_current_setting[2]);
#endif
    // disable E-steppers
    lcd_extrude_quit_move();
}

static void lcd_extrude_fastmove(const float distance)
{
    if (lcd_lib_button_down)
    {
        if (printing_state == PRINT_STATE_NORMAL && !blocks_queued())
        {
            TARGET_POS(E_AXIS) += distance / volume_to_filament_length[active_extruder];
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], TARGET_POS(E_AXIS), max_feedrate[E_AXIS]*0.7f, active_extruder);
        }
    } else {
        quickStop();
        menu.reset_submenu();
    }
}

static void lcd_extrude_pull()
{
  #ifdef PREVENT_LENGTHY_EXTRUDE
    lcd_extrude_fastmove(-EXTRUDE_MAXLENGTH*9/10);
  #else
    lcd_extrude_fastmove(-FILAMENT_REVERSAL_LENGTH*2);
  #endif
}

static void lcd_extrude_load()
{
  #ifdef PREVENT_LENGTHY_EXTRUDE
    lcd_extrude_fastmove(EXTRUDE_MAXLENGTH*9/10);
  #else
    lcd_extrude_fastmove(FILAMENT_REVERSAL_LENGTH*2);
  #endif
}

static void lcd_extrude_tune()
{
    menu.add_menu(lcd_menu_tune_extrude);
}

static const menu_t & get_extrude_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (card.pause()) ++nr;

    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_extrude_tune);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_extrude_return);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_extrude_toggle_heater);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_extrude_temperature, 4);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_fastmove_init, lcd_extrude_pull, lcd_fastmove_quit);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_fastmove_init, lcd_extrude_load, lcd_fastmove_quit);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_extrude_reset_pos);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_extrude_init_move, lcd_extrude_move, lcd_extrude_quit_move, 2);
    }
    return opt;
}

static void drawExtrudeSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (card.pause()) ++nr;

    if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT + 1
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("More options"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_clear_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_draw_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_MARGIN_LEFT
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // toggle heater
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Toggle heater"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-13*LCD_CHAR_SPACING-LCD_CHAR_SPACING/2
                           , 20
                           , 2*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT+1
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-13*LCD_CHAR_SPACING, 20, thermometerGfx);
        }
        else
        {
            lcd_lib_draw_heater(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-13*LCD_CHAR_SPACING, 20, getHeaterPower(active_extruder));
        }
    }
    else if (nr == index++)
    {
        // temp nozzle
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            strcpy_P(buffer, PSTR("Nozzle "));
            char *c=buffer+7;
#if EXTRUDERS > 1
            strcpy_P(c, PSTR("("));
            c=int_to_string(active_extruder+1, c+1, PSTR(") "));
#endif
            int_to_string(target_temperature[active_extruder], int_to_string(dsp_temperature[active_extruder], c, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string_left(5, buffer);
            flags |= MENU_STATUSLINE;
        }

        int_to_string(dsp_temperature[active_extruder], buffer, PSTR(DEGREE_SLASH));
        lcd_lib_draw_string_right(LCD_GFX_WIDTH-2*LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 20, buffer);
        int_to_string(target_temperature[active_extruder], buffer, PSTR(DEGREE_SYMBOL));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                          , 20
                          , 24
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
    else if (nr == index++)
    {
        // reverse material
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click & hold to pull"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+2
                           , 35
                           , 2*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+5, 35, revSpeedGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+5, 35, revSpeedGfx);
        }
    }
    else if (nr == index++)
    {
        // load material
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click & hold to load"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+(3*LCD_CHAR_SPACING)+2
                           , 35
                           , 2*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+(3*LCD_CHAR_SPACING)+5, 35, fwdSpeedGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+(3*LCD_CHAR_SPACING)+5, 35, fwdSpeedGfx);
        }
    }
    else if (nr == index++)
    {
        // zero e position
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Reset position"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-13*LCD_CHAR_SPACING-LCD_CHAR_MARGIN_LEFT
                           , 35
                           , LCD_CHAR_SPACING+2*LCD_CHAR_MARGIN_LEFT
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-13*LCD_CHAR_SPACING-1, 35, flowGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-13*LCD_CHAR_SPACING-1, 35, flowGfx);
        }
    }
    else if (nr == index++)
    {
        // move material
        if (flags & MENU_ACTIVE)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Rotate to extrude"));
            flags |= MENU_STATUSLINE;
        }
        float_to_string2(flags & MENU_ACTIVE ? TARGET_POS(E_AXIS) : st_get_position(E_AXIS) / e_steps_per_unit(active_extruder), buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-11*LCD_CHAR_SPACING
                          , 35
                          , 11*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
}

void lcd_init_extrude()
{
    menu.set_active(get_extrude_menuoption, 6);
}

void lcd_menu_expert_extrude()
{
    // reset heater timeout until target temperature is reached
    if ((target_temperature[active_extruder]) && ((current_temperature[active_extruder] < 100) || (current_temperature[active_extruder] < (target_temperature[active_extruder] - 20))))
    {
        last_user_interaction = millis();
    }

    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    uint8_t len = card.pause() ? 7 : 8;

    menu.process_submenu(get_extrude_menuoption, len);

    uint8_t flags = 0;
    for (uint8_t index=0; index<len; ++index)
    {
        menu.drawSubMenu(drawExtrudeSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, 5, PSTR("Move material"));
#if EXTRUDERS > 1
    char buffer[8] = {0};
    strcpy_P(buffer, PSTR("("));
    int_to_string(active_extruder+1, buffer+1, PSTR(")"));
    lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT+(14*LCD_CHAR_SPACING), 5, buffer);
#endif
    }

    lcd_lib_update_screen();
}

void recover_start_print(const char *cmd)
{
    // recover print from current position
    card.stopPrinting();
    quickStop();
    clear_command_queue();
    // keep last command in mind
    strcpy(LCD_CACHE_FILENAME(0), cmd);
    printing_state = PRINT_STATE_START;

    // move to heatup position
    homeAll();
    char buffer[32] = {0};
    sprintf_P(buffer, PSTR("G1 F12000 X%i Y%i"), max(int(min_pos[X_AXIS]),0)+5, max(int(min_pos[Y_AXIS]),0)+5);
    enquecommand(buffer);

    menu.return_to_main();
    menu.add_menu(menu_t((ui_mode & UI_MODE_EXPERT) ? lcd_menu_print_heatup_tg : lcd_menu_print_heatup));
}

#endif//ENABLE_ULTILCD2
