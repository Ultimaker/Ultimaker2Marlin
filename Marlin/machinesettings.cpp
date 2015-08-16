
#include "machinesettings.h"
#include "Marlin.h"
#include "temperature.h"

MachineSettings::MachineSettings()
{
    for (uint8_t i = 0; i<MAX_MACHINE_SETTINGS; ++i)
    {
        settings[i] = 0;
    }
}

MachineSettings::~MachineSettings()
{
    for (uint8_t i = 0; i<MAX_MACHINE_SETTINGS; ++i)
    {
        delete settings[i];
        settings[i] = 0;
    }
}

// store settings
bool MachineSettings::store(uint8_t index)
{
    if (index >= MAX_MACHINE_SETTINGS)
    {
        return false;
    }
    if (!settings[index])
    {
        if (freeMemory() > 200)
        {
            settings[index] = new t_machinesettings;
        }
        else
        {
            return false;
        }
    }
    settings[index]->feedmultiply = feedmultiply;
#if TEMP_SENSOR_BED != 0
    settings[index]->BedTemperature = target_temperature_bed;
#endif // TEMP_SENSOR_BED
    settings[index]->fanSpeed = fanSpeed;
    for (int i=0; i<EXTRUDERS; i++)
    {
        settings[index]->HotendTemperature[i] = target_temperature[i];
        settings[index]->extrudemultiply[i] = extrudemultiply[i];
    }
    for (int i=0; i<NUM_AXIS; i++)
    {
        settings[index]->max_acceleration_units_per_sq_second[i] = max_acceleration_units_per_sq_second[i];
        settings[index]->max_feedrate[i] = max_feedrate[i];
    }
    settings[index]->acceleration = acceleration;
    settings[index]->minimumfeedrate = minimumfeedrate;
    settings[index]->mintravelfeedrate = mintravelfeedrate;
    settings[index]->minsegmenttime = minsegmenttime;
    settings[index]->max_xy_jerk = max_xy_jerk;
    settings[index]->max_z_jerk = max_z_jerk;
    settings[index]->max_e_jerk = max_e_jerk;

    return true;
}

// recall settings
bool MachineSettings::recall(uint8_t index)
{
    if (index >= MAX_MACHINE_SETTINGS)
    {
        return false;
    }
    if (!settings[index])
    {
        return false;
    }

    feedmultiply = settings[index]->feedmultiply;
#if TEMP_SENSOR_BED != 0
    target_temperature_bed = settings[index]->BedTemperature;
#endif // TEMP_SENSOR_BED
    fanSpeed = settings[index]->fanSpeed;
    for (int i=0; i<EXTRUDERS; i++)
    {
      target_temperature[i] = settings[index]->HotendTemperature[i];
      extrudemultiply[i] = settings[index]->extrudemultiply[i];
    }
    for (int i=0; i<NUM_AXIS; i++)
    {
      max_acceleration_units_per_sq_second[i] = settings[index]->max_acceleration_units_per_sq_second[i];
      max_feedrate[i] = settings[index]->max_feedrate[i];
    }
    acceleration = settings[index]->acceleration;
    minimumfeedrate = settings[index]->minimumfeedrate;
    mintravelfeedrate = settings[index]->mintravelfeedrate;
    minsegmenttime = settings[index]->minsegmenttime;
    max_xy_jerk = settings[index]->max_xy_jerk;
    max_z_jerk = settings[index]->max_z_jerk;
    max_e_jerk = settings[index]->max_e_jerk;

    delete settings[index];
    settings[index] = 0;

    return true;
}
