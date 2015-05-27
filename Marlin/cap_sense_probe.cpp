#include "Marlin.h"
#include "i2c_capacitance.h"
#include "cap_sense_probe.h"
#include "planner.h"
#include "stepper.h"
#include "temperature.h"

static const int sample_count = 100;
static uint16_t samples[sample_count];

static uint16_t captureSample()
{
    uint16_t value;
    static uint16_t previous_value = 0;
    i2cCapacitanceStart();
    while(true)
    {
        if (i2cCapacitanceDone(value))
        {
            if (value != previous_value)
            {
                previous_value = value;
                return value;
            }
            i2cCapacitanceStart();
            previous_value = value;
        }
        manage_heater();
        manage_inactivity();
    }
}

static uint16_t calculateAverage()
{
    uint32_t average = 0;
    for(uint8_t n=0; n<sample_count; n++)
        average += samples[n];
    return average / sample_count;
}

static uint16_t calculatedWeightedAverage()
{
    uint16_t average = calculateAverage();
    uint32_t standard_deviation = 0;
    for(uint8_t n=0; n<sample_count; n++)
    {
        standard_deviation += (samples[n] - average) * (samples[n] - average);
    }
    standard_deviation /= sample_count;
    standard_deviation = sqrt(standard_deviation);
    
    uint32_t weighted_average = 0;
    uint16_t count = 0;
    for(uint8_t n=0; n<sample_count; n++)
    {
        if (abs(samples[n] - average) <= standard_deviation)
        {
            count += 1;
            weighted_average += samples[n];
        }
    }
    return weighted_average / count;
}

float probeWithCapacitiveSensor(float x, float y)
{
    const int diff_average_count = 6;

    float feedrate = 0.2;
    
    float z_target = 0.0;
    float z_distance = 5.0;

    float z_position = 0.0;
    
    plan_buffer_line(x, y, z_target + z_distance, current_position[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
    st_synchronize();
    plan_buffer_line(x, y, z_target - z_distance, current_position[E_AXIS], feedrate, active_extruder);

    uint16_t previous_sensor_average = 0;
    int16_t diff_average = 0;
    int16_t diff_history[diff_average_count];
    uint8_t diff_history_pos = 0;

    for(uint8_t n=0; n<diff_average_count; n++)
        diff_history[n] = 0;
    
    uint8_t noise_measure_delay = 20;

    while(blocks_queued())
    {
        for(uint8_t n=0; n<sample_count; n++)
        {
            samples[n] = captureSample();
            z_position += float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS];
        }
        z_position /= sample_count;
        
        uint16_t average = calculatedWeightedAverage();
        MSerial.print(z_position);
        MSerial.print(' ');
        MSerial.println(float(average) / 100.0);
        
        if (previous_sensor_average == 0)
        {
            previous_sensor_average = average;
        }
        else
        {
            int16_t diff = average - previous_sensor_average;
            
            diff_average -= diff_history[diff_history_pos];
            diff_average += diff;
            diff_history[diff_history_pos] = diff;
            diff_history_pos ++;
            if (diff_history_pos >= diff_average_count)
                diff_history_pos = 0;
            
            previous_sensor_average = average;
            
            if (noise_measure_delay > 0)
            {
                noise_measure_delay --;
            }
            else
            {
                if (diff * diff_average_count * 2 < diff_average)
                {
                    quickStop();
                    //TOFIX: quickStop should update the planner step position instead of the code here.
                    current_position[Z_AXIS] = float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS];
                    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
                }
            }
        }
    }
    
    return z_position;
}
