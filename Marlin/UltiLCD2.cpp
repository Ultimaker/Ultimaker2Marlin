#include "UltiLCD2.h"
#include "UltiLCD2_lib.h"
#include "cardreader.h"
#include "ConfigurationStore.h"
#include "temperature.h"
#include "pins.h"

#define FILAMENT_REVERSAL_LENGTH      750
#define FILAMENT_REVERSAL_SPEED       200
#define FILAMENT_LONG_MOVE_ACCELERATION 30

#define FILAMENT_FORWARD_LENGTH       600
#define FILAMENT_INSERT_SPEED         5     //Initial insert speed to grab the filament.
#define FILAMENT_INSERT_FAST_SPEED    200    //Speed during the forward length
#define FILAMENT_INSERT_EXTRUDE_SPEED 0.5     //Final speed when extruding

#define EEPROM_FIRST_RUN_DONE_OFFSET 0x400
#define IS_FIRST_RUN_DONE() (eeprom_read_byte((const uint8_t*)EEPROM_FIRST_RUN_DONE_OFFSET) == 'U')
#define SET_FIRST_RUN_DONE() do { eeprom_write_byte((uint8_t*)EEPROM_FIRST_RUN_DONE_OFFSET, 'U'); } while(0)

#define EEPROM_MATERIAL_SETTINGS_OFFSET 0x800
#define EEPROM_MATERIAL_SETTINGS_SIZE   8 + 16
#define EEPROM_MATERIAL_COUNT_OFFSET()            ((uint8_t*)EEPROM_MATERIAL_SETTINGS_OFFSET + 0)
#define EEPROM_MATERIAL_NAME_OFFSET(n)            ((uint8_t*)EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * (n))
#define EEPROM_MATERIAL_TEMPERATURE_OFFSET(n)     ((uint16_t*)EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * (n) + 8)
#define EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(n) ((uint16_t*)EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * (n) + 10)
#define EEPROM_MATERIAL_FAN_SPEED_OFFSET(n)       ((uint8_t*)EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * (n) + 12)
#define EEPROM_MATERIAL_FLOW_OFFSET(n)            ((uint16_t*)EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * (n) + 13)
#define EEPROM_MATERIAL_DIAMETER_OFFSET(n)        ((float*)EEPROM_MATERIAL_SETTINGS_OFFSET + 1 + EEPROM_MATERIAL_SETTINGS_SIZE * (n) + 15)


//#ifdef ENABLE_ULTILCD2
typedef void (*menuFunc_t)();

static void lcd_menu_startup();
static void lcd_menu_main();
static void lcd_menu_print_select();
static void lcd_menu_print_heatup();
static void lcd_menu_print_printing();
static void lcd_menu_print_classic_warning();
static void lcd_menu_print_classic();
static void lcd_menu_print_abort();
static void lcd_menu_print_ready();
static void lcd_menu_material();
static void lcd_menu_change_material_preheat();
static void lcd_menu_change_material_remove();
static void lcd_menu_change_material_remove_wait_user();
static void lcd_menu_change_material_insert_wait_user();
static void lcd_menu_change_material_insert_forward();
static void lcd_menu_change_material_insert();
static void lcd_menu_material_select();
static void lcd_menu_material_selected();
static void lcd_menu_material_settings();
static void lcd_menu_maintenance();
static void lcd_menu_maintenance_first_run_initial();
static void lcd_menu_maintenance_first_run_select();
static void lcd_menu_maintenance_first_run_bed_level_start();
static void lcd_menu_maintenance_first_run_bed_level();
static void lcd_menu_maintenance_first_run_bed_level_adjust();
static void lcd_menu_maintenance_first_run_done();
static void lcd_menu_maintenance_advanced();
static void lcd_menu_advanced_factory_reset();
static void lcd_menu_breakout();
static void lcd_menu_TODO();

menuFunc_t currentMenu = lcd_menu_startup;
menuFunc_t previousMenu = lcd_menu_startup;
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

struct materialSettings
{
    int16_t temperature;
    int16_t bed_temperature;
    uint8_t fan_speed; //0-100% of requested speed by GCode
    int16_t flow;      //Flow modification in %
    float diameter; //Filament diameter in mm
};

struct materialSettings material = {210, 65, 100, 100, 2.85};

#define LCD_CACHE_COUNT 6
#define LCD_CACHE_SIZE (1 + (2 + LONG_FILENAME_LENGTH) * LCD_CACHE_COUNT)
uint8_t lcd_cache[LCD_CACHE_SIZE];
#define LCD_CACHE_NR_OF_FILES lcd_cache[(LCD_CACHE_SIZE - 1)]
#define LCD_CACHE_ID(n) lcd_cache[(n)]
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])
#define LCD_CACHE_TYPE(n) lcd_cache[LCD_CACHE_COUNT + (n)]

void lcd_init()
{
    lcd_lib_init();
}

const uint8_t ultimakerTextGfx[] PROGMEM = {
    128, 21, //size
    0x6,0x6,0x6,0x6,0xfe,0xfe,0xfe,0xfe,0xfc,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
    0xfc,0xfe,0xfe,0xfe,0xfc,0x0,0x0,0x0,0xe,0xfe,0xfe,0xfe,0xfe,0x0,0x0,0xe0,
    0xfc,0xfe,0xfe,0xfc,0xe0,0xe0,0x0,0x0,0x0,0xce,0xce,0xce,0x0,0x0,0x0,0xc0,
    0xc0,0xc0,0xc0,0xc0,0xc0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0xc0,0xc0,0x80,
    0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x80,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0xc0,0x80,
    0x80,0x0,0x0,0x0,0x0,0x6,0xfe,0xfe,0xfe,0xfe,0x0,0x0,0x0,0x0,0x80,0xc0,
    0xc0,0xc0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x80,0xc0,0xc0,0xc0,0x80,0x80,0x80,
    0x0,0x0,0x0,0x0,0xc0,0xc0,0xc0,0xc0,0x0,0x80,0xc0,0xc0,0xc0,0xc0,0x80,0x0,
    
    0x0,0x0,0x0,0x0,0x7f,0xff,0xff,0xff,0xff,0x80,0x0,0x0,0x0,0x0,0x0,0x80,
    0xff,0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0xff,0xff,0xff,0xff,0x0,0x0,0x0,
    0xff,0xff,0xff,0xff,0x0,0x0,0x0,0x0,0x0,0xff,0xff,0xff,0x0,0x0,0x0,0x81,
    0xf1,0xff,0xff,0x3f,0xf,0x3f,0xff,0xfc,0xf0,0xf0,0xfc,0xff,0x3f,0xf,0x3f,0xff,
    0xfe,0xf8,0x80,0x0,0x0,0x0,0xc1,0xe1,0xf1,0xf1,0x71,0x31,0x1,0x1,0x83,0xff,
    0xff,0xff,0xfc,0x0,0x0,0x0,0xff,0xff,0xff,0xff,0x0,0xc,0x3e,0xff,0xff,0xf7,
    0xc3,0x1,0x0,0x0,0x0,0xfc,0xfe,0xff,0x87,0x3,0x11,0x31,0x31,0x33,0x3f,0x3f,
    0x3f,0x1e,0x0,0x0,0x1,0xff,0xff,0xff,0xff,0x7,0x3,0x1,0x1,0x1,0x1,0x0,

    0x0,0x0,0x0,0x0,0x0,0x1,0x3,0x7,0x7,0xf,0xf,0xf,0xf,0xf,0xf,0xf,
    0x7,0x7,0x7,0xf,0xf,0x6,0x0,0x0,0x0,0x7,0xf,0xf,0x7,0x0,0x0,0x0,
    0x7,0xf,0xf,0x7,0x0,0x0,0x0,0x0,0x0,0xf,0xf,0xf,0x0,0x0,0x6,0xf,
    0xf,0x7,0x1,0x0,0x0,0x0,0x1,0x7,0xf,0xf,0x7,0x1,0x0,0x0,0x0,0x1,
    0x7,0xf,0xf,0x6,0x0,0x0,0x3,0x7,0xf,0xf,0xe,0xe,0x6,0x7,0x3,0x7,
    0xf,0xf,0x7,0x0,0x0,0x0,0x7,0xf,0xf,0x7,0x0,0x0,0x0,0x0,0x3,0xf,
    0xf,0xf,0x6,0x0,0x0,0x0,0x3,0x7,0x7,0xf,0xe,0xe,0xe,0xe,0xe,0xe,
    0x6,0x0,0x0,0x0,0x0,0xf,0xf,0xf,0xf,0x0,0x0,0x0,0x0,0x0,0x0,0x0
};

const uint8_t ultimakerTextOutlineGfx[] PROGMEM = {
    128, 21,//size

    0x9,0x9,0x9,0xf9,0x1,0x1,0x1,0x1,0x3,0xfe,0x0,0x0,0x0,0x0,0x0,0xfe,
    0x3,0x1,0x1,0x1,0x3,0xfe,0x0,0x1f,0xf1,0x1,0x1,0x1,0x1,0xff,0xf0,0x1e,
    0x3,0x1,0x1,0x3,0x1e,0x10,0xf0,0x0,0xff,0x31,0x31,0x31,0xff,0x0,0xe0,0x20,
    0x20,0x20,0x20,0x20,0x20,0x60,0xc0,0x80,0x0,0x0,0x80,0xc0,0x60,0x20,0x20,0x60,
    0xc0,0x0,0x0,0x0,0x0,0xc0,0x40,0x60,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x60,
    0x40,0xc0,0x80,0x0,0xf,0xf9,0x1,0x1,0x1,0x1,0xff,0x0,0x80,0xc0,0x60,0x20,
    0x20,0x20,0xe0,0x0,0x0,0x0,0x80,0xc0,0x40,0x60,0x20,0x20,0x20,0x60,0x40,0x40,
    0xc0,0x80,0x0,0xe0,0x20,0x20,0x20,0x20,0xe0,0x60,0x20,0x20,0x20,0x20,0x60,0xc0,

    0x0,0x0,0x0,0xff,0x80,0x0,0x0,0x0,0x0,0x7f,0xc0,0x80,0x80,0x80,0xc0,0x7f,
    0x0,0x0,0x0,0x0,0x0,0xff,0x0,0x0,0xff,0x0,0x0,0x0,0x0,0xff,0x1,0xff,
    0x0,0x0,0x0,0x0,0xff,0x1,0x1,0x0,0xff,0x0,0x0,0x0,0xff,0x0,0xc3,0x7a,
    0xe,0x0,0x0,0xc0,0x70,0xc0,0x0,0x3,0xe,0xe,0x3,0x0,0xc0,0x70,0xc0,0x0,
    0x1,0x7,0x7c,0xc0,0x0,0xe3,0x32,0x1a,0xa,0xa,0x8a,0xca,0xfa,0xc6,0x7c,0x0,
    0x0,0x0,0x3,0xfe,0x0,0xff,0x0,0x0,0x0,0x0,0xff,0x73,0xc1,0x0,0x0,0x8,
    0x3c,0xe6,0x83,0x0,0xfe,0x3,0x1,0x0,0x78,0xfc,0xee,0x4a,0x4e,0x4c,0x40,0x40,
    0x40,0x61,0x3f,0x3,0xfe,0x0,0x0,0x0,0x0,0xf8,0xc,0x6,0x2,0x2,0x2,0x3,

    0x0,0x0,0x0,0x0,0x3,0x6,0xc,0x8,0x18,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
    0x18,0x8,0x18,0x10,0x10,0x19,0xf,0x0,0xf,0x18,0x10,0x10,0x18,0xf,0x0,0xf,
    0x18,0x10,0x10,0x18,0xf,0x0,0x0,0x0,0x1f,0x10,0x10,0x10,0x1f,0xf,0x19,0x10,
    0x10,0x18,0xe,0x3,0x0,0x3,0xe,0x18,0x10,0x10,0x18,0xe,0x3,0x0,0x3,0xe,
    0x18,0x10,0x10,0x19,0xf,0x7,0xc,0x18,0x10,0x10,0x11,0x11,0x19,0x8,0xc,0x18,
    0x10,0x10,0x18,0xf,0x0,0xf,0x18,0x10,0x10,0x18,0xf,0x0,0x1,0x7,0x1c,0x10,
    0x10,0x10,0x19,0xf,0x1,0x7,0xc,0x8,0x18,0x10,0x11,0x11,0x11,0x11,0x11,0x11,
    0x19,0xf,0x0,0x0,0x1f,0x10,0x10,0x10,0x10,0x1f,0x0,0x0,0x0,0x0,0x0,0x0,
};

const uint8_t ultimakerRobotGfx[] PROGMEM = {
    47, 64, //Size
    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x80,0x80,0x80,0xc0,
    0xe0,0xfe,0xff,0x19,0x1e,0x98,0x98,0x98,0x18,0x18,0x18,0x18,0x18,0x18,0x9e,0x9f,
    0x99,0x1e,0x38,0xf0,0xe0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,

    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x3c,0xfe,0xff,0xff,0x99,0xf1,0xe1,
    0xc3,0xff,0xff,0x0,0x0,0x63,0x63,0x63,0x60,0x60,0x60,0x60,0x60,0x60,0x63,0x63,
    0x63,0x0,0x0,0xff,0xff,0x7c,0x38,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,

    0x0,0x0,0x0,0x80,0xc0,0x60,0xb0,0xd8,0xd8,0x1c,0xfc,0xff,0x3f,0xf,0x8f,0xcf,
    0xc7,0xc7,0xc7,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,
    0xc6,0xc6,0xc6,0xc7,0xc7,0xc4,0xcc,0xc,0x1c,0xf8,0xf8,0x70,0xc0,0x0,0x0,

    0x0,0xe,0xff,0xf9,0x2a,0x2b,0x2b,0x28,0x28,0xfc,0xff,0xff,0x0,0x0,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xbf,0xb7,0x17,0xb5,0xbd,0xb5,0x0,0xb5,0xb5,0xf7,0x17,
    0xff,0xbf,0xbf,0xff,0xff,0xff,0xff,0x0,0x0,0xff,0xff,0xfc,0xff,0x7,0x0,

    0xc0,0xf0,0xff,0xff,0xfd,0xdd,0x1d,0x19,0x19,0x3f,0xff,0xff,0x0,0x0,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xfb,0xf1,0xfb,0xfb,0xea,0xee,0xc0,0xee,0xea,0xfa,0xf0,
    0xfa,0xfa,0xff,0xff,0xff,0xff,0xff,0x0,0x0,0xff,0xff,0xf,0x1f,0xf8,0xe0,

    0x0,0x3,0x7,0xf,0xf,0x1b,0x18,0x19,0xfb,0xfe,0xff,0xff,0xf0,0xe0,0xc0,0xc1,
    0xe1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf1,0xf9,0xf9,0xf9,0xf9,
    0xf9,0xf9,0x79,0x79,0x79,0xf1,0xf0,0x10,0x18,0x1f,0x7,0x6,0x3,0x3,0x0,

    0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xf8,0xff,0xef,0xdf,0xbf,0x7f,0xff,0xff,0xff,
    0x40,0x41,0x43,0x43,0x7f,0x21,0x20,0x20,0x21,0x7f,0xff,0xff,0xff,0x3f,0xb0,0x91,
    0xd1,0xdf,0xd0,0x50,0x58,0x1f,0x1f,0xf0,0xf0,0x0,0x0,0x0,0x0,0x0,0x0,

    0x0,0x0,0x0,0x0,0x0,0x0,0x3,0x7,0xf,0x1e,0x3d,0x7b,0x77,0xee,0xd1,0xde,
    0xde,0xcf,0x4b,0x4b,0x69,0x69,0x68,0x68,0x68,0x60,0x3f,0x3f,0x37,0x37,0x37,0x35,
    0x34,0x30,0x10,0x10,0x18,0x18,0x1a,0x1b,0xf,0x6,0x0,0x0,0x0,0x0,0x0,
};

const uint8_t folderGfx[] PROGMEM = {
    16, 15,
    0xfc,0xe,0xf,0xcf,0xcf,0xcf,0xce,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xdc,0xc0,0xc0,
    0x7,0x70,0x7e,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0xf,0x1,
};

const uint8_t backArrowGfx[] PROGMEM = {
  28, 24,
  0x0,0x0,0x0,0x0,0x0,0x80,0x80,0xc0,0x40,0x60,0x20,0x30,0x10,0x18,0x8,0xc,
  0x4,0x6,0x2,0x3,0x7f,0x40,0x40,0x40,0x40,0x40,0x40,0xc0,
  0x3c,0x66,0x42,0xc3,0x81,0x81,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
  0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0xff,
  0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x3,0x2,0x6,0x4,0xc,0x8,0x18,0x10,0x30,
  0x20,0x60,0x40,0xc0,0xfe,0x2,0x2,0x2,0x2,0x2,0x2,0x3,
};

#define ENCODER_TICKS_PER_MAIN_MENU_ITEM 4
#define ENCODER_TICKS_PER_FILE_MENU_ITEM 4
#define ENCODER_NO_SELECTION (ENCODER_TICKS_PER_MAIN_MENU_ITEM * -11)
#define MENU_ITEM_POS(n)  (ENCODER_TICKS_PER_MAIN_MENU_ITEM * (n) + ENCODER_TICKS_PER_MAIN_MENU_ITEM / 2)
#define SELECT_MENU_ITEM(n)  do { lcd_encoder_pos = MENU_ITEM_POS(n); } while(0)
#define SELECTED_MENU_ITEM() (lcd_encoder_pos / ENCODER_TICKS_PER_MAIN_MENU_ITEM)
#define IS_SELECTED(n) ((n) == SELECTED_MENU_ITEM())

void lcd_update()
{
    if (!lcd_lib_update_ready()) return;
    lcd_lib_buttons_update();
    
    if (led_glow_dir)
    {
        led_glow-=2;
        if (led_glow == 0) led_glow_dir = 0;
    }else{
        led_glow+=2;
        if (led_glow == 128) led_glow_dir = 1;
    }
    
    if (IsStopped())
    {
        lcd_lib_clear();
        lcd_lib_draw_stringP(15, 10, PSTR("ERROR - STOPPED"));
        switch(StoppedReason())
        {
        case STOP_REASON_MAXTEMP:
        case STOP_REASON_MINTEMP:
            lcd_lib_draw_stringP(15, 20, PSTR("Temp sensor"));
            break;
        case STOP_REASON_MAXTEMP_BED:
            lcd_lib_draw_stringP(15, 20, PSTR("Temp sensor BED"));
            break;
        case STOP_REASON_SAFETY_TRIGGER:
            lcd_lib_draw_stringP(15, 20, PSTR("Safety circuit"));
            break;
        }
        lcd_lib_draw_stringP(1, 40, PSTR("Contact:"));
        lcd_lib_draw_stringP(1, 50, PSTR("support@ultimaker.com"));
        lcd_lib_led_color(led_glow,0,0);
        lcd_lib_update_screen();
    }else{
        currentMenu();
    }
}

void lcd_menu_startup()
{
    lcd_encoder_pos = ENCODER_NO_SELECTION;
    
    lcd_lib_led_color(32,32,40);
    lcd_lib_clear();
    
    if (led_glow < 84)
    {
        lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
        for(uint8_t n=0;n<10;n++)
        {
            if (led_glow*2 >= n + 20)
                lcd_lib_clear(0, 22+n*2, led_glow*2-n-20, 23+n*2);
            if (led_glow*2 >= n)
                lcd_lib_clear(led_glow*2 - n, 22+n*2, 127, 23+n*2);
            else
                lcd_lib_clear(0, 22+n*2, 127, 23+n*2);
        }
    /*
    }else if (led_glow < 86) {
        led_glow--;
        //lcd_lib_set();
        //lcd_lib_clear_gfx(0, 22, ultimakerTextGfx);
        lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
    */
    }else{
        led_glow--;
        lcd_lib_draw_gfx(80, 0, ultimakerRobotGfx);
        lcd_lib_clear_gfx(0, 22, ultimakerTextOutlineGfx);
        lcd_lib_draw_gfx(0, 22, ultimakerTextGfx);
    }
    lcd_lib_update_screen();

    if (led_glow_dir || lcd_lib_button_pressed)
    {
        //led_glow = led_glow_dir = 0;
        //return;
        if (lcd_lib_button_pressed)
            lcd_lib_beep();
        
        if (!IS_FIRST_RUN_DONE())
        {
            currentMenu = lcd_menu_maintenance_first_run_initial;
            SELECT_MENU_ITEM(0);
        }else{
            currentMenu = lcd_menu_main;
        }
    }
}

void lcd_change_to_menu(menuFunc_t nextMenu, int16_t newEncoderPos = ENCODER_NO_SELECTION)
{
    minProgress = 0;
    led_glow = led_glow_dir = 0;
    lcd_lib_led_color(32,32,40);
    lcd_lib_beep();
    previousMenu = currentMenu;
    previousEncoderPos = lcd_encoder_pos;
    currentMenu = nextMenu;
    lcd_encoder_pos = newEncoderPos;
}

void lcd_tripple_menu(const char* left, const char* right, const char* bottom)
{
    if (lcd_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_encoder_pos < 0)
            lcd_encoder_pos += 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_encoder_pos >= 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_encoder_pos -= 3*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }

    lcd_lib_clear();
    lcd_lib_draw_vline(64, 5, 45);
    lcd_lib_draw_hline(3, 124, 48);

    if (IS_SELECTED(0))
    {
        lcd_lib_draw_box(3+2, 5+2, 64-3-2, 45-2);
        lcd_lib_set(3+3, 5+3, 64-3-3, 45-3);
        lcd_lib_clear_stringP(33 - strlen_P(left) * 3, 22, left);
    }else{
        lcd_lib_draw_stringP(33 - strlen_P(left) * 3, 22, left);
    }
    
    if (IS_SELECTED(1))
    {
        lcd_lib_draw_box(64+3+2, 5+2, 124-2, 45-2);
        lcd_lib_set(64+3+3, 5+3, 124-3, 45-3);
        lcd_lib_clear_stringP(64 + 33 - strlen_P(right) * 3, 22, right);
    }else{
        lcd_lib_draw_stringP(64 + 33 - strlen_P(right) * 3, 22, right);
    }
    
    if (IS_SELECTED(2))
    {
        lcd_lib_draw_box(3+2, 49+2, 124-2, 63-2);
        lcd_lib_set(3+3, 49+3, 124-3, 63-3);
        lcd_lib_clear_stringP(65 - strlen_P(bottom) * 3, 53, bottom);
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(bottom) * 3, 53, bottom);
    }
}

static void lcd_info_screen(menuFunc_t cancelMenu, menuFunc_t callbackOnCancel = NULL, const char* cancelButtonText = NULL)
{
    if (lcd_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_encoder_pos < 0)
            lcd_encoder_pos += 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_encoder_pos >= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_encoder_pos -= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }
    if (lcd_lib_button_pressed && IS_SELECTED(0))
    {
        if (callbackOnCancel) callbackOnCancel();
        if (cancelMenu) lcd_change_to_menu(cancelMenu);
    }

    lcd_lib_clear();
    lcd_lib_draw_hline(3, 124, 48);

    if (!cancelButtonText) cancelButtonText = PSTR("CANCEL");
    if (IS_SELECTED(0))
    {
        lcd_lib_draw_box(3+2, 49+2, 124-2, 63-2);
        lcd_lib_set(3+3, 49+3, 124-3, 63-3);
        lcd_lib_clear_stringP(65 - strlen_P(cancelButtonText) * 3, 53, cancelButtonText);
    }else{
        lcd_lib_draw_stringP(65 - strlen_P(cancelButtonText) * 3, 53, cancelButtonText);
    }
}

static void lcd_question_screen(menuFunc_t optionAMenu, menuFunc_t callbackOnA, const char* AButtonText, menuFunc_t optionBMenu, menuFunc_t callbackOnB, const char* BButtonText)
{
    if (lcd_encoder_pos != ENCODER_NO_SELECTION)
    {
        if (lcd_encoder_pos < 0)
            lcd_encoder_pos += 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
        if (lcd_encoder_pos >= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM)
            lcd_encoder_pos -= 2*ENCODER_TICKS_PER_MAIN_MENU_ITEM;
    }
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            if (callbackOnA) callbackOnA();
            if (optionAMenu) lcd_change_to_menu(optionAMenu);
        }else if (IS_SELECTED(1))
        {
            if (callbackOnB) callbackOnB();
            if (optionBMenu) lcd_change_to_menu(optionBMenu);
        }
    }

    lcd_lib_clear();
    lcd_lib_draw_hline(3, 124, 48);

    if (IS_SELECTED(0))
    {
        lcd_lib_draw_box(3+2, 49+2, 64-2, 63-2);
        lcd_lib_set(3+3, 49+3, 64-3, 63-3);
        lcd_lib_clear_stringP(35 - strlen_P(AButtonText) * 3, 53, AButtonText);
    }else{
        lcd_lib_draw_stringP(35 - strlen_P(AButtonText) * 3, 53, AButtonText);
    }
    if (IS_SELECTED(1))
    {
        lcd_lib_draw_box(64+2, 49+2, 64+60-2, 63-2);
        lcd_lib_set(64+3, 49+3, 64+60-3, 63-3);
        lcd_lib_clear_stringP(64+31 - strlen_P(BButtonText) * 3, 53, BButtonText);
    }else{
        lcd_lib_draw_stringP(64+31 - strlen_P(BButtonText) * 3, 53, BButtonText);
    }
}

static void lcd_progressbar(uint8_t progress)
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

static void lcd_scroll_menu(const char* menuNameP, int8_t entryCount, char* (*entryNameCallback)(uint8_t nr), void (*entryDetailsCallback)(uint8_t nr))
{
    if (lcd_lib_button_pressed)
    {
		return;//Selection possibly changed the menu, so do not update it this cycle.
    }
    if (lcd_encoder_pos == ENCODER_NO_SELECTION)
        lcd_encoder_pos = 0;
    
	static int16_t viewPos = 0;
	if (lcd_encoder_pos < 0) lcd_encoder_pos += entryCount * ENCODER_TICKS_PER_FILE_MENU_ITEM;
	if (lcd_encoder_pos >= entryCount * ENCODER_TICKS_PER_FILE_MENU_ITEM) lcd_encoder_pos -= entryCount * ENCODER_TICKS_PER_FILE_MENU_ITEM;

    uint8_t selIndex = uint16_t(lcd_encoder_pos/ENCODER_TICKS_PER_FILE_MENU_ITEM);

    lcd_lib_clear();

    int16_t targetViewPos = selIndex * 8 - 15;

    int16_t viewDiff = targetViewPos - viewPos;
    viewPos += viewDiff / 4;
    if (viewDiff > 0) viewPos ++;
    if (viewDiff < 0) viewPos --;
    
    char selectedName[LONG_FILENAME_LENGTH] = {'\0'};
    uint8_t drawOffset = 10 - (uint16_t(viewPos) % 8);
    uint8_t itemOffset = uint16_t(viewPos) / 8;
    for(uint8_t n=0; n<6; n++)
    {
        uint8_t itemIdx = n + itemOffset;
        if (itemIdx >= entryCount)
            continue;

        char* ptr = entryNameCallback(itemIdx);
		if (itemIdx == selIndex)
			strcpy(selectedName, ptr);
		ptr[10] = '\0';
        if (itemIdx == selIndex)
        {
            lcd_lib_set(3, drawOffset+8*n-1, 62, drawOffset+8*n+7);
            lcd_lib_clear_string(4, drawOffset+8*n, ptr);
        }else{
            lcd_lib_draw_string(4, drawOffset+8*n, ptr);
        }
    }
    selectedName[20] = '\0';

    lcd_lib_set(3, 0, 62, 8);
    lcd_lib_clear(3, 47, 62, 63);
    lcd_lib_clear(3, 9, 62, 9);

    lcd_lib_draw_vline(64, 5, 45);
    lcd_lib_draw_hline(3, 124, 48);

    lcd_lib_clear_stringP(10, 1, menuNameP);
    
    lcd_lib_draw_string(5, 53, selectedName);
	
	entryDetailsCallback(selIndex);
	
    lcd_lib_update_screen();
}

static void lcd_menu_edit_setting()
{
    if (lcd_encoder_pos < lcd_setting_min)
        lcd_encoder_pos = lcd_setting_min;
    if (lcd_encoder_pos > lcd_setting_max)
        lcd_encoder_pos = lcd_setting_max;
    if (lcd_lib_button_pressed)
    {
        if (lcd_setting_type == 1)
            *(uint8_t*)lcd_setting_ptr = lcd_encoder_pos;
        else if (lcd_setting_type == 2)
            *(uint16_t*)lcd_setting_ptr = lcd_encoder_pos;
        else if (lcd_setting_type == 3)
            *(float*)lcd_setting_ptr = float(lcd_encoder_pos) / 100.0;
        lcd_change_to_menu(previousMenu, previousEncoderPos);
    }
    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, lcd_setting_name);
    char buffer[16];
    if (lcd_setting_type == 3)
        float_to_string(float(lcd_encoder_pos) / 100.0, buffer, lcd_setting_postfix);
    else
        int_to_string(lcd_encoder_pos, buffer, lcd_setting_postfix);
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}

#define lcd_edit_setting(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &_setting; \
            lcd_setting_type = sizeof(_setting); \
            lcd_encoder_pos = _setting; \
            lcd_setting_min = _min; \
            lcd_setting_max = _max; \
        } while(0)
#define lcd_edit_setting_float(_setting, _name, _postfix, _min, _max) do { \
            lcd_change_to_menu(lcd_menu_edit_setting); \
            lcd_setting_name = PSTR(_name); \
            lcd_setting_postfix = PSTR(_postfix); \
            lcd_setting_ptr = &_setting; \
            lcd_setting_type = 3; \
            lcd_encoder_pos = _setting * 100.0 + 0.5; \
            lcd_setting_min = _min; \
            lcd_setting_max = _max; \
        } while(0)

static void doCooldown()
{
    for(uint8_t n=0; n<EXTRUDERS; n++)
        setTargetHotend(0, n);
    setTargetBed(0);
    fanSpeed = 0;
    
    //quickStop();         //Abort all moves already in the planner
}
static void doCancelPrint()
{
    card.sdprinting = false;
    doCooldown();
    enquecommand_P(PSTR("G28"));
}
static void cardUpdir()
{
    card.updir();
}

static void lcd_clear_cache()
{
    for(uint8_t n=0; n<LCD_CACHE_COUNT; n++)
        LCD_CACHE_ID(n) = 0xFF;
    LCD_CACHE_NR_OF_FILES = 0xFF;
}

static void lcd_menu_main()
{
    lcd_tripple_menu(PSTR("PRINT"), PSTR("MATERIAL"), PSTR("MAINTENANCE"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_clear_cache();
            lcd_change_to_menu(lcd_menu_print_select, MENU_ITEM_POS(0));
        }
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_material);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance);
    }
    if (lcd_lib_button_down && lcd_encoder_pos == ENCODER_NO_SELECTION)
    {
        led_glow_dir = 0;
        if (led_glow > 200)
        {
            lcd_change_to_menu(lcd_menu_breakout);
        }
    }else{
        led_glow = led_glow_dir = 0;
    }

    lcd_lib_update_screen();
}

static void lcd_menu_material()
{
    lcd_tripple_menu(PSTR("CHANGE"), PSTR("SETTINGS"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            minProgress = 0;
            lcd_change_to_menu(lcd_menu_change_material_preheat);//TODO: If multiple extruders, select extruder.
        }
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_material_select, MENU_ITEM_POS(0));
            //lcd_change_to_menu(lcd_menu_material_settings, MENU_ITEM_POS(0));
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_main);
            
    }

    lcd_lib_update_screen();
}

static char* lcd_material_select_callback(uint8_t nr)
{
    uint8_t count = eeprom_read_byte(EEPROM_MATERIAL_COUNT_OFFSET());
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("<- RETURN"));
    else if (nr > count)
        strcpy_P(card.longFilename, PSTR("Customize"));
    else
        eeprom_read_block(card.longFilename, EEPROM_MATERIAL_NAME_OFFSET(nr - 1), 8);
    return card.longFilename;
}

static void lcd_material_select_details_callback(uint8_t nr)
{
    uint8_t count = eeprom_read_byte(EEPROM_MATERIAL_COUNT_OFFSET());
    if (nr == 0)
    {
        lcd_lib_draw_gfx(80, 13, backArrowGfx);
    }
    else if (nr <= count)
    {
        char buffer[8];
        int_to_string(eeprom_read_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(nr - 1)), buffer, PSTR("C"));
        lcd_lib_draw_string(70, 10, buffer);
        int_to_string(eeprom_read_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(nr - 1)), buffer, PSTR("C"));
        lcd_lib_draw_string(100, 10, buffer);
        int_to_string(eeprom_read_word(EEPROM_MATERIAL_FLOW_OFFSET(nr - 1)), buffer, PSTR("%"));
        lcd_lib_draw_stringP(70, 20, PSTR("Flow"));
        lcd_lib_draw_string(70 + 5 * 6, 20, buffer);

        int_to_string(eeprom_read_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(nr - 1)), buffer, PSTR("%"));
        lcd_lib_draw_stringP(70, 30, PSTR("Fan"));
        lcd_lib_draw_string(70 + 5 * 6, 30, buffer);
        float_to_string(eeprom_read_float(EEPROM_MATERIAL_DIAMETER_OFFSET(nr - 1)), buffer, PSTR("mm"));
        lcd_lib_draw_string(70, 40, buffer);
    }else{
        lcd_lib_draw_stringP(67, 10, PSTR("Modify the"));
        lcd_lib_draw_stringP(67, 20, PSTR("current"));
        lcd_lib_draw_stringP(67, 30, PSTR("settings"));
    }
}

static void lcd_menu_material_select()
{
    uint8_t count = eeprom_read_byte(EEPROM_MATERIAL_COUNT_OFFSET());
    if (count == 0)
    {
        //Fill in the defaults
        char buffer[8];
        
        strcpy_P(buffer, PSTR("PLA"));
        eeprom_write_block(buffer, EEPROM_MATERIAL_NAME_OFFSET(0), 4);
        eeprom_write_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(0), 210);
        eeprom_write_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(0), 65);
        eeprom_write_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(0), 100);
        eeprom_write_word(EEPROM_MATERIAL_FLOW_OFFSET(0), 100);
        eeprom_write_float(EEPROM_MATERIAL_DIAMETER_OFFSET(0), 2.85);

        strcpy_P(buffer, PSTR("ABS"));
        eeprom_write_block(buffer, EEPROM_MATERIAL_NAME_OFFSET(1), 4);
        eeprom_write_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(1), 240);
        eeprom_write_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(1), 80);
        eeprom_write_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(1), 50);
        eeprom_write_word(EEPROM_MATERIAL_FLOW_OFFSET(1), 107);
        eeprom_write_float(EEPROM_MATERIAL_DIAMETER_OFFSET(1), 2.85);
        
        eeprom_write_byte(EEPROM_MATERIAL_COUNT_OFFSET(), 2);
        return;
    }
    
    lcd_scroll_menu(PSTR("MATERIAL"), count + 2, lcd_material_select_callback, lcd_material_select_details_callback);
    if (lcd_lib_button_pressed)
    {
        printf("%i\n", count);
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_material);
        else if (IS_SELECTED(count + 1))
            lcd_change_to_menu(lcd_menu_material_settings);
        else{
            uint8_t nr = SELECTED_MENU_ITEM() - 1;
            material.temperature = eeprom_read_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(nr));
            material.bed_temperature = eeprom_read_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(nr));
            material.flow = eeprom_read_word(EEPROM_MATERIAL_FLOW_OFFSET(nr));

            material.fan_speed = eeprom_read_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(nr));
            material.diameter = eeprom_read_float(EEPROM_MATERIAL_DIAMETER_OFFSET(nr));
            eeprom_read_block(card.longFilename, EEPROM_MATERIAL_NAME_OFFSET(nr), 8);
            card.longFilename[8] = '\0';
            
            lcd_change_to_menu(lcd_menu_material_selected, MENU_ITEM_POS(0));
        }
    }
}

static void lcd_menu_material_selected()
{
    lcd_info_screen(lcd_menu_main, NULL, PSTR("OK"));
    lcd_lib_draw_string_centerP(20, PSTR("Selected material:"));
    lcd_lib_draw_string_center(30, card.longFilename);
    lcd_lib_update_screen();
}

static char* lcd_material_settings_callback(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("<- RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Temperature"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("Heated bed"));
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Diameter"));
    else if (nr == 4)
        strcpy_P(card.longFilename, PSTR("Fan"));
    else if (nr == 5)
        strcpy_P(card.longFilename, PSTR("Flow %"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_material_settings_details_callback(uint8_t nr)
{
    char buffer[10];
    buffer[0] = '\0';
    if (nr == 0)
    {
        lcd_lib_draw_gfx(80, 13, backArrowGfx);
        return;
    }else if (nr == 1)
    {
        int_to_string(material.temperature, buffer, PSTR("C"));
    }else if (nr == 2)
    {
        int_to_string(material.bed_temperature, buffer, PSTR("C"));
    }else if (nr == 3)
    {
        float_to_string(material.diameter, buffer, PSTR("mm"));
    }else if (nr == 4)
    {
        int_to_string(material.fan_speed, buffer, PSTR("%"));
    }else if (nr == 5)
    {
        int_to_string(material.flow, buffer, PSTR("%"));
    }
    lcd_lib_draw_string(70, 25, buffer);
}

static void lcd_menu_material_settings()
{
    lcd_scroll_menu(PSTR("MATERIAL"), 6, lcd_material_settings_callback, lcd_material_settings_details_callback);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_material);
        else if (IS_SELECTED(1))
            lcd_edit_setting(material.temperature, "Temperature", "C", 0, HEATER_0_MAXTEMP - 15);
        else if (IS_SELECTED(2))
            lcd_edit_setting(material.bed_temperature, "Bed Temperature", "C", 0, BED_MAXTEMP - 15);
        else if (IS_SELECTED(3))
            lcd_edit_setting_float(material.diameter, "Material Diameter", "mm", 0, 1000);
        else if (IS_SELECTED(4))
            lcd_edit_setting(material.fan_speed, "Fan speed", "%", 0, 100);
        else if (IS_SELECTED(5))
            lcd_edit_setting(material.flow, "Material flow", "%", 1, 1000);
    }
}

static void lcd_menu_maintenance()
{
    lcd_tripple_menu(PSTR("FIRST RUN"), PSTR("ADVANCED"), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance_first_run_select);
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_maintenance_advanced);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_main);
    }

    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_select()
{
    lcd_tripple_menu(PSTR("BED HEIGHT"), PSTR("..."), PSTR("RETURN"));

    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
        {
            lcd_change_to_menu(lcd_menu_maintenance_first_run_bed_level_start);
        }else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_TODO);
        else if (IS_SELECTED(2))
            lcd_change_to_menu(lcd_menu_maintenance);
    }

    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_initial()
{
    lcd_info_screen(lcd_menu_maintenance_first_run_bed_level_start, NULL, PSTR("OK"));
    lcd_lib_draw_string_centerP(10, PSTR("Ultimaker ready"));
    lcd_lib_draw_string_centerP(20, PSTR("Blabla bla bla"));
    lcd_lib_draw_string_centerP(30, PSTR("This is first bla"));
    lcd_lib_draw_string_centerP(40, PSTR("Not enough room"));
    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_bed_level_start()
{
    add_homeing[Z_AXIS] = 0;
    enquecommand_P(PSTR("G28"));
    char buffer[32];
    sprintf_P(buffer, PSTR("G1 F%i Z%i X%i Y%i"), int(homing_feedrate[0]), 35, X_MAX_LENGTH/2, Y_MAX_LENGTH / 2);
    enquecommand(buffer);
    currentMenu = lcd_menu_maintenance_first_run_bed_level;
}

static void lcd_menu_maintenance_first_run_bed_level()
{
    lcd_info_screen(lcd_menu_maintenance, doCancelPrint);
    lcd_lib_draw_string_centerP(15, PSTR("Moving head to"));
    lcd_lib_draw_string_centerP(25, PSTR("start position..."));
    if (!is_command_queued() && !blocks_queued())
    {
        lcd_change_to_menu(lcd_menu_maintenance_first_run_bed_level_adjust);
        lcd_encoder_pos = 0;
    }
    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_bed_level_adjust()
{
    lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4);
    
    if (lcd_encoder_pos != 0)
    {
        current_position[Z_AXIS] += float(lcd_encoder_pos) * 0.05;
        lcd_encoder_pos = 0;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], 60, 0);
    }
    if (lcd_lib_button_pressed)
    {
        add_homeing[Z_AXIS] = -current_position[Z_AXIS];
        Config_StoreSettings();
        doCancelPrint();
        if (IS_FIRST_RUN_DONE())
        {
            lcd_change_to_menu(lcd_menu_maintenance);
        }else{
            lcd_change_to_menu(lcd_menu_maintenance_first_run_done, MENU_ITEM_POS(0));
            SET_FIRST_RUN_DONE();
        }
    }

    lcd_info_screen(NULL, NULL, PSTR("READY"));
    lcd_lib_draw_string_centerP(10, PSTR("Adjust the bed"));
    lcd_lib_draw_string_centerP(20, PSTR("by rotating the dial"));
    lcd_lib_draw_string_centerP(30, PSTR("Have the nozzle touch"));
    lcd_lib_draw_string_centerP(40, PSTR("the bed."));
    lcd_lib_update_screen();
}

static void lcd_menu_maintenance_first_run_done()
{
    lcd_info_screen(lcd_menu_main, NULL, PSTR("OK"));
    
    lcd_lib_draw_string_centerP(20, PSTR("Your Ultimaker"));
    lcd_lib_draw_string_centerP(30, PSTR("is now ready."));
    lcd_lib_update_screen();
}

static char* lcd_advanced_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("<- RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Factory reset"));
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_advanced_details(uint8_t nr)
{
}

static void lcd_menu_maintenance_advanced()
{
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED(0))
            lcd_change_to_menu(lcd_menu_maintenance);
        else if (IS_SELECTED(1))
            lcd_change_to_menu(lcd_menu_advanced_factory_reset, MENU_ITEM_POS(1));
    }
    lcd_scroll_menu("ADVANCED", 5, lcd_advanced_item, lcd_advanced_details);
}

static void doFactoryReset()
{
    eeprom_write_byte((uint8_t*)100, 0);
    eeprom_write_byte((uint8_t*)101, 0);
    eeprom_write_byte((uint8_t*)102, 0);
    eeprom_write_byte((uint8_t*)EEPROM_FIRST_RUN_DONE_OFFSET, 0);
    eeprom_write_byte(EEPROM_MATERIAL_COUNT_OFFSET(), 0);
    
    cli();
    //NOTE: Jumping to address 0 is not a fully proper way to reset.
    // Letting the watchdog timeout is a better reset, but the bootloader does not continue on a watchdog timeout.
    // So we disable interrupts and hope for the best!
    //Jump to address 0x0000
#ifdef __AVR__
    asm volatile(
            "clr	r30		\n\t"
            "clr	r31		\n\t"
            "ijmp	\n\t"
            );
#else
    exit(0);
#endif
}

static void lcd_menu_advanced_factory_reset()
{
    lcd_question_screen(NULL, doFactoryReset, PSTR("YES"), previousMenu, NULL, PSTR("NO"));
    
    lcd_lib_draw_string_centerP(10, PSTR("Reset everything"));
    lcd_lib_draw_string_centerP(20, PSTR("to default?"));
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_preheat()
{
    setTargetHotend(material.temperature + 10, 0);
    int16_t temp = degHotend(0) - 20;
    int16_t target = degTargetHotend(0) - 10 - 20;
    if (temp < 0) temp = 0;
    if (temp > target)
    {
        volume_to_filament_length = 1.0;//Set the extrusion to 1mm per given value, so we can move the filament a set distance.
        
        float old_max_feedrate_e = max_feedrate[E_AXIS];
        float old_retract_acceleration = retract_acceleration;
        max_feedrate[E_AXIS] = FILAMENT_REVERSAL_SPEED;
        retract_acceleration = FILAMENT_LONG_MOVE_ACCELERATION;
        
        current_position[E_AXIS] = 0;
        plan_set_e_position(current_position[E_AXIS]);
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], -1.0, FILAMENT_REVERSAL_SPEED, 0);
        for(uint8_t n=0;n<6;n++)
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], (n+1)*-FILAMENT_REVERSAL_LENGTH/6, FILAMENT_REVERSAL_SPEED, 0);
        
        max_feedrate[E_AXIS] = old_max_feedrate_e;
        retract_acceleration = old_retract_acceleration;
        
        currentMenu = lcd_menu_change_material_remove;
        temp = target;
    }

    uint8_t progress = uint8_t(temp * 125 / target);
    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;
    
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(3, 10, PSTR("Heating printhead"));
    lcd_lib_draw_stringP(3, 20, PSTR("for material removal"));

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_remove()
{
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(3, 20, PSTR("Reversing material"));
    
    if (!blocks_queued())
    {
        lcd_lib_beep();
        led_glow_dir = led_glow = 0;
        currentMenu = lcd_menu_change_material_remove_wait_user;
        SELECT_MENU_ITEM(0);
        //Disable the extruder motor so you can pull out the remaining filament.
        disable_e0();
        disable_e1();
        disable_e2();
    }

    long pos = -st_get_position(E_AXIS);
    long targetPos = lround(FILAMENT_REVERSAL_LENGTH*axis_steps_per_unit[E_AXIS]);
    uint8_t progress = (pos * 125 / targetPos);
    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_remove_wait_user_ready()
{
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);
    lcd_change_to_menu(lcd_menu_change_material_insert_wait_user, MENU_ITEM_POS(0));
    //TODO: Move head to front
}

static void lcd_menu_change_material_remove_wait_user()
{
    lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4);

    lcd_question_screen(NULL, lcd_menu_change_material_remove_wait_user_ready, PSTR("READY"), lcd_menu_main, doCooldown, PSTR("CANCEL"));
    lcd_lib_draw_string_centerP(20, PSTR("Remove material"));
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert_wait_user_ready()
{
    //Override the max feedrate and acceleration values to get a better insert speed and speedup/slowdown
    float old_max_feedrate_e = max_feedrate[E_AXIS];
    float old_retract_acceleration = retract_acceleration;
    max_feedrate[E_AXIS] = FILAMENT_INSERT_FAST_SPEED;
    retract_acceleration = FILAMENT_LONG_MOVE_ACCELERATION;
    
    current_position[E_AXIS] = 0;
    plan_set_e_position(current_position[E_AXIS]);
    for(uint8_t n=0;n<6;n++)
    {
        current_position[E_AXIS] += FILAMENT_FORWARD_LENGTH / 6;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_FAST_SPEED, 0);
    }
    
    //Put back origonal values.
    max_feedrate[E_AXIS] = old_max_feedrate_e;
    retract_acceleration = old_retract_acceleration;
    
    lcd_change_to_menu(lcd_menu_change_material_insert_forward);
}

static void lcd_menu_change_material_insert_wait_user()
{
    lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4);

    if (movesplanned() < 2)
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_EXTRUDE_SPEED, 0);
    }
    
    lcd_question_screen(NULL, lcd_menu_change_material_insert_wait_user_ready, PSTR("READY"), lcd_menu_main, doCooldown, PSTR("CANCEL"));
    lcd_lib_draw_stringP(65 - strlen_P(PSTR("Insert new material")) * 3, 20, PSTR("Insert new material"));
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert_forward()
{
    lcd_info_screen(lcd_menu_main, doCooldown);
    lcd_lib_draw_stringP(3, 20, PSTR("Forwarding material"));
    
    if (!blocks_queued())
    {
        lcd_lib_beep();
        led_glow_dir = led_glow = 0;
        //TODO: Set E motor power lower so the motor skips instead of digging into the material
        currentMenu = lcd_menu_change_material_insert;
        SELECT_MENU_ITEM(0);
    }

    long pos = st_get_position(E_AXIS);
    long targetPos = lround(FILAMENT_FORWARD_LENGTH*axis_steps_per_unit[E_AXIS]);
    uint8_t progress = (pos * 125 / targetPos);
    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_change_material_insert_ready()
{
    doCooldown();
    lcd_change_to_menu(lcd_menu_main);
}
static void lcd_menu_change_material_insert()
{
    lcd_lib_led_color(8 + led_glow / 2, 8 + led_glow / 2, 16 + led_glow / 4);
    
    lcd_question_screen(NULL, lcd_menu_change_material_insert_ready, PSTR("READY"), lcd_menu_main, doCooldown, PSTR("CANCEL"));
    lcd_lib_draw_string_centerP(20, PSTR("Wait till material"));
    lcd_lib_draw_string_centerP(30, PSTR("comes out the nozzle"));

    if (movesplanned() < 2)
    {
        current_position[E_AXIS] += 0.5;
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], FILAMENT_INSERT_SPEED, 0);
    }
    
    lcd_lib_update_screen();
}

static char* lcd_sd_menu_filename_callback(uint8_t nr)
{
    //This code uses the card.longFilename as buffer to store the filename, to save memory.
    if (nr == 0)
    {
        if (card.atRoot())
        {
            strcpy_P(card.longFilename, PSTR("<- RETURN"));
        }else{
            strcpy_P(card.longFilename, PSTR("<- BACK"));
        }
    }else{
        card.longFilename[0] = '\0';
        for(uint8_t idx=0; idx<LCD_CACHE_COUNT; idx++)
        {
            if (LCD_CACHE_ID(idx) == nr)
                strcpy(card.longFilename, LCD_CACHE_FILENAME(idx));
        }
        if (card.longFilename[0] == '\0')
        {
            card.getfilename(nr - 1);
            if (!card.longFilename[0])
                strcpy(card.longFilename, card.filename);
            if (!card.filenameIsDir)
            {
                if (strchr(card.longFilename, '.')) strchr(card.longFilename, '.')[0] = '\0';
            }

            uint8_t idx = nr % LCD_CACHE_COUNT;
            LCD_CACHE_ID(idx) = nr;
            strcpy(LCD_CACHE_FILENAME(idx), card.longFilename);
            LCD_CACHE_TYPE(idx) = card.filenameIsDir ? 1 : 0;
        }
    }
    return card.longFilename;
}

void lcd_sd_menu_details_callback(uint8_t nr)
{
    if (nr == 0)
    {
        lcd_lib_draw_gfx(80, 13, backArrowGfx);
        return;
    }
    for(uint8_t idx=0; idx<LCD_CACHE_COUNT; idx++)
    {
        if (LCD_CACHE_ID(idx) == nr)
        {
            if (LCD_CACHE_TYPE(idx) == 1)
                lcd_lib_draw_gfx(80, 20, folderGfx);
        }
    }
}

static void lcd_menu_print_select()
{
    if (!IS_SD_INSERTED)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(15, PSTR("No SD CARD!"));
        lcd_lib_draw_string_centerP(25, PSTR("Please insert card"));
        lcd_lib_update_screen();
        card.release();
        return;
    }
    
    if (!card.cardOK)
    {
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(16, PSTR("Reading card..."));
        lcd_lib_update_screen();
        lcd_clear_cache();
        card.initsd();
        return;
    }
    
    if (LCD_CACHE_NR_OF_FILES == 0xFF)
        LCD_CACHE_NR_OF_FILES = card.getnrfilenames();
    uint8_t nrOfFiles = LCD_CACHE_NR_OF_FILES + 1;
    if (nrOfFiles == 1)
    {
        if (card.atRoot())
            lcd_info_screen(lcd_menu_main, NULL, PSTR("OK"));
        else
            lcd_info_screen(lcd_menu_print_select, cardUpdir, PSTR("OK"));
        lcd_lib_draw_string_centerP(25, PSTR("No files found!"));
        lcd_lib_update_screen();
        lcd_clear_cache();
        return;
    }
    
    if (lcd_lib_button_pressed)
    {
        uint8_t selIndex = uint16_t(lcd_encoder_pos/ENCODER_TICKS_PER_FILE_MENU_ITEM);
        if (selIndex == 0)
        {
            if (card.atRoot())
            {
                lcd_change_to_menu(lcd_menu_main);
            }else{
                lcd_clear_cache();
                lcd_lib_beep();
                card.updir();
            }
        }else{
            card.getfilename(selIndex - 1);
            if (!card.filenameIsDir)
            {
                //Start print
                card.openFile(card.filename, true);
                if (card.isFileOpen())
                {
                    card.longFilename[20] = '\0';
                    if (strchr(card.longFilename, '.')) strchr(card.longFilename, '.')[0] = '\0';
                    
                    char buffer[64];
                    card.fgets(buffer, sizeof(buffer));
                    buffer[sizeof(buffer)-1] = '\0';
                    while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                    if (strcmp_P(buffer, PSTR(";FLAVOR:UltiGCode")) != 0)
                    {
                        card.fgets(buffer, sizeof(buffer));
                        buffer[sizeof(buffer)-1] = '\0';
                        while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                    }
                    card.setIndex(0);
                    if (strcmp_P(buffer, PSTR(";FLAVOR:UltiGCode")) == 0)
                    {
                        //New style GCode flavor without start/end code.
                        // Temperature settings, filament settings, fan settings, start and end-code are machine controlled.
                        target_temperature[0] = material.temperature;
                        target_temperature_bed = material.bed_temperature;
                        volume_to_filament_length = 1.0 / (M_PI * (material.diameter / 2.0) * (material.diameter / 2.0));
                        fanSpeedPercent = material.fan_speed;
                        extrudemultiply = material.flow;
                        
                        fanSpeed = 0;
                        lcd_change_to_menu(lcd_menu_print_heatup);
                    }else{
                        //Classic gcode file
                        
                        //Set the settings to defaults so the classic GCode has full control
                        fanSpeedPercent = 100;
                        volume_to_filament_length = 1.0;
                        extrudemultiply = 100;
                        
                        lcd_change_to_menu(lcd_menu_print_classic_warning, MENU_ITEM_POS(0));
                    }
                }
            }else{
                lcd_lib_beep();
                lcd_clear_cache();
                card.chdir(card.filename);
                SELECT_MENU_ITEM(0);
            }
            return;//Return so we do not continue after changing the directory or selecting a file. The nrOfFiles is invalid at this point.
        }
    }
    lcd_scroll_menu(PSTR("SD CARD"), nrOfFiles, lcd_sd_menu_filename_callback, lcd_sd_menu_details_callback);
}

static void lcd_menu_print_heatup()
{
    lcd_info_screen(lcd_menu_print_abort, NULL, PSTR("ABORT"));
    
    if (current_temperature[0] >= target_temperature[0] - TEMP_WINDOW && current_temperature_bed >= target_temperature_bed - TEMP_WINDOW)
    {
        //TODO: Custom start code.
        enquecommand_P(PSTR("G28"));
        enquecommand_P(PSTR("G92 E-5"));
        enquecommand_P(PSTR("G1 F200 E0"));
        card.startFileprint();
        starttime = millis();
        currentMenu = lcd_menu_print_printing;
    }

    uint8_t progress = 125;
    uint8_t p;
    
    p = (current_temperature[0] - 20) * 125 / (target_temperature[0] - 20 - TEMP_WINDOW);
    if (p < progress)
        progress = p;
    p = (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW);
    if (p < progress)
        progress = p;
    
    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;
    
    lcd_lib_draw_string_centerP(15, PSTR("Heating up"));
    lcd_lib_draw_string_centerP(25, PSTR("before printing"));

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_print_printing()
{
    lcd_question_screen(NULL, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));
    
    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    lcd_lib_draw_string_centerP(15, PSTR("Printing"));
    lcd_lib_draw_string_center(25, card.longFilename);
    unsigned long printTime = (millis() - starttime);
    unsigned long totalTime = float(printTime) * float(card.getFileSize()) / float(card.getFilePos());
    unsigned long timeLeft = (totalTime - printTime) / 1000L;
    
    char buffer[8];
    int_to_time_string(timeLeft, buffer);
    lcd_lib_draw_string(5, 5, buffer);

    if (!card.sdprinting && !is_command_queued())
    {
        doCooldown();
        enquecommand_P(PSTR("G92 E5"));
        enquecommand_P(PSTR("G1 F200 E0"));
        enquecommand_P(PSTR("G28"));
        currentMenu = lcd_menu_print_ready;
        SELECT_MENU_ITEM(0);
    }

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void doStartPrint()
{
    card.startFileprint();
    starttime = millis();
}
static void lcd_menu_print_classic_warning()
{
    lcd_question_screen(lcd_menu_print_classic, doStartPrint, PSTR("CONTINUE"), lcd_menu_print_select, NULL, PSTR("CANCEL"));
    
    lcd_lib_draw_string_centerP(10, PSTR("This file ignores"));
    lcd_lib_draw_string_centerP(20, PSTR("material settings"));

    lcd_lib_update_screen();
}

static void lcd_menu_print_classic()
{
    lcd_info_screen(lcd_menu_print_abort, NULL, PSTR("ABORT"));
    
    uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
    if (current_temperature[0] < target_temperature[0] - TEMP_WINDOW - 5 || current_temperature_bed < target_temperature_bed - TEMP_WINDOW - 5)
    {
        progress = 125;
        uint8_t p;
        
        p = (current_temperature[0] - 20) * 125 / (target_temperature[0] - 20 - TEMP_WINDOW);
        if (p < progress)
            progress = p;
        p = (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW);
        if (p < progress)
            progress = p;
        
        if (progress < minProgress)
            progress = minProgress;
        else
            minProgress = progress;
        lcd_lib_draw_string_centerP(15, PSTR("Heating"));
    }else{
        lcd_lib_draw_string_centerP(15, PSTR("Printing"));
    }
    lcd_lib_draw_string_center(25, card.longFilename);
    
    if (!card.sdprinting)
    {
        doCooldown();
        currentMenu = lcd_menu_print_ready;
        SELECT_MENU_ITEM(0);
    }

    lcd_progressbar(progress);
    
    lcd_lib_update_screen();
}

static void lcd_menu_print_abort()
{
    lcd_question_screen(lcd_menu_print_ready, doCancelPrint, PSTR("YES"), previousMenu, NULL, PSTR("NO"));
    
    lcd_lib_draw_string_centerP(15, PSTR("Abort the print?"));

    lcd_lib_update_screen();
}

static void lcd_menu_print_ready()
{
    lcd_info_screen(lcd_menu_main, NULL, PSTR("BACK TO MENU"));

    if (current_temperature[0] > 80)
    {
        lcd_lib_draw_string_centerP(15, PSTR("Printer cooling down"));

        int16_t progress = 124 - (current_temperature[0] - 80);
        if (progress < 0) progress = 0;
        if (progress > 124) progress = 124;
        
        if (progress < minProgress)
            progress = minProgress;
        else
            minProgress = progress;
            
        lcd_progressbar(progress);
        char buffer[8];
        int_to_string(current_temperature[0], buffer, PSTR("C"));
        lcd_lib_draw_string_center(25, buffer);
    }else{
        lcd_lib_draw_string_centerP(15, PSTR("Print finished"));
    }
    lcd_lib_update_screen();
}

static void lcd_menu_TODO()
{
    lcd_info_screen(previousMenu, NULL, PSTR("OK"));
    
    lcd_lib_draw_string_centerP(20, PSTR("UNIMPLEMENTED"));

    lcd_lib_update_screen();
}

#define BREAKOUT_PADDLE_WIDTH 21
#define ball_x (*(int16_t*)&lcd_cache[3*5])
#define ball_y (*(int16_t*)&lcd_cache[3*5+2])
#define ball_dx (*(int16_t*)&lcd_cache[3*5+4])
#define ball_dy (*(int16_t*)&lcd_cache[3*5+6])
static void lcd_menu_breakout()
{
    if (lcd_encoder_pos == ENCODER_NO_SELECTION)
    {
        lcd_encoder_pos = (128 - BREAKOUT_PADDLE_WIDTH) / 2 / 2;
        for(uint8_t y=0; y<3;y++)
            for(uint8_t x=0; x<5;x++)
                lcd_cache[x+y*5] = 3;
        ball_x = 0;
        ball_y = 57 << 8;
        ball_dx = 0;
        ball_dy = 0;
    }
    
    if (lcd_encoder_pos < 0) lcd_encoder_pos = 0;
    if (lcd_encoder_pos * 2 > 128 - BREAKOUT_PADDLE_WIDTH - 1) lcd_encoder_pos = (128 - BREAKOUT_PADDLE_WIDTH - 1) / 2;
    ball_x += ball_dx;
    ball_y += ball_dy;
    if (ball_x < 1 << 8) ball_dx = abs(ball_dx);
    if (ball_x > 124 << 8) ball_dx = -abs(ball_dx);
    if (ball_y < (1 << 8)) ball_dy = abs(ball_dy);
    if (ball_y < (3 * 10) << 8)
    {
        uint8_t x = (ball_x >> 8) / 25;
        uint8_t y = (ball_y >> 8) / 10;
        if (lcd_cache[x+y*5])
        {
            lcd_cache[x+y*5]--;
            ball_dy = abs(ball_dy);
        }
    }
    if (ball_y > (58 << 8))
    {
        if (ball_x < (lcd_encoder_pos * 2 - 2) << 8 || ball_x > (lcd_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH) << 8)
            lcd_change_to_menu(lcd_menu_main);
        ball_dx += (ball_x - ((lcd_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH / 2) * 256)) / 64;
        ball_dy = -512 + abs(ball_dx);
    }
    if (ball_dy == 0)
    {
        ball_y = 57 << 8;
        ball_x = (lcd_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH / 2) << 8;
        if (lcd_lib_button_pressed)
        {
            ball_dx = -256 + lcd_encoder_pos * 8;
            ball_dy = -512 + abs(ball_dx);
        }
    }
    
    lcd_lib_clear();
    
    for(uint8_t y=0; y<3;y++)
        for(uint8_t x=0; x<5;x++)
        {
            if (lcd_cache[x+y*5])
                lcd_lib_draw_box(3 + x*25, 2 + y * 10, 23 + x*25, 10 + y * 10);
            if (lcd_cache[x+y*5] == 2)
                lcd_lib_draw_shade(4 + x*25, 3 + y * 10, 22 + x*25, 9 + y * 10);
            if (lcd_cache[x+y*5] == 3)
                lcd_lib_set(4 + x*25, 3 + y * 10, 22 + x*25, 9 + y * 10);
        }
    
    lcd_lib_draw_box(ball_x >> 8, ball_y >> 8, (ball_x >> 8) + 2, (ball_y >> 8) + 2);
    lcd_lib_draw_box(lcd_encoder_pos * 2, 60, lcd_encoder_pos * 2 + BREAKOUT_PADDLE_WIDTH, 63);
    lcd_lib_update_screen();
}

/* Warning: This function is called from interrupt context */
void lcd_buttons_update()
{
    lcd_lib_buttons_update_interrupt();
}

//#endif//ENABLE_ULTILCD2
