#ifndef TINKERGNOME_H
#define TINKERGNOME_H

#include "temperature.h"
#include "UltiLCD2_menu_utils.h"

extern float recover_height;
extern float recover_position[NUM_AXIS];
extern int recover_temperature[EXTRUDERS];

void tinkergnome_init();
void lcd_menu_maintenance_expert();
void lcd_menu_print_heatup_tg();
void lcd_menu_printing_tg();
void lcd_menu_move_axes();
void manage_led_timeout();
void manage_encoder_position(int8_t encoder_pos_interrupt);
void lcd_menu_expert_extrude();
void lcd_extrude_quit_menu();
void recover_start_print(const char *cmd);
void lcd_menu_recover_init();
void lcd_menu_expert_recover();
void reset_printing_state();
void endofprint_retract_store();


FORCE_INLINE void lcd_print_tune_nozzle0() { lcd_tune_value(target_temperature[0], 0, get_maxtemp(0) - 15); }
#if EXTRUDERS > 1
FORCE_INLINE void lcd_print_tune_nozzle1() { lcd_tune_value(target_temperature[1], 0, get_maxtemp(1) - 15); }
#endif
#if TEMP_SENSOR_BED != 0
FORCE_INLINE void lcd_print_tune_bed() { lcd_tune_value(target_temperature_bed, 0, BED_MAXTEMP - 15); }
#endif
#endif //TINKERGNOME_H
