#ifndef ULTI_LCD2_MENU_MATERIAL_H
#define ULTI_LCD2_MENU_MATERIAL_H

#define FILAMENT_REVERSAL_LENGTH      (FILAMANT_BOWDEN_LENGTH + 50)
#define FILAMENT_REVERSAL_SPEED       100
#define FILAMENT_LONG_MOVE_ACCELERATION 30

#define FILAMENT_FORWARD_LENGTH       (FILAMANT_BOWDEN_LENGTH - 50)
#define FILAMENT_INSERT_SPEED         2     //Initial insert speed to grab the filament.
#define FILAMENT_INSERT_FAST_SPEED    100   //Speed during the forward length
#define FILAMENT_INSERT_EXTRUDE_SPEED 2     //Final speed when extruding

void lcd_menu_material();
void lcd_change_to_menu_change_material(menuFunc_t return_menu);
void lcd_change_to_menu_insert_material(menuFunc_t return_menu);
bool lcd_material_verify_material_settings();
void lcd_material_reset_defaults();
void lcd_material_set_material(uint8_t nr, uint8_t e);
void lcd_material_store_material(uint8_t nr);
void lcd_material_read_current_material();
void lcd_material_store_current_material();

#endif//ULTI_LCD2_MENU_MATERIAL_H
