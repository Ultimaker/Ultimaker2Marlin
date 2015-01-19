
#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2_menu_utils.h"
#include "StackList.h"
#include "UltiLCD2_hi_lib.h"

#define LCD_TIMEOUT_TO_STATUS (MILLISECONDS_PER_SECOND*30UL)		// 30 Sec.

// colors for the encoder led ring
#define LED_INPUT() lcd_lib_led_color(192+led_glow/4, 8+led_glow/4, 0)

// menu stack
static StackList<menu_t> menuStack;

// variables for the menu processing
int8_t selectedMenuItem = -1;
menuActionCallback_t currentMenuFunc = 0;

static int16_t lastEncoderPos = 0;
static int16_t prevEncoderPos = 0;
static menuitem_t cachedItem;

// --------------------------------------------------------------------------
// menu stack handling
// --------------------------------------------------------------------------
static void lcd_init_menu(bool beep)
{
    minProgress = 0;
    led_glow = led_glow_dir = 0;
    LED_NORMAL();
    if (beep)
    {
        lcd_lib_beep();
    }
}

void lcd_add_menu(menuFunc_t nextMenu, int16_t newEncoderPos)
{
    lcd_init_menu(false);
    menuStack.push(menu_t(nextMenu, newEncoderPos, max_encoder_acceleration));
}

void lcd_replace_menu(menuFunc_t nextMenu)
{
    lcd_init_menu(true);
    menuStack.peek().menuFunc = nextMenu;
    menuStack.peek().max_encoder_acceleration = max_encoder_acceleration;
}

void lcd_replace_menu(menuFunc_t nextMenu, int16_t newEncoderPos)
{
    lcd_init_menu(true);
    menuStack.peek().menuFunc = nextMenu;
    menuStack.peek().encoderPos = newEncoderPos;
    menuStack.peek().max_encoder_acceleration = max_encoder_acceleration;
    lcd_lib_encoder_pos = newEncoderPos;
}

void lcd_change_to_menu(menuFunc_t nextMenu, int16_t newEncoderPos, int16_t oldEncoderPos)
{
    lcd_init_menu(true);
    menuStack.peek().encoderPos = oldEncoderPos;
    lcd_add_menu(nextMenu, newEncoderPos);
    lcd_lib_encoder_pos = newEncoderPos;
}

void lcd_change_to_previous_menu()
{
    lcd_init_menu(true);
    if (menuStack.count()>1)
    {
        menuStack.pop();
    }
    lcd_lib_encoder_pos = menuStack.peek().encoderPos;
    max_encoder_acceleration = menuStack.peek().max_encoder_acceleration;
}

void lcd_remove_menu()
{
    // go one step back (without "beep")
    lcd_init_menu(false);
    if (menuStack.count()>1)
    {
        menuStack.pop();
    }
    lcd_lib_encoder_pos = menuStack.peek().encoderPos;
    max_encoder_acceleration = menuStack.peek().max_encoder_acceleration;
}

menu_t & currentMenu()
{
    return menuStack.peek();
}



// --------------------------------------------------------------------------
// menu processing
// --------------------------------------------------------------------------
void lcd_reset_menu()
{
    // reset selection
    lastEncoderPos = lcd_lib_encoder_pos = prevEncoderPos;
    max_encoder_acceleration = 1;
    if (selectedMenuItem >= 0)
    {
        selectedMenuItem = -1;
        currentMenuFunc = 0;
        LED_NORMAL();
    }
}

const menuitem_t & lcd_draw_menu_item(menuItemCallback_t getMenuItem, uint8_t nr)
{
    getMenuItem(nr, cachedItem);
    if (isActive(nr))
    {
        // draw frame
        lcd_lib_draw_box(cachedItem.left-2, cachedItem.top-2,
                         cachedItem.left+cachedItem.width, cachedItem.top+cachedItem.height+1);
    } else if (isSelected(nr))
    {
        // draw box
        lcd_lib_draw_box(cachedItem.left-2, cachedItem.top-1,
                         cachedItem.left+cachedItem.width, cachedItem.top+cachedItem.height);
        lcd_lib_set(cachedItem.left-1, cachedItem.top,
                    cachedItem.left+cachedItem.width-1, cachedItem.top+cachedItem.height-1);
    }

    const char* split = strchr(cachedItem.title, '|');

    if (split)
    {
        char buf[10];
        strncpy(buf, cachedItem.title, split - cachedItem.title);
        buf[split - cachedItem.title] = '\0';
        ++split;

        uint8_t textX1;
        uint8_t textY1;
        uint8_t textX2;
        uint8_t textY2;

        // calculate text position
        if (cachedItem.textalign & ALIGN_LEFT)
        {
            textX1 = textX2 = cachedItem.left;
        }
        else if (cachedItem.textalign & ALIGN_RIGHT)
        {
            textX1 = cachedItem.left + cachedItem.width - (LCD_CHAR_SPACING * strlen(buf));
            textX2 = cachedItem.left + cachedItem.width - (LCD_CHAR_SPACING * strlen(split));
        }
        else // if (cachedItem.textalign & ALIGN_HCENTER)
        {
            textX1 = cachedItem.left + cachedItem.width/2 - (LCD_CHAR_SPACING/2 * strlen(buf));
            textX2 = cachedItem.left + cachedItem.width/2 - (LCD_CHAR_SPACING/2 * strlen(split));
        }

        if (cachedItem.textalign & ALIGN_TOP)
        {
            textY1 = cachedItem.top;
            textY2 = cachedItem.top + LCD_LINE_HEIGHT;
        }
        else if (cachedItem.textalign & ALIGN_BOTTOM)
        {
            textY2 = cachedItem.top + cachedItem.height - LCD_CHAR_HEIGHT;
            textY1 = textY2 - LCD_LINE_HEIGHT;
        }
        else // if (cachedItem.textalign & ALIGN_VCENTER)
        {
            textY1 = cachedItem.top + cachedItem.height/2 - LCD_LINE_HEIGHT/2 - LCD_CHAR_HEIGHT/2;
            textY2 = textY1 + LCD_LINE_HEIGHT;
        }

        if (isSelected(nr) && !currentMenuFunc)
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

        if (cachedItem.textalign & ALIGN_LEFT)
            textX = cachedItem.left;
        else if (cachedItem.textalign & ALIGN_RIGHT)
            textX = cachedItem.left + cachedItem.width - (LCD_CHAR_SPACING * strlen(cachedItem.title));
        else // if (cachedItem.textalign & ALIGN_HCENTER)
            textX = cachedItem.left + cachedItem.width/2 - (LCD_CHAR_SPACING/2 * strlen(cachedItem.title));

        if (cachedItem.textalign & ALIGN_TOP)
            textY = cachedItem.top;
        else if (cachedItem.textalign & ALIGN_BOTTOM)
            textY = cachedItem.top + cachedItem.height - LCD_CHAR_HEIGHT;
        else // if (cachedItem.textalign & ALIGN_VCENTER)
            textY = cachedItem.top + cachedItem.height/2 - LCD_CHAR_HEIGHT/2;


        if (isSelected(nr) && !currentMenuFunc)
        {
            lcd_lib_clear_string(textX, textY, cachedItem.title);
        } else {
            lcd_lib_draw_string(textX, textY, cachedItem.title);
        }
    }
    return cachedItem;
}

void lcd_process_menu(menuItemCallback_t getMenuItem, uint8_t len)
{
    if ((lcd_lib_encoder_pos != lastEncoderPos) || lcd_lib_button_pressed)
    {
        // reset timeout value
        lastEncoderPos = lcd_lib_encoder_pos;

        if (!currentMenuFunc && (lcd_lib_encoder_pos != ENCODER_NO_SELECTION))
        {
            // adjust encoder position
            if (lcd_lib_encoder_pos < 0)
                lcd_lib_encoder_pos += len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
            if (lcd_lib_encoder_pos >= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
                lcd_lib_encoder_pos -= len*ENCODER_TICKS_PER_MAIN_MENU_ITEM;

            currentMenuFunc = 0;
            selectedMenuItem = -1;

            // determine new selection
            int16_t index = (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM);

            if ((index >= 0) && (index < len))
            {
                selectedMenuItem = index;
                prevEncoderPos = lcd_lib_encoder_pos;

                if (lcd_lib_button_pressed)
                {
                    lcd_lib_button_pressed = false;
                    getMenuItem(index, cachedItem);
                    if (cachedItem.initFunc)
                    {
                        cachedItem.initFunc();
                    }
                    if (cachedItem.flags & MENU_INPLACE_EDIT)
                    {
                        // "instant tuning"
                        lcd_lib_encoder_pos = 0;
                        LED_INPUT();
                        lcd_lib_beep();
                        max_encoder_acceleration = cachedItem.max_encoder_acceleration;
                        currentMenuFunc = cachedItem.callbackFunc;
                    }else{
                        // process standard menu item
                        if (cachedItem.callbackFunc)
                        {
                            cachedItem.callbackFunc();
                        }
                        selectedMenuItem = -1;
                        currentMenuFunc = 0;
                        max_encoder_acceleration = 1;
                    }
                }
            }
        }
    }
    else if (selectedMenuItem >= 0)
    {
        if (millis() - last_user_interaction > LCD_TIMEOUT_TO_STATUS)
        {
            // timeout occurred - reset selection
            lcd_reset_menu();
        }
    }
    // process active item
    if ((selectedMenuItem >= 0) && currentMenuFunc)
    {
        if (lcd_lib_button_pressed)
        {
            lcd_lib_beep();
            lcd_reset_menu();
        }else
        {
            currentMenuFunc();
            lastEncoderPos = lcd_lib_encoder_pos;
        }
    }
}


#endif
