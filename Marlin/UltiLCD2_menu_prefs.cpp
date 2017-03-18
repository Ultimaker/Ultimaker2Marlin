#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "Marlin.h"
#include "ConfigurationStore.h"
#include "planner.h"
#include "stepper.h"
#include "preferences.h"
#include "temperature.h"
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_utils.h"
#include "UltiLCD2_menu_prefs.h"

// we use the lcd_cache memory to keep previous values in mind
// #define FLOAT_SETTING(n) (*(float*)&lcd_cache[(n) * sizeof(float)])
// #define INT_SETTING(n) (*(int*)&lcd_cache[(n) * sizeof(int)])
#define WORD_SETTING(n) (*(uint16_t*)&lcd_cache[(n) * sizeof(uint16_t)])

uint8_t ui_mode = UI_MODE_EXPERT;
uint16_t lcd_timeout = LED_DIM_TIME;
uint8_t lcd_contrast = 0xDF;
uint8_t lcd_sleep_contrast = 0;
uint8_t led_sleep_glow = 0;
uint8_t control_flags = FLAG_PID_NOZZLE;
float end_of_print_retraction = END_OF_PRINT_RETRACTION;
uint8_t heater_check_temp = MAX_HEATING_TEMPERATURE_INCREASE;
uint8_t heater_check_time = MAX_HEATING_CHECK_MILLIS / 1000;
#if EXTRUDERS > 1
float pid2[3] = {DEFAULT_Kp, DEFAULT_Ki*PID_dT, DEFAULT_Kd/PID_dT};
#endif // EXTRUDERS
#if EXTRUDERS > 1 && defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
uint16_t motor_current_e2 = 0;
#endif
#if EXTRUDERS > 1
float e2_steps_per_unit = 282.0f;
#endif

uint8_t axis_direction = DEFAULT_AXIS_DIR;

uint16_t led_timeout = LED_DIM_TIME;
uint8_t led_sleep_brightness = 0;

// forward declarations
static bool autotune_callback(uint8_t state, uint8_t cycle, float kp, float ki, float kd);


/////

static void lcd_store_sleeptimer()
{
    SET_LED_TIMEOUT(led_timeout);
    SET_LCD_TIMEOUT(lcd_timeout);
    SET_SLEEP_BRIGHTNESS(led_sleep_brightness);
    SET_SLEEP_CONTRAST(lcd_sleep_contrast);
    SET_SLEEP_GLOW(led_sleep_glow);

    menu.return_to_previous();
}

static void lcd_sleep_led_timeout()
{
    lcd_tune_value(led_timeout, 0, LED_DIM_MAXTIME);
}

static void lcd_sleep_led_brightness()
{
    lcd_tune_byte(led_sleep_brightness, 0, 100);
}

static void lcd_sleep_led_glow()
{
    lcd_tune_byte(led_sleep_glow, 0, 100);
}

static void lcd_sleep_lcd_timeout()
{
    lcd_tune_value(lcd_timeout, 0, LED_DIM_MAXTIME);
}

static void lcd_sleep_lcd_contrast()
{
    lcd_tune_byte(lcd_sleep_contrast, 0, 100);
}

static const menu_t & get_sleeptimer_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;

    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_store_sleeptimer);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_sleep_led_timeout, 4);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_sleep_led_brightness, 4);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_sleep_lcd_timeout, 4);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_sleep_lcd_contrast, 4);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_sleep_led_glow, 4);
    }
    return opt;
}

static void drawSleepTimerSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};

    if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store options"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT + 1
                          , BOTTOM_MENU_YPOS
                          , 52
                          , LCD_CHAR_HEIGHT
                          , PSTR("STORE")
                          , ALIGN_CENTER
                          , flags);
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
        // LED sleep timeout
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("LED timeout"));
            flags |= MENU_STATUSLINE;
        }

        if (led_timeout > 0)
        {
            int_to_string(int(led_timeout), buffer, PSTR("min"));
        }
        else
        {
            strcpy_P(buffer, PSTR("off"));
        }

        lcd_lib_draw_string_leftP(18, PSTR("Light"));
        lcd_lib_draw_gfx(6+6*LCD_CHAR_SPACING, 18, clockGfx);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+8*LCD_CHAR_SPACING
                          , 18
                          , 6*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
    else if (nr == index++)
    {
        // LED sleep brightness
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("LED sleep brightness"));
            flags |= MENU_STATUSLINE;
        }
        int_to_string(int((float(led_sleep_brightness)*100/255)+0.5), buffer, PSTR("%"));
        lcd_lib_draw_gfx(LCD_GFX_WIDTH-2*LCD_CHAR_MARGIN_RIGHT-5*LCD_CHAR_SPACING, 18, brightnessGfx);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                          , 18
                          , 4*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
    else if (nr == index++)
    {
        // screen sleep timeout
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Screen timeout"));
            flags |= MENU_STATUSLINE;
        }
        if (lcd_timeout > 0)
        {
            int_to_string(int(lcd_timeout), buffer, PSTR("min"));
        }
        else
        {
            strcpy_P(buffer, PSTR("off"));
        }
        lcd_lib_draw_string_leftP(29, PSTR("Screen"));
        lcd_lib_draw_gfx(6+6*LCD_CHAR_SPACING, 29, clockGfx);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+8*LCD_CHAR_SPACING
                          , 29
                          , 6*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
    else if (nr == index++)
    {
        // LCD sleep contrast
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("LCD sleep contrast"));
            flags |= MENU_STATUSLINE;
        }
        if (lcd_sleep_contrast > 0)
        {
            int_to_string(int((float(lcd_sleep_contrast)*100/255)+0.5), buffer, PSTR("%"));
        }
        else
        {
            strcpy_P(buffer, PSTR("off"));
        }
        lcd_lib_draw_gfx(LCD_GFX_WIDTH-2*LCD_CHAR_MARGIN_RIGHT-5*LCD_CHAR_SPACING, 29, contrastGfx);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                          , 29
                          , 4*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
    else if (nr == index++)
    {
        // LED sleep encoder glow
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("LED sleep glow"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(40, PSTR("Glow"));
        int_to_string(int((float(led_sleep_glow)*100/255)+0.5), buffer, PSTR("%"));
        // lcd_lib_draw_gfx(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING, 15, brightnessGfx);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+8*LCD_CHAR_SPACING
                          , 40
                          , 6*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
}

void lcd_menu_sleeptimer()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);
// lcd_lib_led_color(8 + int(led_glow)*127/255, 8 + int(led_glow)*127/255, 32 + int(led_glow)*127/255);

    menu.process_submenu(get_sleeptimer_menuoption, 7);

    uint8_t flags = 0;
    for (uint8_t index=0; index<7; ++index) {
        menu.drawSubMenu(drawSleepTimerSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Sleep timer"));
    }

    lcd_lib_update_screen();
}

static void lcd_store_axislimit()
{
    eeprom_write_block(min_pos, (uint8_t *)EEPROM_AXIS_LIMITS, sizeof(min_pos));
    eeprom_write_block(max_pos, (uint8_t *)(EEPROM_AXIS_LIMITS+sizeof(min_pos)), sizeof(max_pos));
    menu.return_to_previous();
}

static void lcd_limit_xmin()
{
    lcd_tune_value(min_pos[X_AXIS], -99.0f, max_pos[X_AXIS], 0.1f);
}

static void lcd_limit_xmax()
{
    lcd_tune_value(max_pos[X_AXIS], min_pos[X_AXIS], +999.0f, 0.1f);
}

static void lcd_limit_ymin()
{
    lcd_tune_value(min_pos[Y_AXIS], -99.0f, max_pos[Y_AXIS], 0.1f);
}

static void lcd_limit_ymax()
{
    lcd_tune_value(max_pos[Y_AXIS], min_pos[Y_AXIS], +999.0f, 0.1f);
}

static void lcd_limit_zmax()
{
    lcd_tune_value(max_pos[Z_AXIS], 0.0f, +999.0f, 0.1f);
}

// create menu options for "print area"
static const menu_t & get_axislimit_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_axislimit);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // x min
        opt.setData(MENU_INPLACE_EDIT, lcd_limit_xmin, 2);
    }
    else if (nr == index++)
    {
        // x max
        opt.setData(MENU_INPLACE_EDIT, lcd_limit_xmax, 2);
    }
    else if (nr == index++)
    {
        // y min
        opt.setData(MENU_INPLACE_EDIT, lcd_limit_ymin, 2);
    }
    else if (nr == index++)
    {
        // y max
        opt.setData(MENU_INPLACE_EDIT, lcd_limit_ymax, 2);
    }
    else if (nr == index++)
    {
        // z max
        opt.setData(MENU_INPLACE_EDIT, lcd_limit_zmax, 2);
    }
    return opt;
}

static void drawAxisLimitSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store axis limits"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // x min
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Minimum X coordinate"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(17, PSTR("X"));
        float_to_string2(min_pos[X_AXIS], buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                                , 17
                                , 48
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // x max
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Maximum X coordinate"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2+51, 17, PSTR("-"));
        float_to_string2(max_pos[X_AXIS], buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 48
                                , 17
                                , 48
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // y min
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Minimum Y coordinate"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(29, PSTR("Y"));
        float_to_string2(min_pos[Y_AXIS], buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                                , 29
                                , 48
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // y max
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Maximum Y coordinate"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2+51, 29, PSTR("-"));
        float_to_string2(max_pos[Y_AXIS], buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 48
                                , 29
                                , 48
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // z max
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Maximum Z coordinate"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(41, PSTR("Z"));
        float_to_string2(max_pos[Z_AXIS], buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 48
                                , 41
                                , 48
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

void lcd_menu_axeslimit()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_axislimit_menuoption, 7);

    uint8_t flags = 0;
    for (uint8_t index=0; index<7; ++index) {
        menu.drawSubMenu(drawAxisLimitSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Print area"));
    }

    lcd_lib_update_screen();
}

static void lcd_cancel_steps()
{
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
    menu.return_to_previous();
}

static void lcd_store_steps()
{
    Config_StoreSettings();
#if EXTRUDERS > 1
    SET_STEPS_E2(e2_steps_per_unit);
#endif // EXTRUDERS
    lcd_cancel_steps();
}

static void lcd_steps_x()
{
    lcd_tune_value(axis_steps_per_unit[X_AXIS], 0.0f, 9999.0f, 0.01f);
}

static void lcd_steps_y()
{
    lcd_tune_value(axis_steps_per_unit[Y_AXIS], 0.0f, 9999.0f, 0.01f);
}

static void lcd_steps_z()
{
    lcd_tune_value(axis_steps_per_unit[Z_AXIS], 0.0f, 9999.0f, 0.01f);
}

static void lcd_steps_e1()
{
    lcd_tune_value(axis_steps_per_unit[E_AXIS], 0.0f, 9999.0f, 0.01f);
}

#if EXTRUDERS > 1
static void lcd_steps_e2()
{
    lcd_tune_value(e2_steps_per_unit, 0.0f, 9999.0f, 0.01f);
}
#endif // EXTRUDERS

// create menu options for "axis steps/mm"
static const menu_t & get_steps_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_steps);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_cancel_steps);
    }
    else if (nr == index++)
    {
        // x steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_x, 5);
    }
    else if (nr == index++)
    {
        // y steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_y, 5);
    }
    else if (nr == index++)
    {
        // z steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_z, 5);
    }
    else if (nr == index++)
    {
        // e1 steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_e1, 5);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // e1 steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_e2, 5);
    }
#endif // EXTRUDERS
    return opt;
}

static void drawStepsSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // x steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("X steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(18, PSTR("X"));
        float_to_string2(axis_steps_per_unit[X_AXIS], buffer, NULL);
        LCDMenu::drawMenuString((LCD_CHAR_MARGIN_LEFT*2)+LCD_CHAR_SPACING
                                , 18
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // y steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Y steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(28, PSTR("Y"));
        float_to_string2(axis_steps_per_unit[Y_AXIS], buffer, NULL);
        LCDMenu::drawMenuString((LCD_CHAR_MARGIN_LEFT*2)+LCD_CHAR_SPACING
                                , 28
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // z steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Z steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(38, PSTR("Z"));
        float_to_string2(axis_steps_per_unit[Z_AXIS], buffer, NULL);
        LCDMenu::drawMenuString((LCD_CHAR_MARGIN_LEFT*2)+LCD_CHAR_SPACING
                                , 38
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // e1 steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Extruder 1 steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - (LCD_CHAR_MARGIN_RIGHT*2) - (LCD_CHAR_SPACING*9), 18, PSTR("E1"));
        float_to_string2(axis_steps_per_unit[E_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                                , 18
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // e2 steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Extruder 2 steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - (LCD_CHAR_MARGIN_RIGHT*2) - (LCD_CHAR_SPACING*9), 28, PSTR("E2"));
        float_to_string2(e2_steps_per_unit, buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                                , 28
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
#else
    else if (nr == index++)
    {
        // x steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("X steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(20, PSTR("X"));
        float_to_string2(axis_steps_per_unit[X_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                                , 20
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // y steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Y steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(32, PSTR("Y"));
        float_to_string2(axis_steps_per_unit[Y_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                                , 32
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // z steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Z steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*9), 20, PSTR("Z"));
        float_to_string2(axis_steps_per_unit[Z_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                                , 20
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // e steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("E steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*9), 32, PSTR("E"));
        float_to_string2(axis_steps_per_unit[E_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                                , 32
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
#endif // EXTRUDERS
}

void lcd_menu_steps()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_steps_menuoption, 5+EXTRUDERS);

    uint8_t flags = 0;
    for (uint8_t index=0; index<5+EXTRUDERS; ++index) {
        menu.drawSubMenu(drawStepsSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Axis steps/mm"));
    }

    lcd_lib_update_screen();
}

static void lcd_store_axisdir()
{
    SET_AXIS_DIRECTION(axis_direction);
    menu.return_to_previous();
}

static void lcd_axisdir_x()
{
    axis_direction ^= 0x01;
}

static void lcd_axisdir_y()
{
    axis_direction ^= 0x02;
}

static void lcd_axisdir_z()
{
    axis_direction ^= 0x04;
}

static void lcd_axisdir_e1()
{
    axis_direction ^= 0x08;
}

#if EXTRUDERS > 1
static void lcd_axisdir_e2()
{
    axis_direction ^= 0x10;
}
#endif // EXTRUDERS

// create menu options for "axis steps/mm"
static const menu_t & get_axisdir_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_axisdir);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // invert x
        opt.setData(MENU_NORMAL, lcd_axisdir_x);
    }
    else if (nr == index++)
    {
        // invert y
        opt.setData(MENU_NORMAL, lcd_axisdir_y);
    }
    else if (nr == index++)
    {
        // invert z
        opt.setData(MENU_NORMAL, lcd_axisdir_z);
    }
    else if (nr == index++)
    {
        // invert e1
        opt.setData(MENU_NORMAL, lcd_axisdir_e1);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // invert e2
        opt.setData(MENU_NORMAL, lcd_axisdir_e2);
    }
#endif // EXTRUDERS
    return opt;
}

static void drawInvertStatusP(const char * strAxis, bool bInvert)
{
    uint8_t xPos(LCD_CHAR_MARGIN_LEFT);
    lcd_lib_draw_string_leftP(5, PSTR("Invert"));
    xPos += LCD_CHAR_SPACING*7;
    lcd_lib_draw_stringP(xPos, 5, strAxis);
    xPos += LCD_CHAR_SPACING*(strlen_P(strAxis));
    lcd_lib_draw_stringP(xPos, 5, PSTR(":"));
    xPos += LCD_CHAR_SPACING*2;
    lcd_lib_draw_stringP(xPos, 5, bInvert?PSTR("yes"):PSTR("no"));
}

static void drawAxisDirSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store invert flags"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // invert x
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("X"), INVERT_X_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(18, PSTR("X"));
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                           , 18
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(9+LCD_CHAR_SPACING*2, 18, INVERT_X_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(9+LCD_CHAR_SPACING*2, 18, INVERT_X_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert y
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Y"), INVERT_Y_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(28, PSTR("Y"));
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                           , 28
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(9+LCD_CHAR_SPACING*2, 28, INVERT_Y_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(9+LCD_CHAR_SPACING*2, 28, INVERT_Y_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert z
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Z"), INVERT_Z_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(38, PSTR("Z"));
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                           , 38
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(9+LCD_CHAR_SPACING*2, 38, INVERT_Z_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(9+LCD_CHAR_SPACING*2, 38, INVERT_Z_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert e0
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Extr.1"), INVERT_E0_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - (LCD_CHAR_MARGIN_RIGHT*2) - (LCD_CHAR_SPACING*9), 18, PSTR("E1"));
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                           , 18
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 18, INVERT_E0_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 18, INVERT_E0_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert e1
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Extr.2"), INVERT_E1_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - (LCD_CHAR_MARGIN_RIGHT*2) - (LCD_CHAR_SPACING*9), 28, PSTR("E2"));
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                           , 28
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 28, INVERT_E1_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 28, INVERT_E1_DIR?checkbox_on:checkbox_off);
        }
    }
#else
    else if (nr == index++)
    {
        // invert x steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("X"), INVERT_X_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(20, PSTR("X"));
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                           , 20
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(9+LCD_CHAR_SPACING*2, 20, INVERT_X_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(9+LCD_CHAR_SPACING*2, 20, INVERT_X_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert y
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Y"), INVERT_Y_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(32, PSTR("Y"));
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                           , 32
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(9+LCD_CHAR_SPACING*2, 32, INVERT_Y_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(9+LCD_CHAR_SPACING*2, 32, INVERT_Y_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert z
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Z"), INVERT_Z_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*9), 20, PSTR("Z"));
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                           , 20
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 20, INVERT_Z_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 20, INVERT_Z_DIR?checkbox_on:checkbox_off);
        }
    }
    else if (nr == index++)
    {
        // invert e
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            drawInvertStatusP(PSTR("Extruder"), INVERT_E0_DIR);
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*9), 32, PSTR("E"));
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*7)
                           , 32
                           , 3*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 32, INVERT_E0_DIR?checkbox_on:checkbox_off);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)-1, 32, INVERT_E0_DIR?checkbox_on:checkbox_off);
        }
    }
#endif // EXTRUDERS
}

void lcd_menu_axisdirection()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_axisdir_menuoption, 5+EXTRUDERS);

    uint8_t flags = 0;
    for (uint8_t index=0; index<5+EXTRUDERS; ++index) {
        menu.drawSubMenu(drawAxisDirSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Invert axis"));
    }

    lcd_lib_update_screen();
}

static void lcd_preset_retract_speed()
{
    lcd_tune_speed(retract_feedrate, 0, max_feedrate[E_AXIS]*60);
}

static void lcd_preset_retract_length()
{
    lcd_tune_value(retract_length, 0, 50, 0.01);
}

#if EXTRUDERS > 1
static void lcd_preset_swap_length()
{
    lcd_tune_value(extruder_swap_retract_length, 0, 50, 0.01);
}
#endif // EXTRUDERS

static void lcd_preset_endofprint_length()
{
    lcd_tune_value(end_of_print_retraction, 0, 50, 0.01);
}

static void store_endofprint_retract()
{
    SET_END_RETRACT(end_of_print_retraction);
}

static void lcd_store_retraction()
{
    Config_StoreSettings();
    store_endofprint_retract();
    menu.return_to_previous();
}

// create menu options for "retraction"
static const menu_t & get_retract_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_retraction);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // retraction length
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_retract_length, 2);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // extruder swap length
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_swap_length, 2);
    }
#endif
    else if (nr == index++)
    {
        // retraction speed
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_retract_speed);
    }
    else if (nr == index++)
    {
        // end of print length
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_endofprint_length, 2);
    }
    return opt;
}

static void drawRetractSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // retract length
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Retract length"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(16, PSTR("Retract"));
        lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 16, retractLenGfx);
        float_to_string2(retract_length, buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                              , 16
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
        lcd_lib_draw_string_leftP(26, PSTR("E"));
        float_to_string2(extruder_swap_retract_length, buffer, PSTR("mm"));
        LCDMenu::drawMenuString(2*LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING
                              , 26
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
        lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 26, retractSpeedGfx);
        int_to_string(retract_feedrate / 60 + 0.5, buffer, PSTR("mm/s"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                              , 26
                              , 7*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
       // end of print length
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("End of print length"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(41, PSTR("End length"));
        float_to_string2(end_of_print_retraction, buffer, PSTR("mm"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                              , 41
                              , 7*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
}

void lcd_menu_retraction()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

#if EXTRUDERS > 1
    menu.process_submenu(get_retract_menuoption, 6);
#else
    menu.process_submenu(get_retract_menuoption, 5);
#endif

    uint8_t flags = 0;
#if EXTRUDERS > 1
    for (uint8_t index=0; index<6; ++index) {
        menu.drawSubMenu(drawRetractSubmenu, index, flags);
    }
#else
    for (uint8_t index=0; index<5; ++index) {
        menu.drawSubMenu(drawRetractSubmenu, index, flags);
    }
#endif
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Retraction settings"));
    }

    lcd_lib_update_screen();
}

static void lcd_store_motorcurrent()
{
#if (EXTRUDERS > 1) && defined(MOTOR_CURRENT_PWM_E_PIN) && (MOTOR_CURRENT_PWM_E_PIN > -1)
    for (uint8_t i=0; i<2; ++i)
    {
        digipot_current(i, motor_current_setting[i]);
    }
    //Set E motor power to default.
    digipot_current(2, active_extruder ? motor_current_e2 : motor_current_setting[2]);
    SET_MOTOR_CURRENT_E2(motor_current_e2);
#else
    for (uint8_t i=0; i<3; ++i)
    {
        digipot_current(i, motor_current_setting[i]);
    }
#endif
    Config_StoreSettings();
    menu.return_to_previous();
}

static void lcd_preset_current_xy()
{
    if (lcd_tune_value(motor_current_setting[0], 0, 1500))
    {
        digipot_current(0, motor_current_setting[0]);
    }
}

static void lcd_preset_current_z()
{
    if (lcd_tune_value(motor_current_setting[1], 0, 1500))
    {
        digipot_current(1, motor_current_setting[1]);
    }
}

static void lcd_preset_current_e()
{
    if (lcd_tune_value(motor_current_setting[2], 0, 1500))
    {
        if (!active_extruder)
        {
            digipot_current(2, motor_current_setting[2]);
        }
    }
}

#if (EXTRUDERS > 1) && defined(MOTOR_CURRENT_PWM_E_PIN) && (MOTOR_CURRENT_PWM_E_PIN > -1)
static void lcd_preset_current_e2()
{
    if (lcd_tune_value(motor_current_e2, 0, 1500))
    {
        if (active_extruder)
        {
            digipot_current(2, motor_current_e2);
        }
    }
}
#endif // EXTRUDERS

// create menu options for "Motor current"
static const menu_t & get_current_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_motorcurrent);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // X/Y motor current
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_current_xy, 2);
    }
    else if (nr == index++)
    {
        // Z motor current
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_current_z, 2);
    }
    else if (nr == index++)
    {
        // motor current extruder 1
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_current_e, 2);
    }
#if (EXTRUDERS > 1) && defined(MOTOR_CURRENT_PWM_E_PIN) && (MOTOR_CURRENT_PWM_E_PIN > -1)
    else if (nr == index++)
    {
        // motor current extruder 2
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_current_e2, 2);
    }
#endif
    return opt;
}

static void drawCurrentSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // X/Y motor current
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("X/Y axis (mA)"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(20, PSTR("X/Y"));
        int_to_string(motor_current_setting[0], buffer, NULL);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+4*LCD_CHAR_SPACING
                              , 20
                              , 4*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // Z motor current
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Z axis (mA)"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(32, PSTR("Z"));
        int_to_string(motor_current_setting[1], buffer, NULL);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+4*LCD_CHAR_SPACING
                              , 32
                              , 4*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#if (EXTRUDERS > 1) && defined(MOTOR_CURRENT_PWM_E_PIN) && (MOTOR_CURRENT_PWM_E_PIN > -1)
    else if (nr == index++)
    {
        // E1 motor current
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Extruder 1 (mA)"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING, 20, PSTR("E1"));
        int_to_string(motor_current_setting[2], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 20
                              , 4*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // E2 motor current
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Extruder 2 (mA)"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING, 32, PSTR("E2"));
        int_to_string(motor_current_e2, buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 32
                              , 4*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#else
    else if (nr == index++)
    {
        // E motor current
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Extruder (mA)"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6*LCD_CHAR_SPACING, 20, PSTR("E"));
        int_to_string(motor_current_setting[2], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-4*LCD_CHAR_SPACING
                              , 20
                              , 4*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
#endif // EXTRUDERS
}

void lcd_menu_motorcurrent()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

#if (EXTRUDERS > 1) && defined(MOTOR_CURRENT_PWM_E_PIN) && (MOTOR_CURRENT_PWM_E_PIN > -1)
    menu.process_submenu(get_current_menuoption, 6);
#else
    menu.process_submenu(get_current_menuoption, 5);
#endif // EXTRUDERS

    uint8_t flags = 0;
#if (EXTRUDERS > 1) && defined(MOTOR_CURRENT_PWM_E_PIN) && (MOTOR_CURRENT_PWM_E_PIN > -1)
    for (uint8_t index=0; index<6; ++index) {
#else
    for (uint8_t index=0; index<5; ++index) {
#endif // EXTRUDERS
        menu.drawSubMenu(drawCurrentSubmenu, index, flags);
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Motor Current"));
    }

    lcd_lib_update_screen();
}

static void lcd_store_maxspeed()
{
    Config_StoreSettings();
    menu.return_to_previous();
}

static void lcd_preset_maxspeed_x()
{
    lcd_tune_value(max_feedrate[X_AXIS], 0, 999, 1.0);
}

static void lcd_preset_maxspeed_y()
{
    lcd_tune_value(max_feedrate[Y_AXIS], 0, 999, 1.0);
}

static void lcd_preset_maxspeed_z()
{
    lcd_tune_value(max_feedrate[Z_AXIS], 0, 99, 1.0);
}

static void lcd_preset_maxspeed_e()
{
    lcd_tune_value(max_feedrate[E_AXIS], 0, 99, 1.0);
}

// create menu options for "Max speed"
static const menu_t & get_maxspeed_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_maxspeed);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // X max speed
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_maxspeed_x, 2);
    }
    else if (nr == index++)
    {
        // Y max speed
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_maxspeed_y, 2);
    }
    else if (nr == index++)
    {
        // Z max speed
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_maxspeed_z, 2);
    }
    else if (nr == index++)
    {
        // E max speed
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_maxspeed_e, 2);
    }
    return opt;
}

static void drawMaxSpeedSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // X max speed
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Max. speed X"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(20, PSTR("X"));
        int_to_string(max_feedrate[X_AXIS], buffer, PSTR(UNIT_SPEED));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+2*LCD_CHAR_SPACING
                              , 20
                              , 7*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // Y max speed
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Max. speed Y"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(35, PSTR("Y"));
        int_to_string(max_feedrate[Y_AXIS], buffer, PSTR(UNIT_SPEED));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+2*LCD_CHAR_SPACING
                              , 35
                              , 7*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // Z max speed
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Max. speed Z"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-8*LCD_CHAR_SPACING, 20, PSTR("Z"));
        int_to_string(max_feedrate[Z_AXIS], buffer, PSTR(UNIT_SPEED));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6*LCD_CHAR_SPACING
                              , 20
                              , 6*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // E max speed
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Max. speed E"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-8*LCD_CHAR_SPACING, 35, PSTR("E"));
        int_to_string(max_feedrate[E_AXIS], buffer, PSTR(UNIT_SPEED));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6*LCD_CHAR_SPACING
                              , 35
                              , 6*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
}

void lcd_menu_maxspeed()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_maxspeed_menuoption, 6);

    uint8_t flags = 0;
    for (uint8_t index=0; index<6; ++index) {
        menu.drawSubMenu(drawMaxSpeedSubmenu, index, flags);
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Max. Speed"));
    }

    lcd_lib_update_screen();
}

static void lcd_store_acceleration()
{
    Config_StoreSettings();
    menu.return_to_previous();
}

static void lcd_preset_acceleration()
{
    lcd_tune_value(acceleration, 0, 20000, 100);
}

static void lcd_preset_jerk()
{
    lcd_tune_value(max_xy_jerk, 0, 100, 1.0);
}

// create menu options for "acceleration and jerk"
static const menu_t & get_acceleration_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_acceleration);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // acceleration
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_acceleration, 1);
    }
    else if (nr == index++)
    {
        // max x/y jerk
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_jerk, 1);
    }
    return opt;
}

static void drawAccelerationSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // Acceleration
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Acceleration"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(20, PSTR("Accel"));
        int_to_string(acceleration, buffer, PSTR(UNIT_ACCELERATION));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-10*LCD_CHAR_SPACING
                              , 20
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

        lcd_lib_draw_string_leftP(35, PSTR("X/Y Jerk"));
        int_to_string(max_xy_jerk, buffer, PSTR("mm/s"));
        LCDMenu::drawMenuString(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING
                              , 35
                              , 7*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
}

void lcd_menu_acceleration()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_acceleration_menuoption, 4);

    uint8_t flags = 0;
    for (uint8_t index=0; index<4; ++index) {
        menu.drawSubMenu(drawAccelerationSubmenu, index, flags);
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Acceleration"));
    }

    lcd_lib_update_screen();
}

#if EXTRUDERS > 1

void init_swap_menu()
{
    menu.set_selection(2);
}

static void lcd_store_swap()
{
    SET_CONTROL_FLAGS(control_flags);
    menu.return_to_previous();
}

static void lcd_swap_extruders()
{
    control_flags ^= FLAG_SWAP_EXTRUDERS;
}

// create menu options for "acceleration and jerk"
static const menu_t & get_swap_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_swap);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // swap extruders
        opt.setData(MENU_NORMAL, lcd_swap_extruders);
    }
    return opt;
}

static void drawSwapSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // Swap Extruders
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Swap Extruders"));
            flags |= MENU_STATUSLINE;
        }

        lcd_lib_draw_string_centerP(16, PSTR("Extruder sequence"));
        lcd_lib_draw_string_centerP(26, PSTR("during printing"));

        uint8_t xpos = (swapExtruders() ? LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-7*LCD_CHAR_SPACING : LCD_CHAR_MARGIN_LEFT);
        lcd_lib_draw_stringP(xpos, 38, PSTR("PRIMARY"));

        xpos = (swapExtruders() ? LCD_CHAR_MARGIN_LEFT : LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT-6*LCD_CHAR_SPACING);
        lcd_lib_draw_stringP(xpos, 38, PSTR("SECOND"));

        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2-2*LCD_CHAR_MARGIN_LEFT-LCD_CHAR_SPACING
                              , 38
                              , 4*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , PSTR("<->")
                              , ALIGN_CENTER
                              , flags);
    }
}

void lcd_menu_swap_extruder()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_swap_menuoption, 3);

    uint8_t flags = 0;
    for (uint8_t index=0; index<3; ++index) {
        menu.drawSubMenu(drawSwapSubmenu, index, flags);
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Swap Extruders"));
    }

    lcd_lib_update_screen();
}

static void lcd_store_pid2()
{
    eeprom_write_block(pid2, (uint8_t *)(EEPROM_PID_2), sizeof(pid2));
    menu.return_to_previous();
}

#endif

static void lcd_store_heatercheck()
{
    SET_HEATER_TIMEOUT(heater_timeout);
    SET_HEATER_CHECK_TEMP(heater_check_temp);
    SET_HEATER_CHECK_TIME(heater_check_time);
    menu.return_to_previous();
}

static void lcd_preset_heatercheck_time()
{
    lcd_tune_value(heater_check_time, 0, 120);
}

static void lcd_preset_heater_timeout()
{
    lcd_tune_value(heater_timeout, 0, 60);
}

// create menu options for "Heater check"
static const menu_t & get_heatercheck_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_heatercheck);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // heater timeout
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_heater_timeout, 2);
    }
    else if (nr == index++)
    {
        // period of max. power
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_heatercheck_time, 2);
    }
    return opt;
}

static void drawHeatercheckSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store options"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 52
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
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
        // max. time
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Idle timeout"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(20, PSTR("Timeout"));
        if (heater_timeout)
            int_to_string(heater_timeout, buffer, PSTR(" min"));
        else
            strcpy_P(buffer, PSTR("off"));

        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*12
                                , 20
                                , LCD_CHAR_SPACING*6
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // max. time
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Period of max. power"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(32, PSTR("Max. power"));
        if (heater_check_time)
            int_to_string(heater_check_time, buffer, PSTR(" sec"));
        else
            strcpy_P(buffer, PSTR("off"));

        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*11
                                , 32
                                , LCD_CHAR_SPACING*7
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

void lcd_menu_heatercheck()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_heatercheck_menuoption, 4);

    uint8_t flags = 0;
    for (uint8_t index=0; index<4; ++index) {
        menu.drawSubMenu(drawHeatercheckSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Heater check"));
    }

    lcd_lib_update_screen();
}

static void lcd_preset_autotune_temp()
{
    lcd_tune_value(WORD_SETTING(2), WORD_SETTING(3), WORD_SETTING(4));
}

static void lcd_preset_autotune_cycles()
{
    lcd_tune_value(lcd_cache[2], 2, 20);
}

static void lcd_cancel_autotune()
{
    lcd_cache[0] |= AUTOTUNE_ABORT;
}

static const menu_t & get_pidinfo_menuoption(uint8_t nr, menu_t &opt)
{
    if (nr == 0)
    {
        // CANCEL
        opt.setData(MENU_NORMAL, lcd_cancel_autotune);
    }
    return opt;
}

static void drawPIDInfoSubmenu(uint8_t nr, uint8_t &flags)
{
    // uint8_t index(0);
    // char buffer[32] = {0};
    // if (nr == index++)
    if (nr == 0)
    {
        // Cancel
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 - 4*LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 8*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("CANCEL")
                                , ALIGN_CENTER
                                , flags);
    }
}

static void autotune_finished()
{
    menu.return_to_previous();
    menu.set_selection(0);
}

static void lcd_menu_autotune_info()
{
    if (lcd_cache[0] == 0)
    {
        lcd_basic_screen();
        lcd_lib_draw_hline(3, 124, 13);

        char buffer[32] = {0};

        lcd_lib_draw_string_leftP(5, PSTR("PID tuning"));

        // progress indicator
        if (!led_glow_dir)
        {
            for (uint8_t i=0; i < (led_glow >> 5); ++i)
            {
                lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + (10+i)*LCD_CHAR_SPACING, 5, PSTR("."));
            }
        }
        int_to_string(lcd_cache[2], int_to_string(lcd_cache[3], buffer, PSTR("/")), NULL);
        lcd_lib_draw_string_right(5, buffer);

        menu.process_submenu(get_pidinfo_menuoption, 1);
        uint8_t flags = 0;
        menu.drawSubMenu(drawPIDInfoSubmenu, 0, flags);

        // draw current temperature
        if ((lcd_cache[1] > 0) && (lcd_cache[1] <= EXTRUDERS))
        {
            lcd_lib_draw_heater(LCD_CHAR_MARGIN_LEFT, 29, getHeaterPower(lcd_cache[1]-1));

            int_to_string(WORD_SETTING(2), int_to_string(dsp_temperature[lcd_cache[1]-1], buffer, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, 29, buffer);

            lcd_lib_draw_string_leftP(17, PSTR("Extruder"));
#if (EXTRUDERS > 1)
            int_to_string(lcd_cache[1], buffer, NULL);
            lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT + 9*LCD_CHAR_SPACING, 17, buffer);
#endif
        }
#if (TEMP_SENSOR_BED != 0)
        else if (lcd_cache[1] == 0)
        {
            lcd_lib_draw_string_leftP(17, PSTR("Buildplate"));

            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT, 29, bedTempGfx);
            int_to_string(WORD_SETTING(2), int_to_string(dsp_temperature_bed, buffer, PSTR(DEGREE_SLASH)), PSTR(DEGREE_SYMBOL));
            lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT + 2*LCD_CHAR_SPACING, 29, buffer);
        }
#endif
        lcd_progressbar((124*lcd_cache[3])/lcd_cache[2]);
    }
    else if (lcd_cache[0] & AUTOTUNE_ABORT)
    {
        menu.return_to_previous();
    }
    else
    {
        // PID tuning finished
        lcd_info_screen(NULL, autotune_finished, PSTR("CONTINUE"));
        if (lcd_cache[0] & AUTOTUNE_OK)
        {
            lcd_lib_draw_string_centerP(10, PSTR("PID Autotune"));
            lcd_lib_draw_string_centerP(20, PSTR("finished!"));
            lcd_lib_draw_string_centerP(30, PSTR("You can store"));
            lcd_lib_draw_string_centerP(40, PSTR("the settings."));
        }
        else if (lcd_cache[0] & AUTOTUNE_TEMP_HIGH)
        {
            lcd_lib_draw_string_centerP(10, PSTR("PID Autotune"));
            lcd_lib_draw_string_centerP(20, PSTR("failed!"));
            lcd_lib_draw_string_centerP(30, PSTR("Temperature too high"));
        }
        else if (lcd_cache[0] & AUTOTUNE_TIMEOUT)
        {
            lcd_lib_draw_string_centerP(10, PSTR("PID Autotune"));
            lcd_lib_draw_string_centerP(20, PSTR("failed!"));
            lcd_lib_draw_string_centerP(30, PSTR("Timeout"));
        }
    }

    lcd_lib_update_screen();
}


static void lcd_start_autotune()
{
    lcd_cache[0] = 0; // status flag
    menu.replace_menu(menu_t(lcd_menu_autotune_info));
    PID_autotune(float(WORD_SETTING(2)), lcd_cache[1]-1, lcd_cache[2], autotune_callback);
}

static const menu_t & get_autotune_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // START
        opt.setData(MENU_NORMAL, lcd_start_autotune);
    }
    else if (nr == index++)
    {
        // CANCEL
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // PID autotune temperature
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_autotune_temp, 4);

    }
    else if (nr == index++)
    {
        // PID autotune cycles
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_autotune_cycles);

    }
    return opt;
}

static void drawAutotuneSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Start
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Start PID autotune"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("START")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Cancel
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT - 7*LCD_CHAR_SPACING
                           , BOTTOM_MENU_YPOS
                           , 7*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // tuning temperature
        if (flags & (MENU_ACTIVE | MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("tuning temperature"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT, 30, PSTR("Temperature"));
        int_to_string(WORD_SETTING(2), buffer, PSTR(DEGREE_SYMBOL));

        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT + 12*LCD_CHAR_SPACING
                                , 30
                                , 4*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // tuning cycles
        if (flags & (MENU_ACTIVE | MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("tuning cycles"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+5*LCD_CHAR_SPACING, 40, PSTR("Cycles"));
        int_to_string(lcd_cache[2], buffer, NULL);

        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT + 13*LCD_CHAR_SPACING
                                , 40
                                , 2*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

static void lcd_menu_autotune_params()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_autotune_menuoption, 4);

    uint8_t flags = 0;
    for (uint8_t index=0; index<4; ++index) {
        menu.drawSubMenu(drawAutotuneSubmenu, index, flags);
    }
#if (TEMP_SENSOR_BED != 0)
    if (lcd_cache[1] == 0)
    {
        lcd_lib_draw_string_leftP(17, PSTR("Buildplate"));
    }
    else
#endif
    {
        lcd_lib_draw_string_leftP(17, PSTR("Extruder"));
#if (EXTRUDERS > 1)
        char buffer[3] = {0};
        int_to_string(lcd_cache[1], buffer, NULL);
        lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT + 9*LCD_CHAR_SPACING, 17, buffer);
#endif
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("PID tune options"));
    }

    lcd_lib_update_screen();
}

static bool autotune_callback(uint8_t state, uint8_t cycle, float kp, float ki, float kd)
{
    lcd_cache[3] = cycle;
    if (lcd_cache[0] & AUTOTUNE_ABORT)
    {
        return false;
    }
    else
    {
        lcd_cache[0] = state;
        if (state & AUTOTUNE_OK)
        {
            // PID tuning successful
            if (lcd_cache[1] == 1)
            {
                Kp = kp;
                Ki = scalePID_i(ki);
                Kd = scalePID_d(kd);
            }
#if (EXTRUDERS > 1)
            else if (lcd_cache[1] == 2)
            {
                pid2[0] = kp;
                pid2[1] = scalePID_i(ki);
                pid2[2] = scalePID_d(kd);
            }
#endif
#if (TEMP_SENSOR_BED != 0)
            else if (lcd_cache[1] == 0)
            {
                bedKp = kp;
                bedKi = scalePID_i(ki);
                bedKd = scalePID_d(kd);
            }
#endif
            menu.set_selection(0);
        }
        return (state == 0);
    }
}

static void init_autotune_params()
{
    menu.set_selection(2);
}

static void autotune_params_e1()
{
    lcd_clear_cache();
    lcd_cache[0] = 0; // status flag
    lcd_cache[1] = 1; // extruder 1
    lcd_cache[2] = 5; // cycles
    lcd_cache[3] = 0; // current cycle
    WORD_SETTING(2) = 150; // tuning temperature
    WORD_SETTING(3) = 40; // min temperature
    WORD_SETTING(4) = get_maxtemp(0)-15; // max temperature
    menu.add_menu(menu_t(init_autotune_params, lcd_menu_autotune_params, NULL));
}

#if (TEMP_SENSOR_BED != 0)
static void autotune_params_bed()
{
    lcd_clear_cache();
    lcd_cache[0] = 0; // status flag
    lcd_cache[1] = 0; // buildplate
    lcd_cache[2] = 5; // cycles
    lcd_cache[3] = 0; // current cycle
    WORD_SETTING(2) = 70; // tuning temperature
    WORD_SETTING(3) = 40; // min temperature
    WORD_SETTING(4) = BED_MAXTEMP-15; // max temperature
    menu.add_menu(menu_t(init_autotune_params, lcd_menu_autotune_params, NULL));
}
#endif

#if (EXTRUDERS > 1)
static void autotune_params_e2()
{
    lcd_clear_cache();
    lcd_cache[0] = 0; // status flag
    lcd_cache[1] = 2; // extruder 2
    lcd_cache[2] = 5; // cycles
    lcd_cache[3] = 0; // current cycle
    WORD_SETTING(2) = 150; // tuning temperature
    WORD_SETTING(3) = 40; // min temperature
    WORD_SETTING(4) = get_maxtemp(1)-15; // max temperature
    menu.add_menu(menu_t(init_autotune_params, lcd_menu_autotune_params, NULL));
}
#endif

#if (TEMP_SENSOR_BED != 0) || (EXTRUDERS > 1)
static void init_tempcontrol_e1()
#else
void init_tempcontrol_e1()
#endif
{
    FLOAT_SETTING(1) = unscalePID_i(Ki);
    FLOAT_SETTING(2) = unscalePID_d(Kd);
    // menu.set_selection(1);
}

static void lcd_store_pid()
{
    Config_StoreSettings();
    menu.return_to_previous();
}

static void lcd_preset_e1_kd()
{
    if (lcd_tune_value(FLOAT_SETTING(2), 0.0f, 999.99f, 0.01f))
    {
        Kd = scalePID_d(FLOAT_SETTING(2));
    }
}

static void lcd_preset_e1_ki()
{
    if (lcd_tune_value(FLOAT_SETTING(1), 0.0f, 999.99f, 0.01f))
    {
        Ki = scalePID_i(FLOAT_SETTING(1));
    }
}

static void lcd_preset_e1_kp()
{
    lcd_tune_value(Kp, 0.0f, 999.99f, 0.01f);
}

static const menu_t & get_temp_e1_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_pid);
    }
    else if (nr == index++)
    {
        // AUTOTUNE
        opt.setData(MENU_NORMAL, autotune_params_e1);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // PID proportional coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_e1_kp, 4);

    }
    else if (nr == index++)
    {
        // PID integral coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_e1_ki, 4);

    }
    else if (nr == index++)
    {
        // PID derivative coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_e1_kd, 4);

    }
    return opt;
}

static void drawTempExtr1Submenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store PID settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // autotune
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Start PID autotune"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 - 3*LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 5*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("AUTO")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // RETURN
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT - 7*LCD_CHAR_SPACING
                           , BOTTOM_MENU_YPOS
                           , 7*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // Kp
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Proportional coeff."));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 20, PSTR("Kp"));
        float_to_string2(Kp, buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 20
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Ki
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Integral coefficient"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 30, PSTR("Ki"));
        float_to_string2(unscalePID_i(Ki), buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 30
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Kd
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Derivative coeff."));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 40, PSTR("Kd"));
        float_to_string2(unscalePID_d(Kd), buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 40
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

#if (TEMP_SENSOR_BED != 0) || (EXTRUDERS > 1)
static void lcd_menu_tempcontrol_e1()
#else
void lcd_menu_tempcontrol_e1()
#endif
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_temp_e1_menuoption, 6);

    uint8_t flags = 0;
    for (uint8_t index=0; index<6; ++index) {
        menu.drawSubMenu(drawTempExtr1Submenu, index, flags);
    }

    lcd_lib_draw_string_leftP(20, PSTR("Extruder"));
#if EXTRUDERS > 1
    lcd_lib_draw_string_leftP(30, PSTR("(1)"));
#endif // EXTRUDERS

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("PID settings"));
    }

    lcd_lib_update_screen();
}

#if EXTRUDERS > 1
static void init_tempcontrol_e2()
{
    FLOAT_SETTING(1) = unscalePID_i(pid2[1]);
    FLOAT_SETTING(2) = unscalePID_d(pid2[2]);
    // menu.set_selection(1);
}


static void lcd_preset_e2_kd()
{
    if (lcd_tune_value(FLOAT_SETTING(2), 0.0f, 999.99f, 0.01f))
    {
        pid2[2] = scalePID_d(FLOAT_SETTING(2));
    }
}

static void lcd_preset_e2_ki()
{
    if (lcd_tune_value(FLOAT_SETTING(1), 0.0f, 999.99f, 0.01f))
    {
        pid2[1] = scalePID_i(FLOAT_SETTING(1));
    }
}

static void lcd_preset_e2_kp()
{
    lcd_tune_value(pid2[0], 0.0f, 999.99f, 0.01f);
}

static const menu_t & get_temp_e2_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_pid2);
    }
    else if (nr == index++)
    {
        // AUTOTUNE
        opt.setData(MENU_NORMAL, autotune_params_e2);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // PID proportional coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_e2_kp, 4);

    }
    else if (nr == index++)
    {
        // PID integral coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_e2_ki, 4);

    }
    else if (nr == index++)
    {
        // PID derivative coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_e2_kd, 4);

    }
    return opt;
}

static void drawTempExtr2Submenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store PID settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // autotune
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Start PID autotune"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 - 3*LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 5*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("AUTO")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // RETURN
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT - 7*LCD_CHAR_SPACING
                           , BOTTOM_MENU_YPOS
                           , 7*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // Kp
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Proportional coeff."));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 20, PSTR("Kp"));
        float_to_string2(pid2[0], buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 20
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Ki
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Integral coefficient"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 30, PSTR("Ki"));
        float_to_string2(FLOAT_SETTING(1), buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 30
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Kd
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Derivative coeff."));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 40, PSTR("Kd"));
        float_to_string2(FLOAT_SETTING(2), buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 40
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

static void lcd_menu_tempcontrol_e2()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_temp_e2_menuoption, 6);

    uint8_t flags = 0;
    for (uint8_t index=0; index<6; ++index) {
        menu.drawSubMenu(drawTempExtr2Submenu, index, flags);
    }

    lcd_lib_draw_string_leftP(20, PSTR("Extruder"));
    lcd_lib_draw_string_leftP(30, PSTR("(2)"));

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("PID settings"));
    }

    lcd_lib_update_screen();
}
#endif

#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
static void init_tempcontrol_bed()
{
    FLOAT_SETTING(1) = unscalePID_i(bedKi);
    FLOAT_SETTING(2) = unscalePID_d(bedKd);
    menu.set_selection(3);
}

static void lcd_store_pidbed()
{
    float pidBed[3];
    pidBed[0] = bedKp;
    pidBed[1] = bedKi;
    pidBed[2] = bedKd;
    eeprom_write_block(pidBed, (uint8_t*)EEPROM_PID_BED, sizeof(pidBed));

    SET_CONTROL_FLAGS(control_flags);
    menu.return_to_previous();
}

static void lcd_toggle_pid_bed()
{
    if (pidTempBed())
    {
        control_flags &= ~FLAG_PID_BED;
        menu.set_selection(2);
    }
    else
    {
        control_flags |= FLAG_PID_BED;
        menu.set_selection(3);
    }
}

static void lcd_preset_bed_kd()
{
    if (lcd_tune_value(FLOAT_SETTING(2), 0.0f, 999.99f, 0.01f))
    {
        bedKd = scalePID_d(FLOAT_SETTING(2));
    }
}

static void lcd_preset_bed_ki()
{
    if (lcd_tune_value(FLOAT_SETTING(1), 0.0f, 999.99f, 0.01f))
    {
        bedKi = scalePID_i(FLOAT_SETTING(1));
    }
}

static void lcd_preset_bed_kp()
{
    lcd_tune_value(bedKp, 0.0f, 999.99f, 0.01f);
}

static const menu_t & get_temp_bed_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t index(0);
    if ((nr > 0) && !pidTempBed())
    {
        ++nr;
    }
    if (nr == index++)
    {
        // STORE
        opt.setData(MENU_NORMAL, lcd_store_pidbed);
    }
    else if (nr == index++)
    {
        // AUTOTUNE
        opt.setData(MENU_NORMAL, autotune_params_bed);
    }
    else if (nr == index++)
    {
        // RETURN
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // PID mode
        opt.setData(MENU_NORMAL, lcd_toggle_pid_bed);
    }
    else if (nr == index++)
    {
        // PID proportional coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_bed_kp, 4);

    }
    else if (nr == index++)
    {
        // PID integral coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_bed_ki, 4);

    }
    else if (nr == index++)
    {
        // PID derivative coefficient
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_bed_kd, 4);

    }
    return opt;
}

static void drawTempBedSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if ((nr > 0) && !pidTempBed())
    {
        ++nr;
    }
    char buffer[32] = {0};
    if (nr == index++)
    {
        // Store
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Store PID settings"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , BOTTOM_MENU_YPOS
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("STORE")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // autotune
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Start PID autotune"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 - 3*LCD_CHAR_SPACING
                                , BOTTOM_MENU_YPOS
                                , 5*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , PSTR("AUTO")
                                , ALIGN_CENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // RETURN
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT - 7*LCD_CHAR_SPACING
                           , BOTTOM_MENU_YPOS
                           , 7*LCD_CHAR_SPACING
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(5, PSTR("Click to return"));
            flags |= MENU_STATUSLINE;
            lcd_lib_clear_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 4*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
    else if (nr == index++)
    {
        // PID mode
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("PID mode"));
            flags |= MENU_STATUSLINE;
        }
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT
                                , 20-LCD_CHAR_HEIGHT/2
                                , 6*LCD_CHAR_SPACING
                                , 3*LCD_CHAR_HEIGHT
                                , PSTR("Build-|plate")
                                , ALIGN_LEFT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Kp
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Proportional coeff."));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 20, PSTR("Kp"));
        float_to_string2(bedKp, buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 20
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Ki
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Integral coefficient"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 30, PSTR("Ki"));
        float_to_string2(FLOAT_SETTING(1), buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 30
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
    else if (nr == index++)
    {
        // Kd
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("Derivative coeff."));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 40, PSTR("Kd"));
        float_to_string2(FLOAT_SETTING(2), buffer, NULL);

        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - 6*LCD_CHAR_SPACING
                                , 40
                                , 6*LCD_CHAR_SPACING
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

static void lcd_menu_tempcontrol_bed()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);


    uint8_t nCount = 3;
    if (pidTempBed()) {
        nCount += 4;
    }
    else
    {
        lcd_lib_draw_stringP(LCD_GFX_WIDTH/2, 20, PSTR("PID off"));
    }

    menu.process_submenu(get_temp_bed_menuoption, nCount);

    uint8_t flags = 0;
    for (uint8_t index=0; index<nCount; ++index)
    {
        menu.drawSubMenu(drawTempBedSubmenu, index, flags);
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("PID settings"));
    }

    lcd_lib_update_screen();
}
#endif

#if (TEMP_SENSOR_BED != 0) && (EXTRUDERS > 1)
static void lcd_tempcontrol_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    uint8_t index(0);
    char buffer[32] = {0};
    if (nr == index++)
        strcpy_P(buffer, PSTR("< RETURN"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Extruder 1"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Extruder 2"));
    else if (nr == index++)
        strcpy_P(buffer, PSTR("Buildplate"));
    else
        strcpy_P(buffer, PSTR("???"));

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_tempcontrol_details(uint8_t nr)
{
}

void lcd_menu_tempcontrol()
{
    lcd_scroll_menu(PSTR("Temperature control"), 1 + BED_MENU_OFFSET + EXTRUDERS, lcd_tempcontrol_item, lcd_tempcontrol_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
            menu.return_to_previous();
        else if (IS_SELECTED_SCROLL(1))
            menu.add_menu(menu_t(init_tempcontrol_e1, lcd_menu_tempcontrol_e1, NULL));
        else if (IS_SELECTED_SCROLL(2))
            menu.add_menu(menu_t(init_tempcontrol_e2, lcd_menu_tempcontrol_e2, NULL));
        else if (IS_SELECTED_SCROLL(EXTRUDERS+1))
            menu.add_menu(menu_t(init_tempcontrol_bed, lcd_menu_tempcontrol_bed, NULL));
    }
    lcd_lib_update_screen();
}

#elif (TEMP_SENSOR_BED != 0)

void lcd_menu_tempcontrol()
{
    lcd_tripple_menu(PSTR("EXTRUDER"), PSTR("BUILD-|PLATE"), PSTR("RETURN"));
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_MAIN(0))
            menu.add_menu(menu_t(init_tempcontrol_e1, lcd_menu_tempcontrol_e1, NULL));
        else if (IS_SELECTED_MAIN(1))
            menu.add_menu(menu_t(init_tempcontrol_bed, lcd_menu_tempcontrol_bed, NULL));
        else
            menu.return_to_previous();
    }
    lcd_lib_update_screen();
}

#elif EXTRUDERS > 1
void lcd_menu_tempcontrol()
{
    lcd_tripple_menu(PSTR("EXTRUDER|1"), PSTR("EXTRUDER|2"), PSTR("RETURN"));
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_MAIN(0))
            menu.add_menu(menu_t(init_tempcontrol_e1, lcd_menu_tempcontrol_e1, NULL));
        else if (IS_SELECTED_MAIN(1))
            menu.add_menu(menu_t(init_tempcontrol_e2, lcd_menu_tempcontrol_e2, NULL));
        else
            menu.return_to_previous();
    }
    lcd_lib_update_screen();
}
#endif

#endif//ENABLE_ULTILCD2
