#ifndef ULTI_LCD2_MENU_MAINTENANCE_H
#define ULTI_LCD2_MENU_MAINTENANCE_H

void homeHead();
void homeBed();
void homeAll();
void lcd_menu_maintenance();
#if TEMP_SENSOR_BED != 0
void lcd_menu_maintenance_advanced_bed_heatup();
#endif

void lcd_menu_maintenance_advanced();
void start_move_material();

#if EXTRUDERS > 1
void lcd_dual_move_material();
#endif // EXTRUDERS


#endif//ULTI_LCD2_MENU_MAINTENANCE_H
