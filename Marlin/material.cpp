#include "material.h"

struct materialSettings material[EXTRUDERS];

void materialSettings::set_material_from_eeprom(uint8_t nr) {
  this->temperature = eeprom_read_word(EEPROM_MATERIAL_TEMPERATURE_OFFSET(nr));
#if TEMP_SENSOR_BED != 0
  this->bed_temperature = eeprom_read_word(EEPROM_MATERIAL_BED_TEMPERATURE_OFFSET(nr));
#endif
  this->flow = eeprom_read_word(EEPROM_MATERIAL_FLOW_OFFSET(nr));

  this->fan_speed = eeprom_read_byte(EEPROM_MATERIAL_FAN_SPEED_OFFSET(nr));
  this->diameter = eeprom_read_float(EEPROM_MATERIAL_DIAMETER_OFFSET(nr));

  if (this->temperature > HEATER_0_MAXTEMP - 15)
      this->temperature = HEATER_0_MAXTEMP - 15;
#if TEMP_SENSOR_BED != 0
  if (this->bed_temperature > BED_MAXTEMP - 15)
      this->bed_temperature = BED_MAXTEMP - 15;
#endif
  this->change_temperature = eeprom_read_word(EEPROM_MATERIAL_CHANGE_TEMPERATURE(nr));
  this->change_preheat_wait_time = eeprom_read_byte(EEPROM_MATERIAL_CHANGE_WAIT_TIME(nr));
  if (this->change_temperature < 10)
      this->change_temperature = this->temperature;
}
