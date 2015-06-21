#ifndef ULTI_LCD2_MENU_TINKERGNOME_H
#define ULTI_LCD2_MENU_TINKERGNOME_H

#define EEPROM_UI_MODE_OFFSET 0x401
#define EEPROM_LED_TIMEOUT_OFFSET 0x402
#define EEPROM_LCD_TIMEOUT_OFFSET 0x404
#define EEPROM_LCD_CONTRAST_OFFSET 0x406
#define EEPROM_EXPERT_VERSION_OFFSET 0x407
#define EEPROM_SLEEP_BRIGHTNESS_OFFSET 0x409
#define EEPROM_SLEEP_CONTRAST_OFFSET 0x40A
#define EEPROM_SLEEP_GLOW_OFFSET 0x40B

// customizations and enhancements (added by Lars June 3,2014)
#define EXTENDED_BEEP 1		// enables extended audio feedback

// UI Mode
#define UI_MODE_STANDARD 0
#define UI_MODE_EXPERT 1

extern uint8_t ui_mode;
extern uint16_t lcd_timeout;
extern uint8_t lcd_contrast;
extern uint8_t led_sleep_glow;
extern uint8_t lcd_sleep_contrast;

extern const uint8_t standbyGfx[];
extern const uint8_t startGfx[];
extern const uint8_t pauseGfx[];
extern const uint8_t backGfx[];
extern const uint8_t hourglassGfx[];
extern const uint8_t filamentGfx[];
extern const uint8_t menuGfx[];

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

char* int_to_time_string_tg(unsigned long i, char* temp_buffer);

#endif//ULTI_LCD2_MENU_TINKERGNOME_H

