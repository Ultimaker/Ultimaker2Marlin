
#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2_menu_utils.h"
#include "UltiLCD2_hi_lib.h"

#define LCD_TIMEOUT_TO_STATUS (MILLISECONDS_PER_SECOND*30UL)		// 30 Sec.

// colors for the encoder led ring
#define LED_INPUT() lcd_lib_led_color(192, 8, 0)

LCDMenu menu;

static int16_t lastEncoderPos = 0;

// --------------------------------------------------------------------------
// menu stack handling
// --------------------------------------------------------------------------

void LCDMenu::init_menu_switch(bool beep)
{
    minProgress = 0;
    LED_NORMAL();
    if (beep)
    {
        lcd_lib_beep();
    }
}

void LCDMenu::init_menu(menu_t mainMenu, bool beep)
{
    currentIndex = 0;
    menuStack[currentIndex].postMenuFunc = 0;
    replace_menu(mainMenu, beep);
}

void LCDMenu::add_menu(menu_t nextMenu, bool beep)
{
    if (currentIndex < MAX_MENU_DEPTH-1)
    {
        // preserve current encoder position
        menuStack[currentIndex].encoderPos = lcd_lib_encoder_pos;
        reset_submenu();
        // add new menu to stack
        ++currentIndex;
        menuStack[currentIndex].postMenuFunc = 0;
        replace_menu(nextMenu, beep);
    }
}

void LCDMenu::replace_menu(menu_t nextMenu, bool beep)
{
    // post processing
    reset_submenu();
    if (menuStack[currentIndex].postMenuFunc)
    {
        menuStack[currentIndex].postMenuFunc();
    }
    // switch to new menu
    init_menu_switch(beep);
    menuStack[currentIndex] = nextMenu;
    lastEncoderPos = lcd_lib_encoder_pos = nextMenu.encoderPos;
    lcd_lib_button_pressed = false;
    // menu initialization
    if (nextMenu.initMenuFunc)
    {
        nextMenu.initMenuFunc();
    }
}

void LCDMenu::return_to_previous(bool beep)
{
    if (currentIndex>0)
    {
        // post processing
        reset_submenu();
        if (menuStack[currentIndex].postMenuFunc)
        {
            menuStack[currentIndex].postMenuFunc();
        }
        init_menu_switch(beep);
        // switch back to previous menu
        --currentIndex;
        lastEncoderPos = lcd_lib_encoder_pos = menuStack[currentIndex].encoderPos;
        lcd_lib_button_pressed = false;
        if (menuStack[currentIndex].initMenuFunc)
        {
            menuStack[currentIndex].initMenuFunc();
        }
    }
}

void LCDMenu::return_to_main(bool beep)
{
    if (currentIndex>0)
    {
        reset_submenu();
        init_menu_switch(beep);
        while (currentIndex>0)
        {
            // post processing
            if (menuStack[currentIndex].postMenuFunc)
            {
                menuStack[currentIndex].postMenuFunc();
            }
            // switch back to previous menu
            --currentIndex;
        }
        lastEncoderPos = lcd_lib_encoder_pos = menuStack[currentIndex].encoderPos;
        lcd_lib_button_pressed = false;
        if (menuStack[currentIndex].initMenuFunc)
        {
            menuStack[currentIndex].initMenuFunc();
        }
    }
}

// --------------------------------------------------------------------------
// submenu processing
// --------------------------------------------------------------------------
void LCDMenu::reset_submenu()
{
    // reset selection
    if (activeSubmenu.postMenuFunc)
    {
        activeSubmenu.postMenuFunc();
    }
    if (activeSubmenu.processMenuFunc)
    {
        LED_NORMAL();
    }
    lastEncoderPos = lcd_lib_encoder_pos = menuStack[currentIndex].encoderPos;
    activeSubmenu = menu_t();
    selectedSubmenu = -1;
    lcd_lib_button_pressed = false;
}

void LCDMenu::drawMenuBox(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t flags)
{
    if (flags & MENU_ACTIVE)
    {
        // draw frame
        lcd_lib_draw_box(left-2, top-2, left+width, top+height+1);
    } else if (flags & MENU_SELECTED)
    {
        // draw box
        lcd_lib_draw_box(left-2, top-1, left+width, top+height);
        lcd_lib_set(left-1, top, left+width-1, top+height-1);
    }
}

void LCDMenu::drawMenuString(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char * str, uint8_t textAlign, uint8_t flags)
{
    drawMenuBox(left, top, width, height, flags);
    char buffer[32];
    const char* split = strchr(str, '|');

    uint8_t textX1;
    uint8_t textY1;
    uint8_t textX2;
    uint8_t textY2;

    if (split)
    {
        strncpy(buffer, str, split - str);
        buffer[split - str] = '\0';
        ++split;

        // calculate text position
        if (textAlign & ALIGN_LEFT)
        {
            textX1 = textX2 = left;
        }
        else if (textAlign & ALIGN_RIGHT)
        {
            textX1 = left + width - (LCD_CHAR_SPACING * strlen(buffer));
            textX2 = left + width - (LCD_CHAR_SPACING * strlen(split));
        }
        else // if (textAlign & ALIGN_HCENTER)
        {
            textX1 = left + width/2 - (LCD_CHAR_SPACING/2 * strlen(buffer));
            textX2 = left + width/2 - (LCD_CHAR_SPACING/2 * strlen(split));
        }

        if (textAlign & ALIGN_TOP)
        {
            textY1 = top;
            textY2 = top + LCD_LINE_HEIGHT;
        }
        else if (textAlign & ALIGN_BOTTOM)
        {
            textY2 = top + height - LCD_CHAR_HEIGHT;
            textY1 = textY2 - LCD_LINE_HEIGHT;
        }
        else // if (textAlign & ALIGN_VCENTER)
        {
            textY1 = top + height/2 - LCD_LINE_HEIGHT/2 - LCD_CHAR_HEIGHT/2;
            textY2 = textY1 + LCD_LINE_HEIGHT;
        }

        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_string(textX1, textY1, buffer);
            lcd_lib_clear_string(textX2, textY2, split);
        } else {
            lcd_lib_draw_string(textX1, textY1, buffer);
            lcd_lib_draw_string(textX2, textY2, split);
        }
    }else{
        // calculate text position
        if (textAlign & ALIGN_LEFT)
            textX1 = left;
        else if (textAlign & ALIGN_RIGHT)
            textX1 = left + width - (LCD_CHAR_SPACING * strlen(str));
        else // if (textAlign & ALIGN_HCENTER)
            textX1 = left + width/2 - (LCD_CHAR_SPACING/2 * strlen(str));

        if (textAlign & ALIGN_TOP)
            textY1 = top;
        else if (textAlign & ALIGN_BOTTOM)
            textY1 = top + height - LCD_CHAR_HEIGHT;
        else // if (textAlign & ALIGN_VCENTER)
            textY1 = top + height/2 - LCD_CHAR_HEIGHT/2;


        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_string(textX1, textY1, str);
        } else {
            lcd_lib_draw_string(textX1, textY1, str);
        }
    }
}

void LCDMenu::drawMenuString_P(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char * str, uint8_t textAlign, uint8_t flags)
{
    char buffer[32];
    strcpy_P(buffer, str);
    drawMenuString(left, top, width, height, buffer, textAlign, flags);
}

void LCDMenu::process_submenu(menuItemCallback_t getMenuItem, uint8_t len)
{
    if ((lcd_lib_encoder_pos != lastEncoderPos) || lcd_lib_button_pressed)
    {
        lastEncoderPos = lcd_lib_encoder_pos;

        if (!activeSubmenu.processMenuFunc && (lcd_lib_encoder_pos != ENCODER_NO_SELECTION))
        {
            // adjust encoder position
            if (lcd_lib_encoder_pos < 0)
                lcd_lib_encoder_pos += len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
            if (lcd_lib_encoder_pos >= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
                lcd_lib_encoder_pos -= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;

            // determine new selection
            int16_t index = (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM);

            if ((index >= 0) && (index < len))
            {
                currentMenu().encoderPos = lcd_lib_encoder_pos;

                if (lcd_lib_button_pressed)
                {
                    lcd_lib_button_pressed = false;
                    menu_t menuItem;
                    getMenuItem(index, menuItem);
                    if (menuItem.initMenuFunc)
                    {
                        menuItem.initMenuFunc();
                    }
                    if (menuItem.flags & MENU_INPLACE_EDIT)
                    {
                        // "instant tuning"
                        lcd_lib_encoder_pos = 0;
                        LED_INPUT();
                        lcd_lib_beep();

                        selectedSubmenu = index;
                        activeSubmenu = menuItem;
                    }
                    else
                    {
                        // process standard menu item
                        if ((selectedSubmenu == index) && menuItem.processMenuFunc)
                        {
                            menuItem.processMenuFunc();
                        }
                        activeSubmenu = menu_t();
                        if (menuItem.flags & MENU_NORMAL)
                            selectedSubmenu = -1;
                    }
                }else {
                    selectedSubmenu = index;
                }
            }
            else
            {
                selectedSubmenu = -1;
            }
        }
    }
    else if (isSubmenuSelected() && millis() - last_user_interaction > LCD_TIMEOUT_TO_STATUS)
    {
        // timeout occurred - reset selection
        reset_submenu();
    }
    // process active item
    if (isSubmenuSelected() && activeSubmenu.processMenuFunc)
    {
        if (lcd_lib_button_pressed)
        {
            lcd_lib_beep();
            reset_submenu();
        }
        else
        {
            activeSubmenu.processMenuFunc();
            lastEncoderPos = lcd_lib_encoder_pos;
        }
    }
}

void LCDMenu::drawSubMenu(menuDrawCallback_t drawFunc, uint8_t nr, uint8_t &flags)
{
    if (drawFunc)
    {
        // reset flags
        flags &= ~(MENU_ACTIVE | MENU_SELECTED);
        if ((activeSubmenu.processMenuFunc) && (selectedSubmenu == nr))
        {
            flags |= MENU_ACTIVE;
        }
        else if (selectedSubmenu == nr)
        {
            flags |= MENU_SELECTED;
        }
        drawFunc(nr, flags);
    }
}

void LCDMenu::drawSubMenu(menuDrawCallback_t drawFunc, uint8_t nr)
{
    uint8_t flags = 0;
    drawSubMenu(drawFunc, nr, flags);
}

void LCDMenu::reset_selection()
{
    lastEncoderPos = lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
    lcd_lib_button_pressed = lcd_lib_button_down = false;
}

void LCDMenu::set_selection(int8_t index)
{
    lcd_lib_encoder_pos = MAIN_MENU_ITEM_POS(index);
    lastEncoderPos = lcd_lib_encoder_pos-1;
}

#endif
