#include <avr/eeprom.h>
#include "Marlin.h"
#include "planner.h"
#include "lifetime_stats.h"

//Random number to verify if the lifetime has actually been written to the EEPROM already
#define LIFETIME_MAGIC 0x2624BA15

//EEPROM has a 100.000 erase cycles guarantee. By writing once a hour we get about 11 years of continues service. Which should be enough to last a lifetime.
#define MILLIS_MINUTE (1000L * 60L)
#define LIFETIME_SAVEINTERVAL (MILLIS_MINUTE * 120L)

// minimum printing time to save statistics (milliseconds)
#define MIN_PRINTTIME (MILLIS_MINUTE * 30L)

//Normal configuration from ConfigurationStore.cpp is stored at offset 100 (not 0x100), and has an undefined length.
//Material profiles are stored at 0x800 and is currently 385 bytes long.
//Storing the lifetime stats at 0x700 gives 256 bytes of storage that should be safe to use.
#define LIFETIME_EEPROM_OFFSET 0x700

static unsigned long startup_millis;
static unsigned long minute_counter_millis;
static unsigned long next_save_millis;
static float last_e_pos;
static float accumulated_e_diff;

unsigned long lifetime_minutes;
unsigned long lifetime_print_minutes;
unsigned long lifetime_print_centimeters;
unsigned long triptime_minutes;
unsigned long triptime_print_minutes;
unsigned long triptime_print_centimeters;
static bool is_printing;

static void load_lifetime_stats();
static void save_lifetime_stats();

void lifetime_stats_init()
{
    startup_millis = millis();
    next_save_millis = startup_millis + LIFETIME_SAVEINTERVAL;
    minute_counter_millis = startup_millis + MILLIS_MINUTE;
    is_printing = false;
    last_e_pos = current_position[E_AXIS];

    load_lifetime_stats();
}

void lifetime_stats_tick()
{
    unsigned long m = millis();

    //Every minute, increase the minute counters that are active.
    if (minute_counter_millis < m)
    {
        minute_counter_millis += MILLIS_MINUTE;

        lifetime_minutes++;
        triptime_minutes++;
        if (is_printing)
        {
            lifetime_print_minutes++;
            triptime_print_minutes++;

            float diff = current_position[E_AXIS] - last_e_pos;
            if (diff > 0 && diff < 60 * 30)
            {
                accumulated_e_diff += diff * volume_to_filament_length[active_extruder];
                while(accumulated_e_diff > 10.0)
                {
                    lifetime_print_centimeters ++;
                    triptime_print_centimeters ++;
                    accumulated_e_diff -= 10.0;
                }
            }
            last_e_pos = current_position[E_AXIS];
        }
    }

    //Every two hour, save the data to EEPROM.
    if (next_save_millis < m)
    {
        next_save_millis = m + LIFETIME_SAVEINTERVAL;
        save_lifetime_stats();
    }
}

void lifetime_stats_print_start()
{
    is_printing = true;
    last_e_pos = current_position[E_AXIS];
    accumulated_e_diff = 0;
}

void lifetime_stats_print_end()
{
    is_printing = false;
    if ((stoptime-starttime) > MIN_PRINTTIME)
    {
        save_lifetime_stats();
    }
}

static void load_lifetime_stats()
{
    unsigned long magic = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 0));
    if (magic == LIFETIME_MAGIC)
    {
        lifetime_minutes = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 4));
        lifetime_print_minutes = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 8));
        lifetime_print_centimeters = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 12));
        triptime_minutes = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 16));
        triptime_print_minutes = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 20));
        triptime_print_centimeters = eeprom_read_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 24));
    }else{
        lifetime_minutes = 0;
        lifetime_print_minutes = 0;
        lifetime_print_centimeters = 0;
        triptime_minutes = 0;
        triptime_print_minutes = 0;
        triptime_print_centimeters = 0;
    }
}

static void save_lifetime_stats()
{
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 0), LIFETIME_MAGIC);
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 4), lifetime_minutes);
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 8), lifetime_print_minutes);
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 12), lifetime_print_centimeters);
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 16), triptime_minutes);
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 20), triptime_print_minutes);
    eeprom_write_dword((uint32_t*)(LIFETIME_EEPROM_OFFSET + 24), triptime_print_centimeters);
}
