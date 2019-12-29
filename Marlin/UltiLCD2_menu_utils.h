#ifndef ULTILCD2_MENU_UTILS_H
#define ULTILCD2_MENU_UTILS_H

#include "UltiLCD2_low_lib.h"
#include "UltiLCD2_menu_print.h"

// menu item flags
#define MENU_NORMAL 0
#define MENU_INPLACE_EDIT 1
#define MENU_SCROLL_ITEM 2
#define MENU_SELECTED 4
#define MENU_ACTIVE   8
#define MENU_STATUSLINE   16

// text alignment
#define ALIGN_TOP 1
#define ALIGN_BOTTOM 2
#define ALIGN_VCENTER 4
#define ALIGN_LEFT 8
#define ALIGN_RIGHT 16
#define ALIGN_HCENTER 32
#define ALIGN_CENTER 36


#define ENCODER_TICKS_PER_MAIN_MENU_ITEM 8
#define ENCODER_TICKS_PER_SCROLL_MENU_ITEM 4
#define ENCODER_NO_SELECTION (ENCODER_TICKS_PER_MAIN_MENU_ITEM * -15)
#define MAIN_MENU_ITEM_POS(n)  (ENCODER_TICKS_PER_MAIN_MENU_ITEM * (n) + ENCODER_TICKS_PER_MAIN_MENU_ITEM / 2)
#define SCROLL_MENU_ITEM_POS(n)  (ENCODER_TICKS_PER_SCROLL_MENU_ITEM * (n) + ENCODER_TICKS_PER_SCROLL_MENU_ITEM / 2)
#define SELECT_MAIN_MENU_ITEM(n)  do { lcd_lib_encoder_pos = MAIN_MENU_ITEM_POS(n); } while(0)
#define SELECT_SCROLL_MENU_ITEM(n)  do { lcd_lib_encoder_pos = SCROLL_MENU_ITEM_POS(n); } while(0)
#define SELECTED_MAIN_MENU_ITEM() (lcd_lib_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM)
#define SELECTED_SCROLL_MENU_ITEM() (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM)
#define IS_SELECTED_MAIN(n) ((n) == SELECTED_MAIN_MENU_ITEM())
#define IS_SELECTED_SCROLL(n) ((n) == SELECTED_SCROLL_MENU_ITEM())

#define UNIT_FLOW "mm\x1D/s"
#define UNIT_SPEED "mm/s"

// --------------------------------------------------------------------------
// menu stack handling
// --------------------------------------------------------------------------
#define MAX_MENU_DEPTH 6

typedef void (*menuFunc_t)();

struct menu_t {
    menuFunc_t  initMenuFunc;
    menuFunc_t  postMenuFunc;
    menuFunc_t  processMenuFunc;
    int16_t     encoderPos;
    uint8_t     max_encoder_acceleration;
    uint8_t     flags;

    // constructor
    menu_t(menuFunc_t func=0, int16_t pos=ENCODER_NO_SELECTION, uint8_t accel=0, uint8_t fl=MENU_NORMAL)
     : initMenuFunc(0)
     , postMenuFunc(0)
     , processMenuFunc(func)
     , encoderPos(pos)
     , max_encoder_acceleration(accel)
     , flags(fl) {}
    // constructor
    menu_t(menuFunc_t initFunc, menuFunc_t eventFunc, menuFunc_t postFunc, int16_t pos=ENCODER_NO_SELECTION, uint8_t accel=0, uint8_t fl=MENU_NORMAL)
     : initMenuFunc(initFunc)
     , postMenuFunc(postFunc)
     , processMenuFunc(eventFunc)
     , encoderPos(pos)
     , max_encoder_acceleration(accel)
     , flags(fl) {}

    void setData(uint8_t fl, menuFunc_t func=0, uint8_t accel=0, int16_t pos=ENCODER_NO_SELECTION)
    {
        initMenuFunc = 0;
        postMenuFunc = 0;
        processMenuFunc = func;
        encoderPos = pos;
        max_encoder_acceleration = accel;
        flags = fl;
    }
    void setData(uint8_t fl, menuFunc_t initFunc, menuFunc_t eventFunc, menuFunc_t postFunc, uint8_t accel=0, int16_t pos=ENCODER_NO_SELECTION)
    {
        initMenuFunc = initFunc;
        postMenuFunc = postFunc;
        processMenuFunc = eventFunc;
        encoderPos = pos;
        max_encoder_acceleration = accel;
        flags = fl;
    }
};

typedef char* (*entryNameCallback_t)(uint8_t nr, char *buffer);
typedef void (*entryDetailsCallback_t)(uint8_t nr);
typedef const menu_t & (*menuItemCallback_t) (uint8_t nr, menu_t &opt);
typedef void (*menuDrawCallback_t) (uint8_t nr, uint8_t &flags);
typedef void (*scrollDrawCallback_t) (uint8_t nr, uint8_t offsetY, uint8_t flags);

class LCDMenu
{
public:
    // constructor
    LCDMenu() : currentIndex(0), selectedSubmenu(-1) {}

    void processEvents();

    void init_menu(menu_t nextMenu, bool beep = false);
    void add_menu(menu_t nextMenu, bool beep = true);
    void replace_menu(menu_t nextMenu, bool beep = true);
    bool return_to_previous(bool beep = true);
    void return_to_menu(menuFunc_t func, bool beep = true);
    void removeMenu(menuFunc_t func);

    void return_to_main(bool beep = true) { return_to_menu(NULL, beep); }

    const menu_t & currentMenu() const { return menuStack[currentIndex]; }
    menu_t & currentMenu() { return menuStack[currentIndex]; }

    const uint8_t encoder_acceleration_factor() const
    {
        return (activeSubmenu.processMenuFunc ? activeSubmenu.max_encoder_acceleration : menuStack[currentIndex].max_encoder_acceleration);
    }

    void process_submenu(menuItemCallback_t getMenuItem, uint8_t len);
    void reset_submenu();
    void set_selection(int8_t index);
    void set_active(menuItemCallback_t getMenuItem, int8_t index);
    void drawSubMenu(menuDrawCallback_t drawFunc, uint8_t nr, uint8_t &flags);
    void drawSubMenu(menuDrawCallback_t drawFunc, uint8_t nr);
    FORCE_INLINE bool isSubmenuSelected() const { return (selectedSubmenu >= 0); }
    FORCE_INLINE bool isSubmenuActive() { return ((selectedSubmenu >= 0) && activeSubmenu.processMenuFunc); }
    FORCE_INLINE bool isSelected(uint8_t nr) { return (selectedSubmenu == nr); }
    FORCE_INLINE bool isActive(uint8_t nr) { return ((selectedSubmenu == nr) && activeSubmenu.processMenuFunc); }

    // standard drawing functions (for convenience)
    static void drawMenuBox(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t flags);
    static void drawMenuString(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char * str, uint8_t textAlign, uint8_t flags);
    static void drawMenuString_P(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char * str, uint8_t textAlign, uint8_t flags);
//    static void reset_selection();

private:
    void enterMenu();
    void exitMenu();

    // menu stack
    menu_t menuStack[MAX_MENU_DEPTH];
    uint8_t currentIndex;

    // submenu item
    menu_t activeSubmenu;
    int8_t selectedSubmenu;

};

extern LCDMenu menu;

FORCE_INLINE void lcd_change_to_previous_menu() { menu.return_to_previous(); }
FORCE_INLINE void lcd_return_to_main_menu() { menu.return_to_main(); }
FORCE_INLINE void lcd_remove_menu() { menu.return_to_previous(false); }
FORCE_INLINE void lcd_select_first_submenu() { menu.set_selection(0); }
FORCE_INLINE void lcd_reset_submenu() { menu.reset_submenu(); }

bool lcd_tune_byte(uint8_t &value, uint8_t _min, uint8_t _max);
bool lcd_tune_speed(float &value, float _min, float _max);

bool lcd_tune_value(uint8_t &value, uint8_t _min, uint8_t _max);
bool lcd_tune_value(int &value, int _min, int _max);
bool lcd_tune_value(float &value, float _min, float _max, float _step);
bool lcd_tune_value(uint16_t &value, uint16_t _min, uint16_t _max);

char* float_to_string1(float f, char* temp_buffer, const char* p_postfix);
char* int_to_time_min(unsigned long i, char* temp_buffer);

void lcd_progressline(uint8_t progress);
void lcd_lib_draw_bargraph( uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, float value );
void lcd_lib_draw_heater(uint8_t x, uint8_t y, uint8_t heaterPower);

#endif
