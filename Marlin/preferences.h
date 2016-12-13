#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "Marlin.h"
#include "fastio.h"

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
#define EEPROM_END_RETRACT 0x426  // 4 Byte
#define EEPROM_HEATER_CHECK_TEMP 0x42A  // 1 Byte
#define EEPROM_HEATER_CHECK_TIME 0x42B  // 1 Byte
#define EEPROM_PID_2 0x42C  // 12 Byte
#define EEPROM_MOTOR_CURRENT_E2 0x438  // 2 Byte
#define EEPROM_PID_BED 0x43A  // 12 Byte
#define EEPROM_STEPS_E2 0x446  // 4 Byte
#define EEPROM_AXIS_DIRECTION 0x44A  // 1 Byte
#define EEPROM_RESERVED 0x44B  // next position

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
#define GET_EXPERT_FLAGS() (eeprom_read_byte((const uint8_t*)EEPROM_PID_FLAGS))
#define SET_EXPERT_FLAGS(n) do { eeprom_write_byte((uint8_t*)EEPROM_PID_FLAGS, n); } while(0)
#define GET_HEATER_TIMEOUT() (eeprom_read_byte((const uint8_t*)EEPROM_HEATER_TIMEOUT))
#define SET_HEATER_TIMEOUT(n) do { eeprom_write_byte((uint8_t*)EEPROM_HEATER_TIMEOUT, n); } while(0)
#define GET_END_RETRACT() (eeprom_read_float((const float*)EEPROM_END_RETRACT))
#define SET_END_RETRACT(n) do { eeprom_write_float((float*)EEPROM_END_RETRACT, n); } while(0)
#define GET_HEATER_CHECK_TEMP() (eeprom_read_byte((const uint8_t*)EEPROM_HEATER_CHECK_TEMP))
#define SET_HEATER_CHECK_TEMP(n) do { eeprom_write_byte((uint8_t*)EEPROM_HEATER_CHECK_TEMP, n); } while(0)
#define GET_HEATER_CHECK_TIME() (eeprom_read_byte((const uint8_t*)EEPROM_HEATER_CHECK_TIME))
#define SET_HEATER_CHECK_TIME(n) do { eeprom_write_byte((uint8_t*)EEPROM_HEATER_CHECK_TIME, n); } while(0)
#define GET_MOTOR_CURRENT_E2() (eeprom_read_word((const uint16_t*)EEPROM_MOTOR_CURRENT_E2))
#define SET_MOTOR_CURRENT_E2(n) do { eeprom_write_word((uint16_t*)EEPROM_MOTOR_CURRENT_E2, n); } while(0)
#define GET_STEPS_E2() (eeprom_read_float((const float*)EEPROM_STEPS_E2))
#define SET_STEPS_E2(n) do { eeprom_write_float((float*)EEPROM_STEPS_E2, n); } while(0)
#define GET_AXIS_DIRECTION() (eeprom_read_byte((const uint8_t*)EEPROM_AXIS_DIRECTION))
#define SET_AXIS_DIRECTION(n) do { eeprom_write_byte((uint8_t*)EEPROM_AXIS_DIRECTION, n); } while(0)

// UI Mode
#define UI_MODE_STANDARD  0
#define UI_MODE_EXPERT    1

#define UI_BEEP_SHORT    32
#define UI_BEEP_OFF      64

// SLEEP/LCD STATE
#define SLEEP_LED_DIMMED     1
#define SLEEP_LED_OFF        2
#define SLEEP_LCD_DIMMED     4
#define SLEEP_COOLING        8
#define SLEEP_SERIAL_CMD    16
#define SLEEP_SERIAL_SCREEN 32

#define SLEEP_UPDATE_LED   128

// control flags
#define FLAG_PID_NOZZLE      1
#define FLAG_PID_BED         2
#define FLAG_SWAP_EXTRUDERS  4
#define FLAG_RESERVED_3      8
#define FLAG_RESERVED_4     16
#define FLAG_RESERVED_5     32
#define FLAG_RESERVED_6     64
#define FLAG_RESERVED_7    128

#define LED_DIM_TIME 0		    // 0 min -> off
#define LED_DIM_MAXTIME 240		// 240 min

extern uint8_t ui_mode;
extern uint16_t lcd_timeout;
extern uint8_t lcd_contrast;
extern uint8_t led_sleep_glow;
extern uint8_t lcd_sleep_contrast;
extern uint8_t expert_flags;
extern float end_of_print_retraction;
extern uint16_t led_timeout;
extern uint8_t led_sleep_brightness;
extern uint8_t heater_check_temp;
extern uint8_t heater_check_time;
extern uint8_t sleep_state;
extern uint8_t axis_direction;

#if EXTRUDERS > 1
extern float pid2[3];
#endif
#if EXTRUDERS > 1 && defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
extern uint16_t motor_current_e2;
#endif

FORCE_INLINE bool pidTempBed() { return (expert_flags & FLAG_PID_BED); }

#if EXTRUDERS > 1
FORCE_INLINE bool swapExtruders() { return (expert_flags & FLAG_SWAP_EXTRUDERS); }
#endif

#define WORD_SETTING(n) (*(uint16_t*)&lcd_cache[(n) * sizeof(uint16_t)])
#define FLOAT_SETTING(n) (*(float*)&lcd_cache[(n) * sizeof(float)])

#define HAS_SERIAL_CMD (sleep_state & SLEEP_SERIAL_CMD)

#endif //PREFERENCES_H
