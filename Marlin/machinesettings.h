#ifndef MACHINESETTINGS_H
#define MACHINESETTINGS_H

#include "Configuration.h"

#define MAX_MACHINE_SETTINGS 10

class MachineSettings {
  public:
    // constructor
    MachineSettings ();

    // destructor
    ~MachineSettings ();

    // store settings
    bool store(uint8_t index);

    // recall settings
    bool recall(uint8_t index);

	bool has_saved_settings(uint8_t index) const { return (index < MAX_MACHINE_SETTINGS) ? settings[index] : false; }

  private:
    // the structure of each setting
    typedef struct {
	  int feedmultiply;
	  int HotendTemperature[EXTRUDERS];
#if TEMP_SENSOR_BED != 0
	  int BedTemperature;
#endif
	  uint8_t fanSpeed;
	  int extrudemultiply[EXTRUDERS];
	  long max_acceleration_units_per_sq_second[NUM_AXIS];
	  float max_feedrate[NUM_AXIS];
	  float acceleration;
	  float minimumfeedrate;
	  float mintravelfeedrate;
	  long minsegmenttime;
	  float max_xy_jerk;
	  float max_z_jerk;
	  float max_e_jerk;
	} t_machinesettings;

    t_machinesettings *settings[MAX_MACHINE_SETTINGS];
};

#endif
