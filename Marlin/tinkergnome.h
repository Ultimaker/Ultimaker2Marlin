#ifndef ULTI_LCD2_MENU_TINKERGNOME_H
#define ULTI_LCD2_MENU_TINKERGNOME_H

#include "temperature.h"

#define EEPROM_UI_MODE_OFFSET 0x401
#define EEPROM_LED_TIMEOUT_OFFSET 0x402
#define EEPROM_LCD_TIMEOUT_OFFSET 0x404
#define EEPROM_LCD_CONTRAST_OFFSET 0x406
#define EEPROM_EXPERT_VERSION_OFFSET 0x407
#define EEPROM_SLEEP_BRIGHTNESS_OFFSET 0x409
#define EEPROM_SLEEP_CONTRAST_OFFSET 0x40A
#define EEPROM_SLEEP_GLOW_OFFSET 0x40B
#define EEPROM_PID_FLAGS 0x40C
#define EEPROM_HEATER_TIMEOUT 0x40D
#define EEPROM_AXIS_LIMITS 0x40E  // 24 Byte

#define GET_UI_MODE() (eeprom_read_byte((const uint8_t*)EEPROM_UI_MODE_OFFSET))
#define SET_UI_MODE(n) do { eeprom_write_byte((uint8_t*)EEPROM_UI_MODE_OFFSET, n); } while(0)
#define GET_LED_TIMEOUT() (eeprom_read_word((const uint16_t*)EEPROM_LED_TIMEOUT_OFFSET))
#define SET_LED_TIMEOUT(n) do { eeprom_write_word((uint16_t*)EEPROM_LED_TIMEOUT_OFFSET, n); } while(0)
#define GET_LCD_TIMEOUT() (eeprom_read_word((const uint16_t*)EEPROM_LCD_TIMEOUT_OFFSET))
#define SET_LCD_TIMEOUT(n) do { eeprom_write_word((uint16_t*)EEPROM_LCD_TIMEOUT_OFFSET, n); } while(0)
#define GET_LCD_CONTRAST() (eeprom_read_byte((const uint8_t*)EEPROM_LCD_CONTRAST_OFFSET))
#define SET_LCD_CONTRAST(n) do { eeprom_write_byte((uint8_t*)EEPROM_LCD_CONTRAST_OFFSET, n); } while(0)
#define GET_EXPERT_VERSION() (eeprom_read_word((const uint16_t*)EEPROM_EXPERT_VERSION_OFFSET))
#define SET_EXPERT_VERSION(n) do { eeprom_write_word((uint16_t*)EEPROM_EXPERT_VERSION_OFFSET, n); } while(0)
#define GET_SLEEP_BRIGHTNESS() (eeprom_read_byte((const uint8_t*)EEPROM_SLEEP_BRIGHTNESS_OFFSET))
#define SET_SLEEP_BRIGHTNESS(n) do { eeprom_write_byte((uint8_t*)EEPROM_SLEEP_BRIGHTNESS_OFFSET, n); } while(0)
#define GET_SLEEP_CONTRAST() (eeprom_read_byte((const uint8_t*)EEPROM_SLEEP_CONTRAST_OFFSET))
#define SET_SLEEP_CONTRAST(n) do { eeprom_write_byte((uint8_t*)EEPROM_SLEEP_CONTRAST_OFFSET, n); } while(0)
#define GET_SLEEP_GLOW() (eeprom_read_byte((const uint8_t*)EEPROM_SLEEP_GLOW_OFFSET))
#define SET_SLEEP_GLOW(n) do { eeprom_write_byte((uint8_t*)EEPROM_SLEEP_GLOW_OFFSET, n); } while(0)
#define GET_PID_FLAGS() (eeprom_read_byte((const uint8_t*)EEPROM_PID_FLAGS))
#define SET_PID_FLAGS(n) do { eeprom_write_byte((uint8_t*)EEPROM_PID_FLAGS, n); } while(0)
#define GET_HEATER_TIMEOUT() (eeprom_read_byte((const uint8_t*)EEPROM_HEATER_TIMEOUT))
#define SET_HEATER_TIMEOUT(n) do { eeprom_write_byte((uint8_t*)EEPROM_HEATER_TIMEOUT, n); } while(0)

// UI Mode
#define UI_MODE_STANDARD  0
#define UI_MODE_EXPERT    1

#define UI_BEEP_SHORT    32
#define UI_BEEP_OFF      64

// PID control flags
#define PID_FLAG_NOZZLE 1
#define PID_FLAG_BED 2

extern uint8_t ui_mode;
extern uint16_t lcd_timeout;
extern uint8_t lcd_contrast;
extern uint8_t led_sleep_glow;
extern uint8_t lcd_sleep_contrast;
extern uint8_t pid_flags;

extern const uint8_t standbyGfx[];
extern const uint8_t startGfx[];
extern const uint8_t pauseGfx[];
extern const uint8_t backGfx[];
extern const uint8_t hourglassGfx[];
extern const uint8_t filamentGfx[];
extern const uint8_t menuGfx[];
extern const uint8_t bedTempGfx[];
extern const uint8_t checkbox_on[];
extern const uint8_t checkbox_off[];

extern float recover_height;
extern float recover_position[NUM_AXIS];
extern int recover_temperature[EXTRUDERS];

void tinkergnome_init();
void lcd_menu_maintenance_expert();
void lcd_menu_print_heatup_tg();
void lcd_menu_printing_tg();
void lcd_menu_move_axes();
void lcd_lib_draw_heater(uint8_t x, uint8_t y, uint8_t heaterPower);
void manage_led_timeout();
void manage_encoder_position(int8_t encoder_pos_interrupt);
void lcd_menu_expert_extrude();
void lcd_extrude_quit_menu();
void lcd_menu_sleeptimer();
void recover_start_print();
void lcd_menu_recover_init();
void lcd_menu_expert_recover();
void reset_printing_state();

bool lcd_tune_byte(uint8_t &value, uint8_t _min, uint8_t _max);
void lcd_tune_value(uint8_t &value, uint8_t _min, uint8_t _max);
void lcd_tune_value(int &value, int _min, int _max);

void lcd_lib_draw_bargraph( uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, float value );

char* int_to_time_string_tg(unsigned long i, char* temp_buffer);

FORCE_INLINE bool pidTempBed() { return (pid_flags & PID_FLAG_BED); }
FORCE_INLINE void lcd_print_tune_nozzle0() { lcd_tune_value(target_temperature[0], 0, get_maxtemp(0) - 15); }
#if EXTRUDERS > 1
FORCE_INLINE void lcd_print_tune_nozzle1() { lcd_tune_value(target_temperature[1], 0, get_maxtemp(1) - 15); }
#endif
#if TEMP_SENSOR_BED != 0
FORCE_INLINE void lcd_print_tune_bed() { lcd_tune_value(target_temperature_bed, 0, BED_MAXTEMP - 15); }
#endif
#endif//ULTI_LCD2_MENU_TINKERGNOME_H
