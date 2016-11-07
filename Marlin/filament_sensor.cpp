
#include "filament_sensor.h"
#if defined(FILAMENT_SENSOR_PIN) && (FILAMENT_SENSOR_PIN > -1)
  #include "fastio.h"
  #include "cardreader.h"
  #include "UltiLCD2_menu_print.h"
  #include "UltiLCD2_hi_lib.h"
#endif
#include "UltiLCD2_menu_utils.h"

void filament_sensor_init()
{
#if defined(FILAMENT_SENSOR_PIN) && (FILAMENT_SENSOR_PIN > -1)
  SET_INPUT(FILAMENT_SENSOR_PIN);
  // enable pullup resistor
  WRITE(FILAMENT_SENSOR_PIN, HIGH);
#endif
}

bool checkFilamentSensor()
{
#if defined(FILAMENT_SENSOR_PIN) && (FILAMENT_SENSOR_PIN > -1)

    bool bResult = READ(FILAMENT_SENSOR_PIN);

    // filament sensor pin is pulled down to LOW when a problem is detected
    if(!bResult)
    {
        // pause print only if:
        //    - printer is printing
        //    - printing is not already paused

        if (!card.pause() && (card.sdprinting() || commands_queued()))
        {
            SERIAL_ERROR_START;
            SERIAL_ERRORLNPGM("Material transport issue detected.");

            SERIAL_ECHO_START
            SERIAL_ECHOLNPGM("Print paused. Check feeder.");

            lcd_lib_beep();

            // pause print
            lcd_print_pause();
        }
    }

    return bResult;
#else
  return true;
#endif
}

void lcd_menu_filament_outage()
{
#if defined(FILAMENT_SENSOR_PIN) && (FILAMENT_SENSOR_PIN > -1)
    lcd_info_screen(lcd_change_to_previous_menu, NULL, PSTR("CONTINUE"));

    lcd_lib_draw_string_centerP(10, PSTR("Material transport"));
    lcd_lib_draw_string_centerP(20, PSTR("issue detected."));
    lcd_lib_draw_string_centerP(30, PSTR("Print paused."));
    lcd_lib_draw_string_centerP(40, PSTR("Check feeder."));

    LED_GLOW_ERROR
    lcd_lib_update_screen();
#else
    lcd_change_to_previous_menu();
#endif
}
