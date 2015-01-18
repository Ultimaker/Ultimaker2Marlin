#ifndef ULTI_LCD2_MENU_TINKERGNOME_H
#define ULTI_LCD2_MENU_TINKERGNOME_H

#define EEPROM_UI_MODE_OFFSET 0x401
#define GET_UI_MODE() (eeprom_read_byte((const uint8_t*)EEPROM_UI_MODE_OFFSET))
#define SET_UI_MODE(n) do { eeprom_write_byte((uint8_t*)EEPROM_UI_MODE_OFFSET, n); } while(0)
#define EEPROM_LED_TIMEOUT_OFFSET 0x402
#define GET_LED_TIMEOUT() (eeprom_read_word((const uint16_t*)EEPROM_LED_TIMEOUT_OFFSET))
#define SET_LED_TIMEOUT(n) do { eeprom_write_word((uint16_t*)EEPROM_LED_TIMEOUT_OFFSET, n); } while(0)

// customizations and enhancements (added by Lars June 3,2014)
#define EXTENDED_BEEP 1					// enables extened audio feedback
//#define MAX_ENCODER_ACCELERATION 4		// set this to 1 for no acceleration

// UI Mode
#define UI_MODE_STANDARD 0
#define UI_MODE_TINKERGNOME 1

extern uint8_t ui_mode;
extern uint16_t led_timeout;

void tinkergnome_init();
void lcd_menu_maintenance_expert();
void lcd_menu_print_heatup_tg();
void lcd_menu_printing_tg();
void lcd_menu_move_axes();
void lcd_lib_draw_heater(uint8_t x, uint8_t y, uint8_t heaterPower);
void manage_led_timeout();

#endif//ULTI_LCD2_MENU_TINKERGNOME_H

