#ifndef ULTI_LCD2_MENU_MAINTENANCE_H
#define ULTI_LCD2_MENU_MAINTENANCE_H

void lcd_menu_maintenance();
#if TEMP_SENSOR_BED != 0
void lcd_menu_maintenance_advanced_bed_heatup();
#endif

#endif//ULTI_LCD2_MENU_MAINTENANCE_H
