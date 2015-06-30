#include <avr/pgmspace.h>
#include <string.h>

#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_utils.h"

menuFunc_t postMenuCheck;
uint8_t minProgress;

const char* lcd_setting_name;
const char* lcd_setting_postfix;
void* lcd_setting_ptr;
uint8_t lcd_setting_type;
int16_t lcd_setting_min;
int16_t lcd_setting_max;


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
    lcd_lib_draw_vline(64, 5, 46);
    lcd_lib_draw_hline(3, 124, 50);

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
            lcd_lib_draw_box(3+2, BOTTOM_MENU_YPOS-1, 124-2, BOTTOM_MENU_YPOS+7);
            lcd_lib_set(3+3, BOTTOM_MENU_YPOS, 124-3, BOTTOM_MENU_YPOS+6);
            lcd_lib_clear_string_centerP(BOTTOM_MENU_YPOS, bottom);
        }else{
            lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, bottom);
        }
    }
}

void lcd_basic_screen()
{
    lcd_lib_clear();
    lcd_lib_draw_hline(3, 124, 51);
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
        if (cancelMenu) menu.replace_menu(menu_t(cancelMenu));
    }

    lcd_basic_screen();

    if (!cancelButtonText) cancelButtonText = PSTR("CANCEL");
    if (IS_SELECTED_MAIN(0))
    {
        lcd_lib_draw_box(3+2, BOTTOM_MENU_YPOS-1, 124-2, BOTTOM_MENU_YPOS+7);
        lcd_lib_set(3+3, BOTTOM_MENU_YPOS, 124-3, BOTTOM_MENU_YPOS+6);
        lcd_lib_clear_stringP(65 - strlen_P(cancelButtonText) * 3, BOTTOM_MENU_YPOS, cancelButtonText);
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(cancelButtonText) * 3, BOTTOM_MENU_YPOS, cancelButtonText);
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
            if (optionAMenu) menu.add_menu(menu_t(optionAMenu));
        }else if (IS_SELECTED_MAIN(1))
        {
            if (callbackOnB) callbackOnB();
            if (optionBMenu) menu.add_menu(menu_t(optionBMenu));
        }
    }

    lcd_basic_screen();

    if (IS_SELECTED_MAIN(0))
    {
        lcd_lib_draw_box(3+2, BOTTOM_MENU_YPOS-1, 64-2, BOTTOM_MENU_YPOS+7);
        lcd_lib_set(3+3, BOTTOM_MENU_YPOS, 64-3, BOTTOM_MENU_YPOS+6);
        lcd_lib_clear_stringP(35 - strlen_P(AButtonText) * 3, BOTTOM_MENU_YPOS, AButtonText);
    }else{
        lcd_lib_draw_stringP(35 - strlen_P(AButtonText) * 3, BOTTOM_MENU_YPOS, AButtonText);
    }
    if (IS_SELECTED_MAIN(1))
    {
        lcd_lib_draw_box(64+2, BOTTOM_MENU_YPOS-1, 64+60-2, BOTTOM_MENU_YPOS+7);
        lcd_lib_set(64+3, BOTTOM_MENU_YPOS, 64+60-3, BOTTOM_MENU_YPOS+6);
        lcd_lib_clear_stringP(64+31 - strlen_P(BButtonText) * 3, BOTTOM_MENU_YPOS, BButtonText);
    }else{
        lcd_lib_draw_stringP(64+31 - strlen_P(BButtonText) * 3, BOTTOM_MENU_YPOS, BButtonText);
    }
}

void lcd_progressbar(uint8_t progress)
{
    lcd_lib_draw_box(3, 39, 124, 47);

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

void lcd_draw_scroll_entry(uint8_t offsetY, char * buffer, uint8_t flags)
{
    if (flags & MENU_SELECTED)
    {
        //lcd_lib_set(3, drawOffset+8*n-1, 62, drawOffset+8*n+7);
        lcd_lib_set(LCD_CHAR_MARGIN_LEFT-1, offsetY-1, LCD_GFX_WIDTH-LCD_CHAR_MARGIN_RIGHT, offsetY+7);
        lcd_lib_clear_string(LCD_CHAR_MARGIN_LEFT, offsetY, buffer);
    }else{
        lcd_lib_draw_string(LCD_CHAR_MARGIN_LEFT, offsetY, buffer);
    }
}

void lcd_scroll_menu(const char* menuNameP, int8_t entryCount, scrollDrawCallback_t entryDrawCallback, entryDetailsCallback_t entryDetailsCallback)
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
//    if (viewDiff > 0) { viewPos ++; led_glow = led_glow_dir = 0; }
//    if (viewDiff < 0) { viewPos --; led_glow = led_glow_dir = 0; }
    if (viewDiff > 0) { viewPos ++; }
    if (viewDiff < 0) { viewPos --; }

    uint8_t drawOffset = 10 - (uint16_t(viewPos) % 8);
    uint8_t itemOffset = uint16_t(viewPos) / 8;
    for(uint8_t n=0; n<6; n++)
    {
        uint8_t itemIdx = n + itemOffset;
        if (itemIdx >= entryCount)
            continue;

        entryDrawCallback(itemIdx, drawOffset+8*n, (itemIdx == selIndex) ? MENU_SELECTED : 0);

    }
    lcd_lib_set(3, 0, 124, 8);
    lcd_lib_clear(3, 47, 124, 63);
    lcd_lib_clear(3, 9, 124, 9);

    lcd_lib_draw_hline(3, 124, 50);

    lcd_lib_clear_string_centerP(1, menuNameP);

    if (entryDetailsCallback)
    {
        entryDetailsCallback(selIndex);
    }

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
        lcd_change_to_previous_menu();
}
#endif//ENABLE_ULTILCD2
