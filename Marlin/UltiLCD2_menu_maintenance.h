#ifndef ULTI_LCD2_MENU_MAINTENANCE_H
#define ULTI_LCD2_MENU_MAINTENANCE_H

void lcd_menu_maintenance();
#if TEMP_SENSOR_BED != 0
void lcd_menu_maintenance_advanced_bed_heatup();
#endif

void lcd_menu_maintenance_advanced();
void lcd_menu_maintenance_advanced_heatup();
void lcd_menu_maintenance_led();
void lcd_menu_maintenance_extrude();
void lcd_menu_maintenance_retraction();
void lcd_menu_advanced_version();
void lcd_menu_advanced_stats();
void lcd_menu_maintenance_motion();
void lcd_menu_advanced_factory_reset();

#endif//ULTI_LCD2_MENU_MAINTENANCE_H
