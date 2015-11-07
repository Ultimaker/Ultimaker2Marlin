#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "Marlin.h"
#include "ConfigurationStore.h"
#include "planner.h"
#include "stepper.h"
#include "preferences.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_utils.h"
#include "UltiLCD2_menu_prefs.h"

// we use the lcd_cache memory to keep previous values in mind
// #define FLOAT_SETTING(n) (*(float*)&lcd_cache[(n) * sizeof(float)])
// #define INT_SETTING(n) (*(int*)&lcd_cache[(n) * sizeof(int)])

uint8_t ui_mode = UI_MODE_EXPERT;
uint16_t lcd_timeout = LED_DIM_TIME;
uint8_t lcd_contrast = 0xDF;
uint8_t lcd_sleep_contrast = 0;
uint8_t led_sleep_glow = 0;
uint8_t expert_flags = FLAG_PID_NOZZLE;
float end_of_print_retraction = END_OF_PRINT_RETRACTION;
uint8_t heater_check_temp = MAX_HEATING_TEMPERATURE_INCREASE;
uint8_t heater_check_time = MAX_HEATING_CHECK_MILLIS / 1000;

uint16_t led_timeout = LED_DIM_TIME;
uint8_t led_sleep_brightness = 0;

void lcd_store_expertflags()
{
    SET_EXPERT_FLAGS(expert_flags);
    uint16_t version = GET_EXPERT_VERSION()+1;
    if (version < 2)
    {
        SET_EXPERT_VERSION(1);
    }
}

static void lcd_store_sleeptimer()
{
    SET_LED_TIMEOUT(led_timeout);
    SET_LCD_TIMEOUT(lcd_timeout);
    SET_SLEEP_BRIGHTNESS(led_sleep_brightness);
    SET_SLEEP_CONTRAST(lcd_sleep_contrast);
    SET_SLEEP_GLOW(led_sleep_glow);

    uint16_t version = GET_EXPERT_VERSION()+1;
    if (version < 1)
    {
        SET_EXPERT_VERSION(0);
    }
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
    uint16_t version = GET_EXPERT_VERSION()+1;
    if (version < 4)
    {
        SET_EXPERT_VERSION(3);
    }
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

static void lcd_store_steps()
{
    Config_StoreSettings();
    menu.return_to_previous();
}

static void lcd_steps_x()
{
    lcd_tune_value(axis_steps_per_unit[X_AXIS], 0.0f, 9999.0f, 0.1f);
}

static void lcd_steps_y()
{
    lcd_tune_value(axis_steps_per_unit[Y_AXIS], 0.0f, 9999.0f, 0.1f);
}

static void lcd_steps_z()
{
    lcd_tune_value(axis_steps_per_unit[Z_AXIS], 0.0f, 9999.0f, 0.1f);
}

static void lcd_steps_e()
{
    lcd_tune_value(axis_steps_per_unit[E_AXIS], 0.0f, 9999.0f, 0.1f);
}

// create menu options for "print area"
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
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == index++)
    {
        // x steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_x, 3);
    }
    else if (nr == index++)
    {
        // y steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_y, 3);
    }
    else if (nr == index++)
    {
        // z steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_z, 3);
    }
    else if (nr == index++)
    {
        // e steps
        opt.setData(MENU_INPLACE_EDIT, lcd_steps_e, 3);
    }
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
    else if (nr == index++)
    {
        // x steps
        if ((flags & MENU_ACTIVE) | (flags & MENU_SELECTED))
        {
            lcd_lib_draw_string_leftP(5, PSTR("X steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        float_to_string1(axis_steps_per_unit[X_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                                , 20
                                , LCD_CHAR_SPACING*6
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
        float_to_string1(axis_steps_per_unit[Y_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+LCD_CHAR_SPACING*2
                                , 32
                                , LCD_CHAR_SPACING*6
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
            lcd_lib_draw_string_leftP(5, PSTR("Z steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        float_to_string1(axis_steps_per_unit[Z_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)
                                , 20
                                , LCD_CHAR_SPACING*6
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
            lcd_lib_draw_string_leftP(5, PSTR("E steps/mm"));
            flags |= MENU_STATUSLINE;
        }
        float_to_string1(axis_steps_per_unit[E_AXIS], buffer, NULL);
        LCDMenu::drawMenuString(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*6)
                                , 32
                                , LCD_CHAR_SPACING*6
                                , LCD_CHAR_HEIGHT
                                , buffer
                                , ALIGN_RIGHT | ALIGN_VCENTER
                                , flags);
    }
}

void lcd_menu_steps()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_steps_menuoption, 6);

    lcd_lib_draw_string_leftP(20, PSTR("X"));
    lcd_lib_draw_string_leftP(32, PSTR("Y"));
    lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*8), 20, PSTR("Z"));
    lcd_lib_draw_stringP(LCD_GFX_WIDTH - LCD_CHAR_MARGIN_RIGHT - (LCD_CHAR_SPACING*8), 32, PSTR("E"));

    uint8_t flags = 0;
    for (uint8_t index=0; index<6; ++index) {
        menu.drawSubMenu(drawStepsSubmenu, index, flags);
    }
    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(5, PSTR("Axis steps/mm"));
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

    uint16_t version = GET_EXPERT_VERSION()+1;
    if (version < 5)
    {
        SET_EXPERT_VERSION(4);
    }
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
    for (uint8_t i=0; i<3; ++i)
    {
        digipot_current(i, motor_current_setting[i]);
    }
    Config_StoreSettings();
    menu.return_to_previous();
}

static void lcd_preset_current_xy()
{
    lcd_tune_value(motor_current_setting[0], 0, 1300);
}

static void lcd_preset_current_z()
{
    lcd_tune_value(motor_current_setting[1], 0, 1300);
}

static void lcd_preset_current_e()
{
    lcd_tune_value(motor_current_setting[2], 0, 1300);
}

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
        // E motor current
        opt.setData(MENU_INPLACE_EDIT, lcd_preset_current_e, 2);
    }
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
            lcd_lib_draw_string_leftP(5, PSTR("X/Y motor current"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(17, PSTR("X/Y"));
        // lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 15, PSTR("L"));
        int_to_string(motor_current_setting[0], buffer, PSTR("mA"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+4*LCD_CHAR_SPACING
                              , 17
                              , 6*LCD_CHAR_SPACING
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
            lcd_lib_draw_string_leftP(5, PSTR("Z motor current"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(28, PSTR("Z"));
        // lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 15, PSTR("L"));
        int_to_string(motor_current_setting[1], buffer, PSTR("mA"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+4*LCD_CHAR_SPACING
                              , 28
                              , 6*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
    else if (nr == index++)
    {
        // E motor current
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(5, PSTR("E motor current"));
            flags |= MENU_STATUSLINE;
        }
        lcd_lib_draw_string_leftP(39, PSTR("E"));
        // lcd_lib_draw_stringP(LCD_GFX_WIDTH - 2*LCD_CHAR_MARGIN_RIGHT - 8*LCD_CHAR_SPACING, 15, PSTR("L"));
        int_to_string(motor_current_setting[2], buffer, PSTR("mA"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+4*LCD_CHAR_SPACING
                              , 39
                              , 6*LCD_CHAR_SPACING
                              , LCD_CHAR_HEIGHT
                              , buffer
                              , ALIGN_RIGHT | ALIGN_VCENTER
                              , flags);
    }
}

void lcd_menu_motorcurrent()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 13);

    menu.process_submenu(get_current_menuoption, 5);

    uint8_t flags = 0;
    for (uint8_t index=0; index<5; ++index) {
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
    lcd_store_expertflags();
    menu.return_to_previous();
}

static void lcd_swap_extruders()
{
    expert_flags ^= FLAG_SWAP_EXTRUDERS;
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

#endif
static void lcd_store_heatercheck()
{
    SET_HEATER_TIMEOUT(heater_timeout);
    SET_HEATER_CHECK_TEMP(heater_check_temp);
    SET_HEATER_CHECK_TIME(heater_check_time);
    uint16_t version = GET_EXPERT_VERSION()+1;
    if (version < 6)
    {
        SET_EXPERT_VERSION(5);
    }
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



#endif//ENABLE_ULTILCD2
