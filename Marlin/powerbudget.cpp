#include "Configuration.h"
#include "Marlin.h"
#include "powerbudget.h"

#ifdef ENABLE_ULTILCD2
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_utils.h"
#endif

uint16_t power_budget=DEFAULT_POWER_BUDGET;
uint16_t power_buildplate=DEFAULT_POWER_BUILDPLATE;
uint16_t power_extruder[EXTRUDERS]=ARRAY_BY_EXTRUDERS(DEFAULT_POWER_EXTRUDER, DEFAULT_POWER_EXTRUDER, DEFAULT_POWER_EXTRUDER);


/////

#ifdef EEPROM_SETTINGS

//Random number to verify that the settings are already written to the EEPROM
#define EEPROM_POWER_MAGIC 0x1E23D465

#define POWER_POSTFIX             'r'

#define EEPROM_POWER_START        0x03FC  //  4 Byte Magic number
#define EEPROM_POWER_VERSION      0x03FA  //  2 Byte Version
#define EEPROM_POWER_BUDGET       0x03F8  //  2 Byte budget
#define EEPROM_POWER_BUILDPATE    0x03F6  //  2 Byte buildplate
#define EEPROM_POWER_EXTRUDER     0x03F2  //  4 Byte 2x extruder
#define EEPROM_POWER_POSTFIX      0x03F1  //  1 Byte "r"

// IMPORTANT:  Whenever there are changes made to the variables stored in EEPROM
// in the functions below, also increment the version number. This makes sure that
// the default values are used whenever there is a change to the data, to prevent
// wrong data being written to the variables.
#define STORE_POWER_VERSION 0


static bool PowerBudget_RetrieveVersion(uint16_t &version)
{
    uint32_t magic = eeprom_read_dword((uint32_t*)(EEPROM_POWER_START));
    char postfix = eeprom_read_byte((const uint8_t*)EEPROM_POWER_POSTFIX);
    if ((magic == EEPROM_POWER_MAGIC) && (postfix == POWER_POSTFIX))
    {
        version = eeprom_read_word((const uint16_t*)EEPROM_POWER_VERSION);
        return true;
    }
    return false;
}

//void PowerBudget_ClearStorage()
//{
//    // invalidate data
//    eeprom_write_dword((uint32_t*)(EEPROM_WATTAGE_START), 0);
//}

void PowerBudget_RetrieveSettings()
{
    uint16_t version;
    // check storage
    bool bValid = PowerBudget_RetrieveVersion(version);
    if (bValid)
    {
        power_budget     = eeprom_read_word((const uint16_t*)EEPROM_POWER_BUDGET);
        power_buildplate = eeprom_read_word((const uint16_t*)EEPROM_POWER_BUILDPATE);

        // read extruder wattage
        uint16_t tmp_array[2];
        eeprom_read_block(tmp_array, (uint8_t*)EEPROM_POWER_EXTRUDER, sizeof(tmp_array));
        power_extruder[0] = tmp_array[0];
      #if EXTRUDERS > 1
        for (uint8_t e=1; e<EXTRUDERS; ++e)
        {
            power_extruder[e] = tmp_array[1];
        }
      #endif
    }
    // don't let any of these variables equal zero or we will get divide by zero errors
    if (power_budget==0) power_budget=DEFAULT_POWER_BUDGET;
    if (power_buildplate==0) power_buildplate=DEFAULT_POWER_BUILDPLATE;
    for (uint8_t e=0; e<EXTRUDERS; ++e)
    {
        if (power_extruder[e]==0) power_extruder[e] = DEFAULT_POWER_EXTRUDER;
    }
}

static void PowerBudget_SaveSettings()
{
    uint16_t version;
    // check storage
    bool bValid = PowerBudget_RetrieveVersion(version);

    // write values to EEPROM
    eeprom_write_word((uint16_t*)EEPROM_POWER_BUDGET, power_budget);
    eeprom_write_word((uint16_t*)EEPROM_POWER_BUILDPATE, power_buildplate);

    uint16_t tmp_array[2];
    tmp_array[0] = power_extruder[0];
  #if EXTRUDERS > 1
    tmp_array[1] = power_extruder[1];
  #else
    tmp_array[1] = power_extruder[0];
  #endif
    eeprom_write_block(tmp_array, (uint8_t*)EEPROM_POWER_EXTRUDER, sizeof(tmp_array));

    if (!bValid)
    {
        // validate stored data
        // version = STORE_WATTAGE_VERSION;
        eeprom_write_word((uint16_t*)EEPROM_POWER_VERSION, STORE_POWER_VERSION);
        eeprom_write_dword((uint32_t*)(EEPROM_POWER_START), EEPROM_POWER_MAGIC);
        eeprom_write_byte((uint8_t*)EEPROM_POWER_POSTFIX, POWER_POSTFIX);
    }

}

  #define STORE_MENU_OFFSET   1
#else
  void PowerBudget_RetrieveSettings() {}
  #define STORE_MENU_OFFSET   0
#endif // EEPROM_SETTINGS


#ifdef ENABLE_ULTILCD2

#define WATTAGE_MENU_LEN   STORE_MENU_OFFSET+BED_MENU_OFFSET+EXTRUDERS+2


static void lcd_powerbudget_store()
{
    PowerBudget_SaveSettings();
    menu.return_to_previous();
}

static void lcd_budget()
{
    lcd_tune_value(power_budget, POWER_MINVALUE, POWER_MAXVALUE);
}

#if (TEMP_SENSOR_BED != 0)
static void lcd_power_buildplate()
{
    lcd_tune_value(power_buildplate, POWER_MINVALUE, POWER_MAXVALUE);
}
#endif

static void lcd_power_extruder0()
{
    lcd_tune_value(power_extruder[0], POWER_MINVALUE, POWER_MAXVALUE);
}

#if (EXTRUDERS > 1)
static void lcd_power_extruder1()
{
    lcd_tune_value(power_extruder[1], POWER_MINVALUE, POWER_MAXVALUE);
}
#endif

static const menu_t & get_powerbudget_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;

#if STORE_MENU_OFFSET > 0
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_powerbudget_store);
    }
    else if (nr == menu_index++)
#else
    if (nr == menu_index++)
#endif // STORE_MENU_OFFSET
    {
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_budget, 4);
    }
#if (TEMP_SENSOR_BED != 0)
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_power_buildplate, 4);
    }
#endif
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_power_extruder0, 4);
    }
#if (EXTRUDERS > 1)
    else if (nr == menu_index++)
    {
        opt.setData(MENU_INPLACE_EDIT, lcd_power_extruder1, 4);
    }
#endif
    return opt;
}

static void drawPowerBudgetSubmenu (uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    char buffer[32] = {0};

    const uint8_t ylineoffset = 15 - (2*BED_MENU_OFFSET) - (2*EXTRUDERS);

#if STORE_MENU_OFFSET > 0
    if (nr == index++)
    {
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(4, PSTR("Store options"));
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
#else
    if (nr == index++)
#endif
    {
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_MARGIN_LEFT
                           , BOTTOM_MENU_YPOS
                           , 52
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_draw_string_leftP(4, PSTR("Click to return"));
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
        // Wattage Budget
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(4, PSTR("Total Power Budget"));
            flags |= MENU_STATUSLINE;
        }

        uint8_t ypos = (18 - BED_MENU_OFFSET - EXTRUDERS) + ((nr-2)*ylineoffset);
        lcd_lib_draw_string_leftP(ypos, PSTR("Total"));
        int_to_string(power_budget, buffer, PSTR("W"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+11*LCD_CHAR_SPACING
                          , ypos
                          , 4*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#if (TEMP_SENSOR_BED != 0)
    else if (nr == index++)
    {
        // Wattage Buildplate
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(4, PSTR("Wattage Buildplate"));
            flags |= MENU_STATUSLINE;
        }

        uint8_t ypos = (18 - BED_MENU_OFFSET - EXTRUDERS) + ((nr-2)*ylineoffset);
        lcd_lib_draw_string_leftP(ypos, PSTR("Buildplate"));
        int_to_string(power_buildplate, buffer, PSTR("W"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+11*LCD_CHAR_SPACING
                          , ypos
                          , 4*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#endif
    else if (nr == index++)
    {
        // Wattage Extruder 0
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(4, PSTR("Wattage Extruder"));
          #if EXTRUDERS > 1
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+17*LCD_CHAR_SPACING, 4, PSTR("1"));
          #endif
            flags |= MENU_STATUSLINE;
        }

        uint8_t ypos = (18 - BED_MENU_OFFSET - EXTRUDERS) + ((nr-2)*ylineoffset);
        lcd_lib_draw_string_leftP(ypos, PSTR("Extruder"));
      #if EXTRUDERS > 1
        lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+9*LCD_CHAR_SPACING, ypos, PSTR("1"));
      #endif
        int_to_string(power_extruder[0], buffer, PSTR("W"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+11*LCD_CHAR_SPACING
                          , ypos
                          , 4*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#if EXTRUDERS > 1
    else if (nr == index++)
    {
        // Wattage Extruder 1
        if (flags & (MENU_SELECTED | MENU_ACTIVE))
        {
            lcd_lib_draw_string_leftP(4, PSTR("Wattage Extruder"));
          #if EXTRUDERS > 1
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT+17*LCD_CHAR_SPACING, 4, PSTR("2"));
          #endif
            flags |= MENU_STATUSLINE;
        }

        uint8_t ypos = (18 - BED_MENU_OFFSET - EXTRUDERS) + ((nr-2)*ylineoffset);
        lcd_lib_draw_string_leftP(ypos, PSTR("Extruder 2"));
        int_to_string(power_extruder[1], buffer, PSTR("W"));
        LCDMenu::drawMenuString(LCD_CHAR_MARGIN_LEFT+11*LCD_CHAR_SPACING
                          , ypos
                          , 4*LCD_CHAR_SPACING
                          , LCD_CHAR_HEIGHT
                          , buffer
                          , ALIGN_RIGHT | ALIGN_VCENTER
                          , flags);
    }
#endif
}

void lcd_menu_powerbudget()
{
    lcd_basic_screen();
    lcd_lib_draw_hline(3, 124, 12);

    menu.process_submenu(get_powerbudget_menuoption, WATTAGE_MENU_LEN);

    uint8_t flags = 0;
    for (uint8_t index=0; index<WATTAGE_MENU_LEN; ++index) {
        menu.drawSubMenu(drawPowerBudgetSubmenu, index, flags);
    }

    if (!(flags & MENU_STATUSLINE))
    {
        lcd_lib_draw_string_leftP(4, PSTR("Power Budget"));
    }

    lcd_lib_update_screen();
}

#endif // ENABLE_ULTILCD2
