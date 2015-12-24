#include <avr/pgmspace.h>

#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "Marlin.h"
#include "language.h"
#include "cardreader.h"
#include "temperature.h"
#include "lifetime_stats.h"
#include "filament_sensor.h"
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_maintenance.h"
#include "UltiLCD2_menu_utils.h"
#include "UltiLCD2_menu_prefs.h"
#include "preferences.h"
#include "tinkergnome.h"

uint8_t lcd_cache[LCD_CACHE_SIZE];
#define LCD_CACHE_NR_OF_FILES() lcd_cache[(LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))]
#define LCD_CACHE_TYPE(n) lcd_cache[LCD_CACHE_COUNT + (n)]
#define LCD_DETAIL_CACHE_ID() lcd_cache[LCD_DETAIL_CACHE_START]

unsigned long predictedTime = 0;

void lcd_menu_print_heatup();
static void lcd_menu_print_printing();
static void lcd_menu_print_error_sd();
static void lcd_menu_print_error_position();
static void lcd_menu_print_classic_warning();
static void lcd_menu_print_tune_retraction();

static bool primed = false;
static bool pauseRequested = false;

void lcd_clear_cache()
{
    for(uint8_t n=0; n<LCD_CACHE_COUNT; n++)
        LCD_CACHE_ID(n) = 0xFF;
    LCD_DETAIL_CACHE_ID() = 0;
    LCD_CACHE_NR_OF_FILES() = 0xFF;
}

void abortPrint()
{
    postMenuCheck = NULL;
    clear_command_queue();
    // enquecommand_P(PSTR("M401"));
    quickStop();

    if (primed || card.sdprinting)
    {
        // retract
        enquecommand_P(PSTR("G10"));

        char buffer[32] = {0};
        // set up the end of print retraction
        sprintf_P(buffer, PSTR("G92 E%i"), int((end_of_print_retraction / volume_to_filament_length[active_extruder])+0.5f));
        enquecommand(buffer);
        // perform the retraction at the standard retract speed
        sprintf_P(buffer, PSTR("G1 F%i E0"), int(retract_feedrate));
        enquecommand(buffer);

        // no longer primed
        primed = false;
    }

    if (card.sdprinting)
    {
    	// we're not printing any more
        card.sdprinting = false;
        if (!card.pause)
        {
            recover_height = current_position[Z_AXIS];
        }
    }

    if (current_position[Z_AXIS] > max_pos[Z_AXIS] - 30)
    {
        enquecommand_P(PSTR("G28 X0 Y0"));
        enquecommand_P(PSTR("G28 Z0"));
    }
    else
    {
        enquecommand_P(PSTR("G28"));
    }
    enquecommand_P(PSTR("M84"));

    // finish all moves
    st_synchronize();

    doCooldown();

    //If we where paused, make sure we abort that pause. Else strange things happen: https://github.com/Ultimaker/Ultimaker2Marlin/issues/32
    stoptime=millis();
    lifetime_stats_print_end();
    card.pause = false;
    pauseRequested = false;
    printing_state  = PRINT_STATE_NORMAL;
    if (led_mode == LED_MODE_WHILE_PRINTING)
        analogWrite(LED_PIN, 0);
}

static void userAbortPrint()
{
    abortPrint();
    sleep_state &= ~SLEEP_LED_OFF;
    menu.return_to_main();
}

static void checkPrintFinished()
{
    if ((printing_state != PRINT_STATE_RECOVER) && (printing_state != PRINT_STATE_START) && !card.sdprinting && !is_command_queued())
    {
        abortPrint();
        recover_height = 0.0f;
        sleep_state |= SLEEP_COOLING;
        menu.replace_menu(menu_t(lcd_menu_print_ready, MAIN_MENU_ITEM_POS(0)));
    }else if (position_error)
    {
        quickStop();
        abortPrint();
        sleep_state &= ~SLEEP_LED_OFF;
        menu.replace_menu(menu_t(lcd_menu_print_error_position, MAIN_MENU_ITEM_POS(0)));
    }else if (card.errorCode())
    {
        abortPrint();
        sleep_state &= ~SLEEP_LED_OFF;
        menu.replace_menu(menu_t(lcd_menu_print_error_sd, MAIN_MENU_ITEM_POS(0)));
    }
}

void doStartPrint()
{
    if (printing_state == PRINT_STATE_RECOVER)
    {
        return;
    }

    float priming_z = min(PRIMING_HEIGHT + recover_height, max_pos[Z_AXIS]-PRIMING_HEIGHT);

    // zero the extruder position
    current_position[E_AXIS] = 0.0;
    plan_set_e_position(0);

	// since we are going to prime the nozzle, forget about any G10/G11 retractions that happened at end of previous print
	retracted = false;
	primed = false;
	position_error = false;

    for(uint8_t e = 0; e<EXTRUDERS; e++)
    {
        // don't prime the extruder if it isn't used in the (Ulti)gcode
        // traditional gcode files typically won't have the Material lines at start, so we won't prime for those
        // Also, on dual/multi extrusion files, only prime the extruders that are used in the gcode-file.

#if EXTRUDERS == 2
        uint8_t index = (swapExtruders() ? e ^ 0x01 : e);
        if (!LCD_DETAIL_CACHE_MATERIAL(index))
            continue;
#else
        if (!LCD_DETAIL_CACHE_MATERIAL(e))
            continue;
#endif

        if (!primed)
        {
            // move to priming height
            current_position[Z_AXIS] = priming_z;
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], homing_feedrate[Z_AXIS], e);
            // note that we have primed, so that we know to de-prime at the end
            primed = true;
        }
        // undo the end-of-print retraction
        plan_set_e_position((- end_of_print_retraction) / volume_to_filament_length[e]);
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], END_OF_PRINT_RECOVERY_SPEED, e);

        // perform additional priming
        plan_set_e_position(-PRIMING_MM3);
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], (PRIMING_MM3_PER_SEC * volume_to_filament_length[e]), e);

#if EXTRUDERS > 1
        // for extruders other than the first one, perform end of print retraction
        if (e != active_extruder)
        {
            plan_set_e_position((volume_to_filament_length[e] + end_of_print_retraction) / volume_to_filament_length[e]);
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], retract_feedrate/60, e);
        }
#endif
    }

    if (printing_state == PRINT_STATE_START)
    {
        // move to the recover start position
        plan_set_e_position(recover_position[E_AXIS]);
        plan_buffer_line(recover_position[X_AXIS], recover_position[Y_AXIS], recover_position[Z_AXIS], recover_position[E_AXIS], min(homing_feedrate[X_AXIS], homing_feedrate[Z_AXIS]), active_extruder);
        for(int8_t i=0; i < NUM_AXIS; i++) {
            current_position[i] = recover_position[i];
        }
        // first recovering move
        enquecommand(LCD_CACHE_FILENAME(0));
    }

    printing_state = PRINT_STATE_NORMAL;

    postMenuCheck = checkPrintFinished;
    card.startFileprint();
    lifetime_stats_print_start();
    starttime = millis();
    stoptime = starttime;
    predictedTime = 0;
    sleep_state = 0x0;
}

static void userStartPrint()
{
    menu.return_to_main();
    if (printing_state == PRINT_STATE_RECOVER)
    {
        menu.add_menu(menu_t(lcd_menu_recover_init, lcd_menu_expert_recover, NULL, MAIN_MENU_ITEM_POS(3)));
    }
    else
    {
        recover_height = 0.0f;
#if EXTRUDERS > 1
        active_extruder = (swapExtruders() ? 1 : 0);
#else
        active_extruder = 0;
#endif // EXTRUDERS
        menu.add_menu(menu_t((ui_mode & UI_MODE_EXPERT) ? lcd_menu_printing_tg : lcd_menu_print_printing, MAIN_MENU_ITEM_POS(1)));
        doStartPrint();
    }
}

static void cardUpdir()
{
    card.updir();
}

static void lcd_sd_menu_filename_callback(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[32] = {0};
    memset(buffer, '\0', sizeof(buffer));
    if (nr == 0)
    {
        if (card.atRoot())
        {
            strcpy_P(buffer, PSTR("< RETURN"));
        }else{
            strcpy_P(buffer, PSTR("< BACK"));
        }
    }else{
        // buffer[0] = '\0';
        for(uint8_t idx=0; idx<LCD_CACHE_COUNT; ++idx)
        {
            if (LCD_CACHE_ID(idx) == nr)
            {
                strncpy(buffer, LCD_CACHE_FILENAME(idx), LONG_FILENAME_LENGTH-1);
                break;
            }
        }
        if (buffer[0] == '\0')
        {
            card.getfilename(nr - 1);
            if (card.longFilename[0])
            {
                strncpy(buffer, card.longFilename, LONG_FILENAME_LENGTH-1);
            } else {
                strncpy(buffer, card.filename, LONG_FILENAME_LENGTH-1);
            }
            if (!card.filenameIsDir)
            {
                if (strrchr(buffer, '.')) strrchr(buffer, '.')[0] = '\0';
            }

            uint8_t idx = nr % LCD_CACHE_COUNT;
            LCD_CACHE_ID(idx) = nr;
            strncpy(LCD_CACHE_FILENAME(idx), buffer, LONG_FILENAME_LENGTH-1);
            LCD_CACHE_TYPE(idx) = card.filenameIsDir ? 1 : 0;
            if (card.errorCode() && card.sdInserted)
            {
                //On a read error reset the file position and try to keep going. (not pretty, but these read errors are annoying as hell)
                card.clearError();
                LCD_CACHE_ID(idx) = 0xFF;
                card.longFilename[0] = '\0';
            }
        }
        buffer[20] = '\0';
    }
    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

void lcd_sd_menu_details_callback(uint8_t nr)
{
    if (nr == 0)
    {
        return;
    }
    for(uint8_t idx=0; idx<LCD_CACHE_COUNT; ++idx)
    {
        if (LCD_CACHE_ID(idx) == nr)
        {
            if (LCD_CACHE_TYPE(idx) == 1)
            {
                lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Folder"));
            }else{
                char buffer[64];
                if (LCD_DETAIL_CACHE_ID() != nr)
                {
                    card.getfilename(nr - 1);
                    if (card.errorCode())
                    {
                        card.clearError();
                        return;
                    }
                    LCD_DETAIL_CACHE_ID() = nr;
                    LCD_DETAIL_CACHE_TIME() = 0;
                    for(uint8_t e=0; e<EXTRUDERS; e++)
                        LCD_DETAIL_CACHE_MATERIAL(e) = 0;
                    card.openFile(card.filename, true);
                    if (card.isFileOpen())
                    {
                        for(uint8_t n=0;n<8;n++)
                        {
                            card.fgets(buffer, sizeof(buffer));
                            buffer[sizeof(buffer)-1] = '\0';
                            while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                            if (strncmp_P(buffer, PSTR(";TIME:"), 6) == 0)
                                LCD_DETAIL_CACHE_TIME() = strtol(buffer + 6, 0, 0);
                            else if (strncmp_P(buffer, PSTR(";MATERIAL:"), 10) == 0)
                            {
                                LCD_DETAIL_CACHE_MATERIAL(0) = strtol(buffer + 10, 0, 10);
                            }
#if EXTRUDERS > 1
                            else if (strncmp_P(buffer, PSTR(";MATERIAL2:"), 11) == 0)
                            {
                                LCD_DETAIL_CACHE_MATERIAL(1) = strtol(buffer + 11, 0, 10);
                            }
#endif
                        }
                    }
                    if (card.errorCode())
                    {
                        //On a read error reset the file position and try to keep going. (not pretty, but these read errors are annoying as hell)
                        card.clearError();
                        LCD_DETAIL_CACHE_ID() = 0xFF;
                    }
                }

                if (LCD_DETAIL_CACHE_TIME() > 0)
                {
                    char* c = buffer;
                    if (led_glow_dir)
                    {
                        strcpy_P(c, PSTR("Time ")); c += 5;
                        c = int_to_time_min(LCD_DETAIL_CACHE_TIME(), c);
                        if (LCD_DETAIL_CACHE_TIME() < 60)
                        {
                                strcat_P(c, PSTR("min"));
                        }
                        else
                        {
                                strcat_P(c, PSTR("h"));
                        }
                    }else{
                        strcpy_P(c, PSTR("Material ")); c += 9;
                        float length = float(LCD_DETAIL_CACHE_MATERIAL(0)) / (M_PI * (material[0].diameter / 2.0) * (material[0].diameter / 2.0));
                        if (length < 10000)
                            c = float_to_string2(length / 1000.0, c, PSTR("m"));
                        else
                            c = int_to_string(length / 1000.0, c, PSTR("m"));
#if EXTRUDERS > 1
                        if (LCD_DETAIL_CACHE_MATERIAL(1))
                        {
                            *c++ = '/';
                            length = float(LCD_DETAIL_CACHE_MATERIAL(1)) / (M_PI * (material[1].diameter / 2.0) * (material[1].diameter / 2.0));
                            if (length < 10000)
                                c = float_to_string2(length / 1000.0, c, PSTR("m"));
                            else
                                c = int_to_string(length / 1000.0, c, PSTR("m"));
                        }
#endif
                    }
                    lcd_lib_draw_string(3, BOTTOM_MENU_YPOS, buffer);
                }else{
                    lcd_lib_draw_stringP(3, BOTTOM_MENU_YPOS, PSTR("No info available"));
                }
            }
        }
    }
}

void lcd_menu_print_select()
{
    if (!card.sdInserted)
    {
        LED_GLOW
        lcd_lib_encoder_pos = MAIN_MENU_ITEM_POS(0);
        lcd_info_screen(NULL, lcd_change_to_previous_menu);
        lcd_lib_draw_string_centerP(15, PSTR("No SD-CARD!"));
        lcd_lib_draw_string_centerP(25, PSTR("Please insert card"));
        lcd_lib_update_screen();
        card.release();
        return;
    }
    if (!card.isOk())
    {
        lcd_info_screen(NULL, lcd_change_to_previous_menu);
        lcd_lib_draw_string_centerP(16, PSTR("Reading card..."));
        lcd_lib_update_screen();
        lcd_clear_cache();
        card.initsd();
        return;
    }

    if (LCD_CACHE_NR_OF_FILES() == 0xFF)
        LCD_CACHE_NR_OF_FILES() = card.getnrfilenames();
    if (card.errorCode())
    {
        LCD_CACHE_NR_OF_FILES() = 0xFF;
        return;
    }
    uint8_t nrOfFiles = LCD_CACHE_NR_OF_FILES();
    if (nrOfFiles == 0)
    {
        if (card.atRoot())
            lcd_info_screen(reset_printing_state, lcd_change_to_previous_menu, PSTR("OK"));
        else
            lcd_info_screen(lcd_menu_print_select, cardUpdir, PSTR("OK"));
        lcd_lib_draw_string_centerP(25, PSTR("No files found!"));
        lcd_lib_update_screen();
        lcd_clear_cache();
        return;
    }

    if (lcd_lib_button_pressed)
    {
        uint16_t selIndex = uint16_t(SELECTED_SCROLL_MENU_ITEM());
        if (selIndex == 0)
        {
            if (card.atRoot())
            {
                reset_printing_state();
                menu.return_to_previous();
            }else{
                lcd_clear_cache();
                lcd_lib_keyclick();
                card.updir();
            }
        }else{
            card.getfilename(selIndex - 1);
            if (!card.filenameIsDir)
            {
                //Start print
                sleep_state = 0x0;
            #if EXTRUDERS > 1
                active_extruder = (swapExtruders() ? 1 : 0);
            #else
                active_extruder = 0;
            #endif // EXTRUDERS
                card.openFile(card.filename, true);
                if (card.isFileOpen() && !is_command_queued())
                {
                    if (led_mode == LED_MODE_WHILE_PRINTING || led_mode == LED_MODE_BLINK_ON_DONE)
                        analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
                    if (!card.longFilename[0])
                        strncpy(card.longFilename, card.filename, LONG_FILENAME_LENGTH-1);
                    card.longFilename[20] = '\0';
                    if (strchr(card.longFilename, '.')) strchr(card.longFilename, '.')[0] = '\0';

                    char buffer[64];
                    card.fgets(buffer, sizeof(buffer));
                    buffer[sizeof(buffer)-1] = '\0';
                    if (strncmp_P(buffer, PSTR(";FLAVOR:UltiGCode"), 17) != 0)
                    {
                        card.fgets(buffer, sizeof(buffer));
                        buffer[sizeof(buffer)-1] = '\0';
                    }
                    card.setIndex(0);

                    fanSpeed = 0;
                    feedmultiply = 100;
                    current_nominal_speed = 0.0f;

                    lcd_clearstatus();
                    menu.return_to_main();

                    //reset all printing parameters to defaults
                    fanSpeedPercent = 100;
                    for(uint8_t e=0; e<EXTRUDERS; e++)
                    {
                        volume_to_filament_length[e] = 1.0;
                        extrudemultiply[e] = 100;
                        e_smoothed_speed[e] = 0.0f;
                    }

                    if (strncmp_P(buffer, PSTR(";FLAVOR:UltiGCode"), 17) == 0)
                    {
                        //New style GCode flavor without start/end code.
                        // Temperature settings, filament settings, fan settings, start and end-code are machine controlled.
#if TEMP_SENSOR_BED != 0
                        target_temperature_bed = 0;
#endif
                        fanSpeedPercent = 0;
                        for(uint8_t e=0; e<EXTRUDERS; ++e)
                        {
//                            SERIAL_ECHOPGM("MATERIAL_");
//                            SERIAL_ECHO(e+1);
//                            SERIAL_ECHOPGM(": ");
//                            SERIAL_ECHOLN(LCD_DETAIL_CACHE_MATERIAL(e));

#if EXTRUDERS == 2
                            uint8_t index = (swapExtruders() ? e^0x01 : e);
                            if (LCD_DETAIL_CACHE_MATERIAL(index) < 1)
                                continue;
#else
                            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1)
                                continue;
#endif

                            target_temperature[e] = 0;//material[e].temperature;
#if TEMP_SENSOR_BED != 0
                            target_temperature_bed = max(target_temperature_bed, material[e].bed_temperature);
#endif
                            fanSpeedPercent = max(fanSpeedPercent, material[e].fan_speed);
                            volume_to_filament_length[e] = 1.0 / (M_PI * (material[e].diameter / 2.0) * (material[e].diameter / 2.0));
                            extrudemultiply[e] = material[e].flow;
                        }

                        if (printing_state == PRINT_STATE_RECOVER)
                        {
                            menu.add_menu(menu_t(lcd_menu_recover_init, lcd_menu_expert_recover, NULL, MAIN_MENU_ITEM_POS(3)));
                        }
                        else
                        {
                            recover_height = 0.0f;
                        #if EXTRUDERS > 1
                            active_extruder = (swapExtruders() ? 1 : 0);
                        #else
                            active_extruder = 0;
                        #endif // EXTRUDERS
                            // move to heatup position
                            char buffer[32] = {0};
                            sprintf_P(buffer, PSTR("G1 F12000 X%i Y%i"), max(int(min_pos[X_AXIS]), 0)+5, max(int(min_pos[Y_AXIS]), 0)+5);
                            enquecommand_P(PSTR("G28"));
                            enquecommand(buffer);
                            printing_state = PRINT_STATE_NORMAL;

                            if (ui_mode & UI_MODE_EXPERT)
                                menu.add_menu(menu_t(lcd_menu_print_heatup_tg));
                            else
                                menu.add_menu(menu_t(lcd_menu_print_heatup));
                        }
                    }
                    else
                    {
                        //Classic gcode file
                        menu.add_menu(menu_t(lcd_menu_print_classic_warning, MAIN_MENU_ITEM_POS(0)));
                    }
                }
            }else{
                lcd_lib_keyclick();
                lcd_clear_cache();
                card.chdir(card.filename);
                SELECT_SCROLL_MENU_ITEM(0);
            }
            return;//Return so we do not continue after changing the directory or selecting a file. The nrOfFiles is invalid at this point.
        }
    }
    lcd_scroll_menu(PSTR("SD CARD"), nrOfFiles+1, lcd_sd_menu_filename_callback, lcd_sd_menu_details_callback);
    lcd_lib_update_screen();
}

void lcd_menu_print_heatup()
{
    lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));

#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > target_temperature_bed - TEMP_WINDOW*2)
    {
#endif
        for(uint8_t e=0; e<EXTRUDERS; ++e)
        {
#if EXTRUDERS == 2
            uint8_t index = (swapExtruders() ? e ^ 0x01 : e);
            if (LCD_DETAIL_CACHE_MATERIAL(index) < 1 || target_temperature[e] > 0)
                continue;
#else
            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] > 0)
                continue;
#endif
            target_temperature[e] = material[e].temperature;
            // printing_state = PRINT_STATE_START;
        }

#if TEMP_SENSOR_BED != 0
        if (current_temperature_bed >= target_temperature_bed - TEMP_WINDOW * 2 && !is_command_queued())
        {
#endif // TEMP_SENSOR_BED
            bool ready = true;
            for(uint8_t e=0; e<EXTRUDERS; e++)
                if (current_temperature[e] < target_temperature[e] - TEMP_WINDOW)
                    ready = false;

            if (ready)
            {
                doStartPrint();
                if (ui_mode & UI_MODE_EXPERT)
                {
                    menu.replace_menu(menu_t(lcd_menu_printing_tg, MAIN_MENU_ITEM_POS(1)), false);
                }else{
                    menu.replace_menu(menu_t(lcd_menu_print_printing), false);
                }
            }
#if TEMP_SENSOR_BED != 0
        }
    }
#endif // TEMP_SENSOR_BED

    uint8_t progress = 125;
    for(uint8_t e=0; e<EXTRUDERS; e++)
    {
#if EXTRUDERS == 2
        uint8_t index = (swapExtruders() ? e ^ 0x01 : e);
        if (((printing_state != PRINT_STATE_RECOVER) && (LCD_DETAIL_CACHE_MATERIAL(index) < 1)) || (target_temperature[e] < 1))
            continue;
#else
        if (((printing_state != PRINT_STATE_RECOVER) && (LCD_DETAIL_CACHE_MATERIAL(e) < 1)) || (target_temperature[e] < 1))
            continue;
#endif
        if (current_temperature[e] > 20)
            progress = min(progress, (current_temperature[e] - 20) * 125 / (target_temperature[e] - 20 - TEMP_WINDOW));
        else
            progress = 0;
    }
#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > 20)
        progress = min(progress, (current_temperature_bed - 20) * 125 / (target_temperature_bed - 20 - TEMP_WINDOW));
    else if (target_temperature_bed > current_temperature_bed - 20)
        progress = 0;
#endif

    if (progress < minProgress)
        progress = minProgress;
    else
        minProgress = progress;

    lcd_lib_draw_string_centerP(10, PSTR("Heating up..."));
    lcd_lib_draw_string_centerP(20, PSTR("Preparing to print:"));
    lcd_lib_draw_string_center(30, card.longFilename);

    lcd_progressbar(progress);

    lcd_lib_update_screen();
}

void lcd_change_to_menu_change_material_return()
{
    plan_set_e_position(current_position[E_AXIS]);
    setTargetHotend(material[active_extruder].temperature, active_extruder);
    menu.return_to_previous(false);
}

static void lcd_menu_print_printing()
{
    if (card.pause || isPauseRequested())
    {
        menu.add_menu(menu_t(lcd_select_first_submenu, lcd_menu_print_resume, NULL, MAIN_MENU_ITEM_POS(0)), true);
        if (!checkFilamentSensor())
        {
            menu.add_menu(menu_t(lcd_menu_filament_outage, MAIN_MENU_ITEM_POS(0)));
        }
    }
    else
    {
        lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_pause, lcd_select_first_submenu, PSTR("PAUSE"));
        uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
        char buffer[32] = {0};
        switch(printing_state)
        {
        default:
            lcd_lib_draw_string_centerP(20, pauseRequested ? PSTR("Pausing:") : PSTR("Printing:"));
            lcd_lib_draw_string_center(30, card.longFilename);
            break;
        case PRINT_STATE_HEATING:
            lcd_lib_draw_string_centerP(20, PSTR("Heating"));
            int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer, PSTR("C/")), PSTR("C"));
            lcd_lib_draw_string_center(30, buffer);
            break;
#if TEMP_SENSOR_BED != 0
        case PRINT_STATE_HEATING_BED:
            lcd_lib_draw_string_centerP(20, PSTR("Heating buildplate"));
            int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, buffer, PSTR("C/")), PSTR("C"));
            lcd_lib_draw_string_center(30, buffer);
            break;
#endif
        }
        float printTimeMs = (millis() - starttime);
        float printTimeSec = printTimeMs / 1000L;
        float totalTimeMs = float(printTimeMs) * float(card.getFileSize()) / float(card.getFilePos());
        static float totalTimeSmoothSec;
        totalTimeSmoothSec = (totalTimeSmoothSec * 999L + totalTimeMs / 1000L) / 1000L;
        if (isinf(totalTimeSmoothSec))
            totalTimeSmoothSec = totalTimeMs;

        if (LCD_DETAIL_CACHE_TIME() == 0 && printTimeSec < 60)
        {
            totalTimeSmoothSec = totalTimeMs / 1000;
            lcd_lib_draw_stringP(5, 10, PSTR("Time left unknown"));
        }else{
            unsigned long totalTimeSec;
            if (printTimeSec < LCD_DETAIL_CACHE_TIME() / 2)
            {
                float f = float(printTimeSec) / float(LCD_DETAIL_CACHE_TIME() / 2);
                if (f > 1.0)
                    f = 1.0;
                totalTimeSec = float(totalTimeSmoothSec) * f + float(LCD_DETAIL_CACHE_TIME()) * (1 - f);
            }else{
                totalTimeSec = totalTimeSmoothSec;
            }
            unsigned long timeLeftSec;
            if (printTimeSec > totalTimeSec)
                timeLeftSec = 1;
            else
                timeLeftSec = totalTimeSec - printTimeSec;
            int_to_time_string(timeLeftSec, buffer);
            lcd_lib_draw_stringP(5, 10, PSTR("Time left"));
            lcd_lib_draw_string(65, 10, buffer);
        }

        lcd_progressbar(progress);
        lcd_lib_update_screen();
    }
}

static void lcd_menu_print_error_sd()
{
    LED_GLOW_ERROR
    lcd_info_screen(NULL, lcd_return_to_main_menu, PSTR("RETURN TO MAIN"));

    lcd_lib_draw_string_centerP(10, PSTR("Error while"));
    lcd_lib_draw_string_centerP(20, PSTR("reading"));
    lcd_lib_draw_string_centerP(30, PSTR("SD-card!"));
    char buffer[12];
    strcpy_P(buffer, PSTR("Code:"));
    int_to_string(card.errorCode(), buffer+5);
    lcd_lib_draw_string_center(40, buffer);

    lcd_lib_update_screen();
}

static void lcd_menu_print_error_position()
{
    LED_GLOW_ERROR
    lcd_info_screen(NULL, lcd_return_to_main_menu, PSTR("RETURN TO MAIN"));

    lcd_lib_draw_string_centerP(15, PSTR("ERROR:"));
    lcd_lib_draw_string_centerP(25, PSTR("Tried printing out"));
    lcd_lib_draw_string_centerP(35, PSTR("of printing area"));

    lcd_lib_update_screen();
}

static void lcd_menu_print_classic_warning()
{
    lcd_question_screen(NULL, userStartPrint, PSTR("CONTINUE"), lcd_menu_print_select, lcd_remove_menu, PSTR("CANCEL"));

    lcd_lib_draw_string_centerP(10, PSTR("This file will"));
    lcd_lib_draw_string_centerP(20, PSTR("override machine"));
    lcd_lib_draw_string_centerP(30, PSTR("setting with setting"));
    lcd_lib_draw_string_centerP(40, PSTR("from the slicer."));

    lcd_lib_update_screen();
}

void lcd_menu_print_abort()
{
    if (pauseRequested)
    {
        lcd_print_pause();
    }
    LED_GLOW
    lcd_question_screen(lcd_menu_print_ready, userAbortPrint, PSTR("YES"), NULL, lcd_change_to_previous_menu, PSTR("NO"));

    lcd_lib_draw_string_centerP(20, PSTR("Abort the print?"));
    lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 - 4, 32, standbyGfx);

    lcd_lib_update_screen();
}

static void postPrintReady()
{
    sleep_state &= ~SLEEP_LED_OFF;
    if (led_mode == LED_MODE_BLINK_ON_DONE)
        analogWrite(LED_PIN, 0);
    menu.return_to_previous();
}

void lcd_menu_print_ready()
{
    if ((led_mode == LED_MODE_BLINK_ON_DONE) && !(sleep_state & SLEEP_LED_OFF))
        analogWrite(LED_PIN, (led_glow << 1) * int(led_brightness_level) / 100);

    lcd_info_screen(NULL, postPrintReady, PSTR("BACK TO MENU"));

    lcd_lib_draw_hline(3, 124, 13);
    // lcd_lib_draw_string_left(5, card.longFilename);

    char buffer[32] = {0};
    unsigned long t=(stoptime-starttime)/1000;

    char *c = buffer;
    strcpy_P(c, PSTR("Time ")); c += 5;
    c = int_to_time_min(t, c);
    if (t < 60)
    {
            strcat_P(c, PSTR("min"));
    }
    else
    {
            strcat_P(c, PSTR("h"));
    }
    lcd_lib_draw_string_center(5, buffer);

    c = buffer;
    for(uint8_t e=0; e<EXTRUDERS; e++)
        c = int_to_string(dsp_temperature[e], c, PSTR("C "));
#if TEMP_SENSOR_BED != 0
    int_to_string(dsp_temperature_bed, c, PSTR("C"));
#endif
    lcd_lib_draw_string_center(26, buffer);

    if (sleep_state & SLEEP_COOLING)
    {
#if TEMP_SENSOR_BED != 0
        if ((current_temperature[0] > 60) || (current_temperature_bed > 40))
#else
        if (current_temperature[0] > 60)
#endif // TEMP_SENSOR_BED
        {
            lcd_lib_draw_string_centerP(16, PSTR("Printer cooling down"));

            int16_t progress = 124 - (current_temperature[0] - 60);
            if (progress < 0) progress = 0;
            if (progress > 124) progress = 124;

            if (progress < minProgress)
                progress = minProgress;
            else
                minProgress = progress;

            lcd_progressbar(progress);
        }
        else
        {
            sleep_state &= ~SLEEP_COOLING;
        }
    }
    else
    {
        if (sleep_state ^ SLEEP_LED_OFF)
        {
            LED_GLOW
        }
        lcd_lib_draw_string_center(16, card.longFilename);
        lcd_lib_draw_string_centerP(40, PSTR("Print finished"));
    }
    lcd_lib_update_screen();
}

static void tune_item_callback(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    uint8_t index = 0;
    char buffer[32] = {0};
    if (index++ == nr)
        strcpy_P(buffer, PSTR("< RETURN"));
//    else if (index++ == nr)
//        strcpy_P(c, PSTR("Abort"));
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("Speed"));
    else if (index++ == nr)
#if EXTRUDERS > 1
        strcpy_P(buffer, PSTR("Temperature 1"));
#else
        strcpy_P(buffer, PSTR("Temperature"));
#endif
#if EXTRUDERS > 1
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("Temperature 2"));
#endif
#if TEMP_SENSOR_BED != 0
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("Buildplate temp."));
#endif
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("Fan speed"));
    else if (index++ == nr)
#if EXTRUDERS > 1
        strcpy_P(buffer, PSTR("Material flow 1"));
#else
        strcpy_P(buffer, PSTR("Material flow"));
#endif
#if EXTRUDERS > 1
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("Material flow 2"));
#endif
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("Retraction"));
    else if (index++ == nr)
        strcpy_P(buffer, PSTR("LED Brightness"));
    else if ((ui_mode & UI_MODE_EXPERT) && card.sdprinting && card.pause && (index++ == nr))
        strcpy_P(buffer, PSTR("Move material"));
    else if (ui_mode & UI_MODE_EXPERT)
        strcpy_P(buffer, PSTR("Sleep timer"));
    else
        strcpy_P(buffer, PSTR("???"));

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void tune_item_details_callback(uint8_t nr)
{
    char buffer[32] = {0};
    if (nr == 1)
        int_to_string(feedmultiply, buffer, PSTR("%"));
    else if (nr == 2)
    {
        int_to_string(target_temperature[0], int_to_string(dsp_temperature[0], buffer, PSTR("C/")), PSTR("C"));
    }
#if EXTRUDERS > 1
    else if (nr == 3)
    {
        int_to_string(target_temperature[1], int_to_string(dsp_temperature[1], buffer, PSTR("C/")), PSTR("C"));
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == 2 + EXTRUDERS)
    {
        int_to_string(target_temperature_bed, int_to_string(dsp_temperature_bed, buffer, PSTR("C/")), PSTR("C"));
    }
#endif
    else if (nr == 2 + BED_MENU_OFFSET + EXTRUDERS)
        int_to_string(int(fanSpeed) * 100 / 255, buffer, PSTR("%"));
    else if (nr == 3 + BED_MENU_OFFSET + EXTRUDERS)
        int_to_string(extrudemultiply[0], buffer, PSTR("%"));
#if EXTRUDERS > 1
    else if (nr == 4 + BED_MENU_OFFSET + EXTRUDERS)
        int_to_string(extrudemultiply[1], buffer, PSTR("%"));
#endif
    else if (nr == 4 + BED_MENU_OFFSET + 2*EXTRUDERS)
    {
        int_to_string(led_brightness_level, buffer, PSTR("%"));
//        if (led_mode == LED_MODE_ALWAYS_ON || led_mode == LED_MODE_WHILE_PRINTING || led_mode == LED_MODE_BLINK_ON_DONE)
//            analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
    }
    else
        return;
    lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
}

void lcd_menu_print_tune_heatup_nozzle0()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature[0] = int(target_temperature[0]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM);
        if (target_temperature[0] < 0)
            target_temperature[0] = 0;
        if (target_temperature[0] > get_maxtemp(0) - 15)
            target_temperature[0] = get_maxtemp(0) - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        menu.return_to_previous();

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Nozzle temperature:"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16] = {0};
    int_to_string(int(dsp_temperature[0]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[0]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 40, getHeaterPower(0));
    lcd_lib_update_screen();
}
#if EXTRUDERS > 1
void lcd_menu_print_tune_heatup_nozzle1()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature[1] = int(target_temperature[1]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM);
        if (target_temperature[1] < 0)
            target_temperature[1] = 0;
        if (target_temperature[1] > get_maxtemp(1) - 15)
            target_temperature[1] = get_maxtemp(1) - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_previous_menu();

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Nozzle2 temperature:"));
    lcd_lib_draw_string_centerP(BOTTOM_MENU_YPOS, PSTR("Click to return"));
    char buffer[16] = {0};
    int_to_string(int(dsp_temperature[1]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[1]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_draw_heater(LCD_GFX_WIDTH/2-2, 40, getHeaterPower(1));
    lcd_lib_update_screen();
}
#endif

void lcd_menu_print_tune()
{
    if (pauseRequested)
    {
        lcd_print_pause();
    }
    uint8_t len = 5 + BED_MENU_OFFSET + EXTRUDERS * 2;
    if (ui_mode & UI_MODE_EXPERT)
    {
        ++len; // sleep timer
        if (card.sdprinting && card.pause)
        {
            ++len; // move material
        }
    }

    lcd_scroll_menu(PSTR("TUNE"), len, tune_item_callback, tune_item_details_callback);
    if (lcd_lib_button_pressed)
    {
        uint8_t index(0);
        if (IS_SELECTED_SCROLL(index++))
        {
            menu.return_to_previous();
//        }else if (IS_SELECTED_SCROLL(index++))
//        {
//            menu.add_menu(menu_t(lcd_menu_print_abort));
        }else if (IS_SELECTED_SCROLL(index++))
            LCD_EDIT_SETTING(feedmultiply, "Print speed", "%", 10, 1000);
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_print_tune_heatup_nozzle0, 0));
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_print_tune_heatup_nozzle1, 0));
#endif
#if TEMP_SENSOR_BED != 0
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_maintenance_advanced_bed_heatup, 0));//Use the maintainace heatup menu, which shows the current temperature.
#endif
        else if (IS_SELECTED_SCROLL(index++))
            LCD_EDIT_SETTING_BYTE_PERCENT(fanSpeed, "Fan speed", "%", 0, 100);
        else if (IS_SELECTED_SCROLL(index++))
            LCD_EDIT_SETTING(extrudemultiply[0], "Material flow", "%", 10, 1000);
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(index++))
            LCD_EDIT_SETTING(extrudemultiply[1], "Material flow 2", "%", 10, 1000);
#endif
        else if (IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_print_tune_retraction, 0));
        else if (IS_SELECTED_SCROLL(index++))
            LCD_EDIT_SETTING(led_brightness_level, "Brightness", "%", 0, 100);
        else if ((ui_mode & UI_MODE_EXPERT) && card.sdprinting && card.pause && IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(NULL, lcd_menu_expert_extrude, lcd_extrude_quit_menu, 0)); // Move material
        else if ((ui_mode & UI_MODE_EXPERT) && IS_SELECTED_SCROLL(index++))
            menu.add_menu(menu_t(lcd_menu_sleeptimer));
    }
    lcd_lib_update_screen();
}

static void lcd_retraction_item(uint8_t nr, uint8_t offsetY, uint8_t flags)
{
    char buffer[32] = {0};
    if (nr == 0)
        strcpy_P(buffer, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(buffer, PSTR("Retract length"));
    else if (nr == 2)
        strcpy_P(buffer, PSTR("Retract speed"));
    else if (nr == 3)
        strcpy_P(buffer, PSTR("End of print length"));
#if EXTRUDERS > 1
    else if (nr == 4)
        strcpy_P(buffer, PSTR("Extruder change len"));
#endif
    else
        strcpy_P(buffer, PSTR("???"));

    lcd_draw_scroll_entry(offsetY, buffer, flags);
}

static void lcd_retraction_details(uint8_t nr)
{
    char buffer[32] = {0};
    if(nr == 1)
        float_to_string2(retract_length, buffer, PSTR("mm"));
    else if(nr == 2)
        int_to_string(retract_feedrate / 60 + 0.5, buffer, PSTR("mm/sec"));
    else if(nr == 3)
        float_to_string2(end_of_print_retraction, buffer, PSTR("mm"));
#if EXTRUDERS > 1
    else if(nr == 4)
        int_to_string(extruder_swap_retract_length, buffer, PSTR("mm"));
#endif
    else
        return;
    lcd_lib_draw_string(5, BOTTOM_MENU_YPOS, buffer);
}

static void lcd_menu_print_tune_retraction()
{
    lcd_scroll_menu(PSTR("RETRACTION"), 4 + (EXTRUDERS > 1 ? 1 : 0), lcd_retraction_item, lcd_retraction_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
            menu.return_to_previous();
        else if (IS_SELECTED_SCROLL(1))
            LCD_EDIT_SETTING_FLOAT001(retract_length, "Retract length", "mm", 0, 50);
        else if (IS_SELECTED_SCROLL(2))
            LCD_EDIT_SETTING_SPEED(retract_feedrate, "Retract speed", "mm/sec", 0, max_feedrate[E_AXIS] * 60);
        else if (IS_SELECTED_SCROLL(3))
            LCD_EDIT_SETTING_FLOAT001(end_of_print_retraction, "End of print retract", "mm", 0, 50);
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(4))
            LCD_EDIT_SETTING_FLOAT001(extruder_swap_retract_length, "Extruder change", "mm", 0, 50);
#endif
    }
    lcd_lib_update_screen();
}

void lcd_print_pause()
{
    if (card.sdprinting && !card.pause)
    {
        if (movesplanned() > 0 && commands_queued() < BUFSIZE)
        {
            card.pause = true;
            pauseRequested = false;
            recover_height = current_position[Z_AXIS];

            // move z up according to the current height - but minimum to z=70mm (above the gantry height)
            uint16_t zdiff = 0;
            if (current_position[Z_AXIS] < 70)
                zdiff = max(70 - floor(current_position[Z_AXIS]), 20);
            else if (current_position[Z_AXIS] < max_pos[Z_AXIS] - 60)
            {
                zdiff = 20;
            }
            else if (current_position[Z_AXIS] < max_pos[Z_AXIS] - 30)
            {
                zdiff = 2;
            }

            char buffer[32] = {0};
            sprintf_P(buffer, PSTR("M601 X%i Y%i Z%i L%f"), int(min_pos[X_AXIS])+5, int(min_pos[Y_AXIS])+5, zdiff, end_of_print_retraction);
            enquecommand(buffer);

            primed = false;
        }
        else if (!pauseRequested)
        {
            pauseRequested = true;
        }
    }
}

bool isPauseRequested()
{
    return pauseRequested;
}

void lcd_print_tune()
{
    // switch to tune menu
    menu.add_menu(menu_t(lcd_menu_print_tune));
}

void lcd_print_abort()
{
    // switch to abort menu
    menu.add_menu(menu_t(lcd_menu_print_abort, MAIN_MENU_ITEM_POS(1)));
}

static void lcd_menu_print_resume_ready()
{
    menu.return_to_previous();
    pauseRequested = false;
    card.pause = false;
    if (LCD_DETAIL_CACHE_MATERIAL(active_extruder))
        primed = true;
}

static void lcd_print_resume()
{
    if (card.sdprinting)
    {
        menu.replace_menu(menu_t(lcd_menu_print_resume_ready));
        check_preheat();
    }
    else
    {
        menu.return_to_previous();
    }
}

static void lcd_print_change_material()
{
    if (!movesplanned())
    {
        menu.add_menu(menu_t(lcd_change_to_menu_change_material_return), false);
        menu.add_menu(menu_t(lcd_menu_change_material_preheat));
    }
}

static void lcd_show_pause_menu()
{
    lcd_print_pause();
    menu.replace_menu(menu_t(lcd_select_first_submenu, lcd_menu_print_resume, NULL, MAIN_MENU_ITEM_POS(0)));
}

static const menu_t & get_pause_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_show_pause_menu);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_abort);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_tune);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_change_to_previous_menu);
    }
    return opt;
}

static void drawPauseSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if (nr == index++)
    {
        LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
                                , LCD_LINE_HEIGHT
                                , 52
                                , LCD_LINE_HEIGHT*4
                                , PSTR("PAUSE|")
                                , ALIGN_CENTER
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, pauseGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, pauseGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuString_P(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                                , LCD_LINE_HEIGHT
                                , 52
                                , LCD_LINE_HEIGHT*4
                                , PSTR("ABORT|")
                                , ALIGN_CENTER
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, standbyGfx);
        }
        else
        {
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, standbyGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT*2
                           , BOTTOM_MENU_YPOS
                           , 48
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_clear_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_draw_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT*2
                                , BOTTOM_MENU_YPOS
                                , 48
                                , LCD_CHAR_HEIGHT
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("BACK"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + 2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, backGfx);
        }
    }
}

void lcd_menu_print_pause()
{
    lcd_lib_clear();
    lcd_lib_draw_vline(64, 5, 46);
    lcd_lib_draw_hline(3, 124, 50);

    menu.process_submenu(get_pause_menuoption, 4);

    for (uint8_t index=0; index<4; ++index)
    {
        menu.drawSubMenu(drawPauseSubmenu, index);
    }
    lcd_lib_update_screen();
}

static const menu_t & get_resume_menuoption(uint8_t nr, menu_t &opt)
{
    uint8_t menu_index = 0;
    if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_resume);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_change_material);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_tune);
    }
    else if (nr == menu_index++)
    {
        opt.setData(MENU_NORMAL, lcd_print_abort);
    }
    return opt;
}

static void drawResumeSubmenu(uint8_t nr, uint8_t &flags)
{
    uint8_t index(0);
    if (nr == index++)
    {
        if (card.pause && (movesplanned() == 0))
        {
            LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
                                    , LCD_LINE_HEIGHT
                                    , 52
                                    , LCD_LINE_HEIGHT*4
                                    , PSTR("RESUME|")
                                    , ALIGN_CENTER
                                    , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, startGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, startGfx);
            }
        } else {
            LCDMenu::drawMenuString_P(LCD_CHAR_MARGIN_LEFT+3
                                    , LCD_LINE_HEIGHT
                                    , 52
                                    , LCD_LINE_HEIGHT*4
                                    , PSTR("PAUSING|")
                                    , ALIGN_CENTER
                                    , flags);
            if (flags & MENU_SELECTED)
            {
                lcd_lib_clear_gfx(LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, hourglassGfx);
            }
            else
            {
                lcd_lib_draw_gfx(LCD_CHAR_MARGIN_LEFT+26, LCD_LINE_HEIGHT*3+2, hourglassGfx);
            }
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+3
                           , LCD_LINE_HEIGHT
                           , 52
                           , LCD_LINE_HEIGHT*4
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_string_center_atP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+29, LCD_LINE_HEIGHT+LCD_LINE_HEIGHT/2+2, PSTR("CHANGE"));
            lcd_lib_clear_string_center_atP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+29, 2*LCD_LINE_HEIGHT+LCD_LINE_HEIGHT/2+2, PSTR("MATERIAL"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+26, 3*LCD_LINE_HEIGHT+LCD_LINE_HEIGHT/2+2, filamentGfx);
        }
        else
        {
            lcd_lib_draw_string_center_atP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+29, LCD_LINE_HEIGHT+LCD_LINE_HEIGHT/2+2, PSTR("CHANGE"));
            lcd_lib_draw_string_center_atP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+29, 2*LCD_LINE_HEIGHT+LCD_LINE_HEIGHT/2+2, PSTR("MATERIAL"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT+26, 3*LCD_LINE_HEIGHT+LCD_LINE_HEIGHT/2+2, filamentGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_CHAR_MARGIN_LEFT*2
                           , BOTTOM_MENU_YPOS
                           , 48
                           , LCD_CHAR_HEIGHT
                           , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_clear_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING+1, BOTTOM_MENU_YPOS, PSTR("TUNE"));
            lcd_lib_draw_gfx(2*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, menuGfx);
        }
    }
    else if (nr == index++)
    {
        LCDMenu::drawMenuBox(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT*2
                                , BOTTOM_MENU_YPOS
                                , 48
                                , LCD_CHAR_HEIGHT
                                , flags);
        if (flags & MENU_SELECTED)
        {
            lcd_lib_clear_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_clear_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
        else
        {
            lcd_lib_draw_stringP(LCD_GFX_WIDTH/2 + LCD_CHAR_MARGIN_LEFT + 3*LCD_CHAR_SPACING, BOTTOM_MENU_YPOS, PSTR("ABORT"));
            lcd_lib_draw_gfx(LCD_GFX_WIDTH/2 + 10, BOTTOM_MENU_YPOS, standbyGfx);
        }
    }
}

void lcd_menu_print_resume()
{
    if (pauseRequested)
    {
        lcd_print_pause();
    }
    if (movesplanned())
    {
        last_user_interaction = millis();
    }

    lcd_lib_clear();
    lcd_lib_draw_vline(64, 5, 46);
    lcd_lib_draw_hline(3, 124, 50);

    menu.process_submenu(get_resume_menuoption, 4);

    for (uint8_t index=0; index<4; ++index)
    {
        menu.drawSubMenu(drawResumeSubmenu, index);
    }
    lcd_lib_update_screen();
}

#endif//ENABLE_ULTILCD2
