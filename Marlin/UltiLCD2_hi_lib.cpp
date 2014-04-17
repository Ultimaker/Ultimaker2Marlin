#include <avr/pgmspace.h>
#include <string.h>

#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2_hi_lib.h"

menuFunc_t currentMenu;
menuFunc_t previousMenu;
menuFunc_t postMenuCheck;
int16_t previousEncoderPos;
uint8_t led_glow = 0;
uint8_t led_glow_dir;
uint8_t minProgress;

const char* lcd_setting_name;
const char* lcd_setting_postfix;
void* lcd_setting_ptr;
uint8_t lcd_setting_type;
int16_t lcd_setting_min;
int16_t lcd_setting_max;

void lcd_change_to_menu(menuFunc_t nextMenu, int16_t newEncoderPos)
{
    minProgress = 0;
    led_glow = led_glow_dir = 0;
    LED_NORMAL();
    lcd_lib_beep();
    previousMenu = currentMenu;
    previousEncoderPos = lcd_lib_encoder_pos;
    currentMenu = nextMenu;
    lcd_lib_encoder_pos = newEncoderPos;
}

void lcd_tripple_menu(const char* left, const char* right, const char* bottom)
{
    if (lcd_lib_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_lib_encoder_pos < 0)
            lcd_lib_encoder_pos += 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_lib_encoder_pos >= 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_lib_encoder_pos -= 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }

    lcd_lib_clear();
    lcd_lib_draw_vline(64, 5, 45);
    lcd_lib_draw_hline(3, 124, 48);

    if (IS_SELECTED_MAIN(0))
    {
        lcd_lib_draw_box(3+2, 5+2, 64-3-2, 45-2);
        lcd_lib_set(3+3, 5+3, 64-3-3, 45-3);
        lcd_lib_clear_string_center_atP(33, 22, left);
    }else{
        lcd_lib_draw_string_center_atP(33, 22, left);
    }
    
    if (IS_SELECTED_MAIN(1))
    {
        lcd_lib_draw_box(64+3+2, 5+2, 125-2, 45-2);
        lcd_lib_set(64+3+3, 5+3, 125-3, 45-3);
        lcd_lib_clear_string_center_atP(64 + 33, 22, right);
    }else{
        lcd_lib_draw_string_center_atP(64 + 33, 22, right);
    }
    
    if (bottom != NULL)
    {
        if (IS_SELECTED_MAIN(2))
        {
            lcd_lib_draw_box(3+2, 49+2, 125-2, 63-2);
            lcd_lib_set(3+3, 49+3, 125-3, 63-3);
            lcd_lib_clear_string_centerP(53, bottom);
        }else{
            lcd_lib_draw_string_centerP(53, bottom);
        }
    }
}

void lcd_basic_screen()
{
    lcd_lib_clear();
    lcd_lib_draw_hline(3, 124, 48);
}

void lcd_info_screen(menuFunc_t cancelMenu, menuFunc_t callbackOnCancel, const char* cancelButtonText)
{
    if (lcd_lib_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_lib_encoder_pos < 0)
            lcd_lib_encoder_pos += 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_lib_encoder_pos >= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_lib_encoder_pos -= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }
    if (lcd_lib_button_pressed && IS_SELECTED_MAIN(0))
    {
        if (callbackOnCancel) callbackOnCancel();
        if (cancelMenu) lcd_change_to_menu(cancelMenu);
    }

    lcd_basic_screen();

    if (!cancelButtonText) cancelButtonText = PSTR("CANCEL");
    if (IS_SELECTED_MAIN(0))
    {
        lcd_lib_draw_box(3+2, 49+2, 125-2, 63-2);
        lcd_lib_set(3+3, 49+3, 125-3, 63-3);
        lcd_lib_clear_stringP(65 - strlen_P(cancelButtonText) * 3, 53, cancelButtonText);
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(cancelButtonText) * 3, 53, cancelButtonText);
    }
}

void lcd_question_screen(menuFunc_t optionAMenu, menuFunc_t callbackOnA, const char* AButtonText, menuFunc_t optionBMenu, menuFunc_t callbackOnB, const char* BButtonText)
{
    if (lcd_lib_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_lib_encoder_pos < 0)
            lcd_lib_encoder_pos += 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_lib_encoder_pos >= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_lib_encoder_pos -= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_MAIN(0))
        {
            if (callbackOnA) callbackOnA();
            if (optionAMenu) lcd_change_to_menu(optionAMenu);
        }else if (IS_SELECTED_MAIN(1))
        {
            if (callbackOnB) callbackOnB();
            if (optionBMenu) lcd_change_to_menu(optionBMenu);
        }
    }

    lcd_basic_screen();

    if (IS_SELECTED_MAIN(0))
    {
        lcd_lib_draw_box(3+2, 49+2, 64-2, 63-2);
        lcd_lib_set(3+3, 49+3, 64-3, 63-3);
        lcd_lib_clear_stringP(35 - strlen_P(AButtonText) * 3, 53, AButtonText);
    }else{
        lcd_lib_draw_stringP(35 - strlen_P(AButtonText) * 3, 53, AButtonText);
    }
    if (IS_SELECTED_MAIN(1))
    {
        lcd_lib_draw_box(64+2, 49+2, 64+60-2, 63-2);
        lcd_lib_set(64+3, 49+3, 64+60-3, 63-3);
        lcd_lib_clear_stringP(64+31 - strlen_P(BButtonText) * 3, 53, BButtonText);
    }else{
        lcd_lib_draw_stringP(64+31 - strlen_P(BButtonText) * 3, 53, BButtonText);
    }
}

void lcd_progressbar(uint8_t progress)
{
    lcd_lib_draw_box(3, 38, 124, 46);
    
    for(uint8_t n=0; n<progress;n++)
    {
        if (n>120) break;
        uint8_t m = (progress-n-1) % 12;
        if (m < 5)
            lcd_lib_draw_vline(4 + n, 40, 40+m);
        else if (m < 10)
            lcd_lib_draw_vline(4 + n, 40+m-5, 44);
    }
}

void lcd_scroll_menu(const char* menuNameP, int8_t entryCount, entryNameCallback_t entryNameCallback, entryDetailsCallback_t entryDetailsCallback)
{
    if (lcd_lib_button_pressed)
		return;//Selection possibly changed the menu, so do not update it this cycle.

    if (lcd_lib_encoder_pos == ENCODER_NO_SELECTION)
        lcd_lib_encoder_pos = 0;

	static int16_t viewPos = 0;
	if (lcd_lib_encoder_pos < 0) lcd_lib_encoder_pos += entryCount * ENCODER_TICKS_PER_SCROLL_MENU_ITEM;
	if (lcd_lib_encoder_pos >= entryCount * ENCODER_TICKS_PER_SCROLL_MENU_ITEM) lcd_lib_encoder_pos -= entryCount * ENCODER_TICKS_PER_SCROLL_MENU_ITEM;

    uint8_t selIndex = uint16_t(lcd_lib_encoder_pos/ENCODER_TICKS_PER_SCROLL_MENU_ITEM);

    lcd_lib_clear();

    int16_t targetViewPos = selIndex * 8 - 15;

    int16_t viewDiff = targetViewPos - viewPos;
    viewPos += viewDiff / 4;
    if (viewDiff > 0) { viewPos ++; led_glow = led_glow_dir = 0; }
    if (viewDiff < 0) { viewPos --; led_glow = led_glow_dir = 0; }

    uint8_t drawOffset = 10 - (uint16_t(viewPos) % 8);
    uint8_t itemOffset = uint16_t(viewPos) / 8;
    for(uint8_t n=0; n<6; n++)
    {
        uint8_t itemIdx = n + itemOffset;
        if (itemIdx >= entryCount)
            continue;

        char* ptr = entryNameCallback(itemIdx);
		//ptr[10] = '\0';
		ptr[20] = '\0';
        if (itemIdx == selIndex)
        {
            //lcd_lib_set(3, drawOffset+8*n-1, 62, drawOffset+8*n+7);
            lcd_lib_set(3, drawOffset+8*n-1, 124, drawOffset+8*n+7);
            lcd_lib_clear_string(4, drawOffset+8*n, ptr);
        }else{
            lcd_lib_draw_string(4, drawOffset+8*n, ptr);
        }
    }
    lcd_lib_set(3, 0, 124, 8);
    lcd_lib_clear(3, 47, 124, 63);
    lcd_lib_clear(3, 9, 124, 9);

    lcd_lib_draw_hline(3, 124, 48);

    lcd_lib_clear_string_centerP(1, menuNameP);
    
	entryDetailsCallback(selIndex);
	
    lcd_lib_update_screen();
}

void lcd_menu_edit_setting()
{
    if (lcd_lib_encoder_pos < lcd_setting_min)
        lcd_lib_encoder_pos = lcd_setting_min;
    if (lcd_lib_encoder_pos > lcd_setting_max)
        lcd_lib_encoder_pos = lcd_setting_max;

    if (lcd_setting_type == 1)
        *(uint8_t*)lcd_setting_ptr = lcd_lib_encoder_pos;
    else if (lcd_setting_type == 2)
        *(uint16_t*)lcd_setting_ptr = lcd_lib_encoder_pos;
    else if (lcd_setting_type == 3)
        *(float*)lcd_setting_ptr = float(lcd_lib_encoder_pos) / 100.0;
    else if (lcd_setting_type == 4)
        *(int32_t*)lcd_setting_ptr = lcd_lib_encoder_pos;
    else if (lcd_setting_type == 5)
        *(uint8_t*)lcd_setting_ptr = lcd_lib_encoder_pos * 255 / 100;
    else if (lcd_setting_type == 6)
        *(float*)lcd_setting_ptr = float(lcd_lib_encoder_pos) * 60;
    else if (lcd_setting_type == 7)
        *(float*)lcd_setting_ptr = float(lcd_lib_encoder_pos) * 100;
    else if (lcd_setting_type == 8)
        *(float*)lcd_setting_ptr = float(lcd_lib_encoder_pos);

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, lcd_setting_name);
    char buffer[16];
    if (lcd_setting_type == 3)
        float_to_string(float(lcd_lib_encoder_pos) / 100.0, buffer, lcd_setting_postfix);
    else
        int_to_string(lcd_lib_encoder_pos, buffer, lcd_setting_postfix);
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();

    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);
}
#endif//ENABLE_ULTILCD2
