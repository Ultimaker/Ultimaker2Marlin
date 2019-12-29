
#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "preferences.h"
#include "UltiLCD2_menu_utils.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2.h"
#include "cardreader.h"

#define LCD_TIMEOUT_TO_STATUS (MILLISECONDS_PER_SECOND*30UL)		// 30 Sec.

// colors for the encoder led ring
#define LED_INPUT lcd_lib_led_color(192, 8, 0); led_update();

LCDMenu menu;

static int16_t lastEncoderPos = 0;

static void init_menu_switch(bool beep)
{
    if (last_user_interaction) last_user_interaction = millis();
    minProgress = 0;
    LED_NORMAL
    if (beep)
    {
        lcd_lib_keyclick();
    }
    if (!(sleep_state & SLEEP_LED_OFF) && (led_mode == LED_MODE_ALWAYS_ON))
    {
        analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
    }
}

// --------------------------------------------------------------------------
// menu stack handling
// --------------------------------------------------------------------------

void LCDMenu::processEvents()
{
    // check printing state
    if ((printing_state != PRINT_STATE_HEATING) &&
        (printing_state != PRINT_STATE_START) &&
        ((!card.sdprinting() && !HAS_SERIAL_CMD && !commands_queued()) || card.pause()))
    {
        // cool down nozzle after timeout
        check_heater_timeout();
    }
    if (menuStack[currentIndex].processMenuFunc)
    {
        menuStack[currentIndex].processMenuFunc();
    }
}

void LCDMenu::enterMenu()
{
    lastEncoderPos = lcd_lib_encoder_pos = menuStack[currentIndex].encoderPos;
    lcd_lib_button_pressed = false;
    if (menuStack[currentIndex].initMenuFunc)
    {
        menuStack[currentIndex].initMenuFunc();
    }
}

void LCDMenu::exitMenu()
{
    // post processing
    if (menuStack[currentIndex].postMenuFunc)
    {
        menuStack[currentIndex].postMenuFunc();
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
    exitMenu();
    // switch to new menu
    init_menu_switch(beep);
    menuStack[currentIndex] = nextMenu;
    enterMenu();
}

bool LCDMenu::return_to_previous(bool beep)
{
    LED_NORMAL
    if (currentIndex>0)
    {
        // post processing
        reset_submenu();
        init_menu_switch(beep);
        exitMenu();
        // switch back to previous menu
        --currentIndex;
        enterMenu();
        return true;
    }
    return false;
}

void LCDMenu::return_to_menu(menuFunc_t func, bool beep)
{
    if ((currentIndex>0) && (menuStack[currentIndex].processMenuFunc != func))
    {
        reset_submenu();
        init_menu_switch(beep);
        while ((currentIndex>0) && (menuStack[currentIndex].processMenuFunc != func))
        {
            // post processing
            exitMenu();
            // switch back to previous menu
            --currentIndex;
        }
        // initialize the new menu
        enterMenu();
    }
    LED_NORMAL
}

void LCDMenu::removeMenu(menuFunc_t func)
{
    if (currentIndex>0)
    {
        reset_submenu();
        while (currentIndex>0)
        {
            // post processing
            exitMenu();
            // switch back to previous menu
            --currentIndex;
            // abort loop if the requested menu is removed
            if (menuStack[currentIndex+1].processMenuFunc == func)
            {
                break;
            }
        }
        enterMenu();
    }
    LED_NORMAL
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
        LED_NORMAL
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
    char buffer[32] = {0};
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
    char buffer[32] = {0};
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
                        LED_INPUT
                        lcd_lib_keyclick();

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
            lcd_lib_keyclick();
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

//void LCDMenu::reset_selection()
//{
//    lastEncoderPos = lcd_lib_encoder_pos = ENCODER_NO_SELECTION;
//    lcd_lib_button_pressed = lcd_lib_button_down = false;
//}

void LCDMenu::set_selection(int8_t index)
{
    lcd_lib_encoder_pos = MAIN_MENU_ITEM_POS(index);
    lastEncoderPos = lcd_lib_encoder_pos-1;
}

void LCDMenu::set_active(menuItemCallback_t getMenuItem, int8_t index)
{
    currentMenu().encoderPos = MAIN_MENU_ITEM_POS(index);
    lastEncoderPos = lcd_lib_encoder_pos = 0;
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
        LED_INPUT
        selectedSubmenu = index;
        activeSubmenu = menuItem;
    }
    else
    {
        // process standard menu item
        if (menuItem.processMenuFunc)
        {
            menuItem.processMenuFunc();
        }
        activeSubmenu = menu_t();
        if (menuItem.flags & MENU_NORMAL)
            selectedSubmenu = -1;
    }
}

bool lcd_tune_byte(uint8_t &value, uint8_t _min, uint8_t _max)
{
    int temp_value = int((float(value)*_max/255)+0.5);
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        lcd_lib_tick();
        temp_value += lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM;
        temp_value = constrain(temp_value, _min, _max);
        value = uint8_t((float(temp_value)*255/_max)+0.5);
        lcd_lib_encoder_pos = 0;
        return true;
    }
    return false;
}

bool lcd_tune_speed(float &value, float _min, float _max)
{
    if (lcd_lib_encoder_pos != 0)
    {
        lcd_lib_tick();
        value = constrain(value + (lcd_lib_encoder_pos * 60), _min, _max);
        lcd_lib_encoder_pos = 0;
        return true;
    }
    return false;
}

bool lcd_tune_value(int &value, int _min, int _max)
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        lcd_lib_tick();
        value = constrain(value + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM), _min, _max);
        lcd_lib_encoder_pos = 0;
        return true;
    }
    return false;
}

bool lcd_tune_value(uint16_t &value, uint16_t _min, uint16_t _max)
{
    int iValue = value;
    if (lcd_tune_value(iValue, _min, _max))
    {
        value = iValue;
        return true;
    }
    return false;
}

bool lcd_tune_value(uint8_t &value, uint8_t _min, uint8_t _max)
{
    int iValue = value;
    if (lcd_tune_value(iValue, _min, _max))
    {
        value = iValue;
        return true;
    }
    return false;
}

bool lcd_tune_value(float &value, float _min, float _max, float _step)
{
    if (lcd_lib_encoder_pos != 0)
    {
        lcd_lib_tick();
        value = constrain(round((value + (lcd_lib_encoder_pos * _step))/_step)*_step, _min, _max);
        lcd_lib_encoder_pos = 0;
        return true;
    }
    return false;
}

char* float_to_string1(float f, char* temp_buffer, const char* p_postfix)
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
    if (p_postfix)
    {
        strcpy_P(c, p_postfix);
        c += strlen_P(p_postfix);
    }
    *c = '\0';
    return c;
}

char* int_to_time_min(unsigned long i, char* temp_buffer)
{
    char* c = temp_buffer;
    uint16_t hours = constrain(i / 60 / 60, 0, 999);
    uint8_t mins = (i / 60) % 60;
    uint8_t secs = i % 60;

    if (!hours & !mins)
    {
        *c++ = '0';
        *c++ = '0';
        *c++ = ':';
        *c++ = '0' + secs / 10;
        *c++ = '0' + secs % 10;
    }
    else
    {
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

void lcd_progressline(uint8_t progress)
{
    progress = constrain(progress, 0, 124);
    if (progress)
    {
        lcd_lib_set(LCD_GFX_WIDTH-2, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), LCD_GFX_WIDTH-1, LCD_GFX_HEIGHT-1);
        lcd_lib_set(0, min(LCD_GFX_HEIGHT-1, LCD_GFX_HEIGHT - (progress*LCD_GFX_HEIGHT/124)), 1, LCD_GFX_HEIGHT-1);
    }
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

#endif
