#include <avr/pgmspace.h>

#include "Configuration.h"
#ifdef ENABLE_ULTILCD2
#include "Marlin.h"
#include "cardreader.h"
#include "temperature.h"
#include "lifetime_stats.h"
#include "UltiLCD2.h"
#include "UltiLCD2_hi_lib.h"
#include "UltiLCD2_menu_print.h"
#include "UltiLCD2_menu_material.h"
#include "UltiLCD2_menu_maintenance.h"

#define HEATUP_POSITION_COMMAND "G1 F12000 X5 Y10"

uint8_t lcd_cache[LCD_CACHE_SIZE];
#define LCD_CACHE_NR_OF_FILES() lcd_cache[(LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))]
#define LCD_CACHE_ID(n) lcd_cache[(n)]
#define LCD_CACHE_FILENAME(n) ((char*)&lcd_cache[2*LCD_CACHE_COUNT + (n) * LONG_FILENAME_LENGTH])
#define LCD_CACHE_TYPE(n) lcd_cache[LCD_CACHE_COUNT + (n)]
#define LCD_DETAIL_CACHE_START ((LCD_CACHE_COUNT*(LONG_FILENAME_LENGTH+2))+1)
#define LCD_DETAIL_CACHE_ID() lcd_cache[LCD_DETAIL_CACHE_START]
#define LCD_DETAIL_CACHE_TIME() (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+1])
#define LCD_DETAIL_CACHE_MATERIAL(n) (*(uint32_t*)&lcd_cache[LCD_DETAIL_CACHE_START+5+4*n])

void doCooldown();//TODO
static void lcd_menu_print_heatup();
static void lcd_menu_print_printing();
static void lcd_menu_print_error_sd();
static void lcd_menu_print_error_position();
static void lcd_menu_print_classic_warning();
static void lcd_menu_print_abort();
static void lcd_menu_print_ready();
static void lcd_menu_print_ready_cooled_down();
static void lcd_menu_print_tune();
static void lcd_menu_print_tune_retraction();
static void lcd_menu_print_pause();

bool primed = false;
static bool pauseRequested = false;


void lcd_clear_cache()
{
    for(uint8_t n=0; n<LCD_CACHE_COUNT; n++)
        LCD_CACHE_ID(n) = 0xFF;
    LCD_DETAIL_CACHE_ID() = 0;
    LCD_CACHE_NR_OF_FILES() = 0xFF;
}

static void abortPrint()
{
    postMenuCheck = NULL;
    lifetime_stats_print_end();
    doCooldown();

    clear_command_queue();
    char buffer[32];
    if (card.sdprinting)
    {
        // we're not printing any more
        card.sdprinting = false;
    }
    //If we where paused, make sure we abort that pause. Else strange things happen: https://github.com/Ultimaker/Ultimaker2Marlin/issues/32
    card.pause = false;
    pauseRequested = false;

    enquecommand_P(PSTR("M401"));

    if (primed)
    {
        // set up the end of print retraction
        sprintf_P(buffer, PSTR("G92 E%i"), int( ((float)END_OF_PRINT_RETRACTION) / volume_to_filament_length[active_extruder]) - (retracted ? retract_length : 0) );
        enquecommand(buffer);
        // since we have just parked the filament accounting for the retracted length, forget about any G10/G11 retractions that happened at end of this print.
        retracted = false;

        // perform the retraction at the standard retract speed
        sprintf_P(buffer, PSTR("G1 F%i E0"), int(retract_feedrate));
        enquecommand(buffer);

        // no longer primed
        primed = false;
    }

    if (current_position[Z_AXIS] > Z_MAX_POS - 30)
    {
        enquecommand_P(PSTR("G28 X0 Y0"));
        enquecommand_P(PSTR("G28 Z0"));
    }else{
        enquecommand_P(PSTR("G28"));
    }
    enquecommand_P(PSTR("M84"));
}

static void checkPrintFinished()
{
    if (pauseRequested)
    {
        lcd_menu_print_pause();
    }

    if (!card.sdprinting && !is_command_queued())
    {
        abortPrint();
        currentMenu = lcd_menu_print_ready;
        SELECT_MAIN_MENU_ITEM(0);
    }else if (position_error)
    {
        quickStop();
        abortPrint();
        currentMenu = lcd_menu_print_error_position;
        SELECT_MAIN_MENU_ITEM(0);
    }else if (card.errorCode())
    {
        abortPrint();
        currentMenu = lcd_menu_print_error_sd;
        SELECT_MAIN_MENU_ITEM(0);
    }
}

static void doStartPrint()
{
    // zero the extruder position
    current_position[E_AXIS] = 0.0;
    plan_set_e_position(0);
    primed = false;
    position_error = false;

    for(uint8_t e = 0; e<EXTRUDERS; e++)
    {
        if (!LCD_DETAIL_CACHE_MATERIAL(e))
        {
                // don't prime the extruder if it isn't used in the (Ulti)gcode
                // traditional gcode files typically won't have the Material lines at start, so we won't prime for those
                // Also, on dual/multi extrusion files, only prime the extruders that are used in the gcode-file.
            continue;
        }
        active_extruder = e;

        if (!primed)
        {
            // move to priming height
            current_position[Z_AXIS] = PRIMING_HEIGHT;
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
            // note that we have primed, so that we know to de-prime at the end
            primed = true;
        }

        // undo the end-of-print retraction
        plan_set_e_position((0.0 - END_OF_PRINT_RETRACTION) / volume_to_filament_length[e]);
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], END_OF_PRINT_RECOVERY_SPEED, e);

        // perform additional priming
        plan_set_e_position(-PRIMING_MM3);
        plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], (PRIMING_MM3_PER_SEC * volume_to_filament_length[e]), e);

#if EXTRUDERS > 1
        // for extruders other than the first one, perform end of print retraction
        if (e > 0)
        {
            plan_set_e_position((END_OF_PRINT_RETRACTION) / volume_to_filament_length[e]);
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], retract_feedrate/60, e);
        }
#endif
    }
    active_extruder = 0;
    primed = true;

    postMenuCheck = checkPrintFinished;
    card.startFileprint();
    lifetime_stats_print_start();
    starttime = millis();
}

static void cardUpdir()
{
    card.updir();
}

static char* lcd_sd_menu_filename_callback(uint8_t nr)
{
    //This code uses the card.longFilename as buffer to store the filename, to save memory.
    if (nr == 0)
    {
        if (card.atRoot())
        {
            strcpy_P(card.longFilename, PSTR("< RETURN"));
        }else{
            strcpy_P(card.longFilename, PSTR("< BACK"));
        }
    }else{
        card.longFilename[0] = '\0';
        for(uint8_t idx=0; idx<LCD_CACHE_COUNT; idx++)
        {
            if (LCD_CACHE_ID(idx) == nr)
                strcpy(card.longFilename, LCD_CACHE_FILENAME(idx));
        }
        if (card.longFilename[0] == '\0')
        {
            card.getfilename(nr - 1);
            if (!card.longFilename[0])
                strcpy(card.longFilename, card.filename);
            if (!card.filenameIsDir)
            {
                if (strchr(card.longFilename, '.')) strrchr(card.longFilename, '.')[0] = '\0';
            }

            uint8_t idx = nr % LCD_CACHE_COUNT;
            LCD_CACHE_ID(idx) = nr;
            strcpy(LCD_CACHE_FILENAME(idx), card.longFilename);
            LCD_CACHE_TYPE(idx) = card.filenameIsDir ? 1 : 0;
            if (card.errorCode() && card.sdInserted)
            {
                //On a read error reset the file position and try to keep going. (not pretty, but these read errors are annoying as hell)
                card.clearError();
                LCD_CACHE_ID(idx) = 255;
                card.longFilename[0] = '\0';
            }
        }
    }
    return card.longFilename;
}

void lcd_sd_menu_details_callback(uint8_t nr)
{
    if (nr == 0)
    {
        return;
    }
    for(uint8_t idx=0; idx<LCD_CACHE_COUNT; idx++)
    {
        if (LCD_CACHE_ID(idx) == nr)
        {
            if (LCD_CACHE_TYPE(idx) == 1)
            {
                lcd_lib_draw_string_centerP(53, PSTR("Folder"));
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
                                LCD_DETAIL_CACHE_TIME() = atol(buffer + 6);
                            else if (strncmp_P(buffer, PSTR(";MATERIAL:"), 10) == 0)
                                LCD_DETAIL_CACHE_MATERIAL(0) = atol(buffer + 10);
#if EXTRUDERS > 1
                            else if (strncmp_P(buffer, PSTR(";MATERIAL2:"), 11) == 0)
                                LCD_DETAIL_CACHE_MATERIAL(1) = atol(buffer + 11);
#endif
                        }
                    }
                    if (card.errorCode())
                    {
                        //On a read error reset the file position and try to keep going. (not pretty, but these read errors are annoying as hell)
                        card.clearError();
                        LCD_DETAIL_CACHE_ID() = 255;
                    }
                }

                if (LCD_DETAIL_CACHE_TIME() > 0)
                {
                    char* c = buffer;
                    if (led_glow_dir)
                    {
                        strcpy_P(c, PSTR("Time: ")); c += 6;
                        c = int_to_time_string(LCD_DETAIL_CACHE_TIME(), c);
                    }else{
                        strcpy_P(c, PSTR("Material: ")); c += 10;
                        float length = float(LCD_DETAIL_CACHE_MATERIAL(0)) / (M_PI * (material[0].diameter / 2.0) * (material[0].diameter / 2.0));
                        if (length < 10000)
                            c = float_to_string(length / 1000.0, c, PSTR("m"));
                        else
                            c = int_to_string(length / 1000.0, c, PSTR("m"));
#if EXTRUDERS > 1
                        if (LCD_DETAIL_CACHE_MATERIAL(1))
                        {
                            *c++ = '/';
                            float length = float(LCD_DETAIL_CACHE_MATERIAL(1)) / (M_PI * (material[1].diameter / 2.0) * (material[1].diameter / 2.0));
                            if (length < 10000)
                                c = float_to_string(length / 1000.0, c, PSTR("m"));
                            else
                                c = int_to_string(length / 1000.0, c, PSTR("m"));
                        }
#endif
                    }
                    lcd_lib_draw_string(3, 53, buffer);
                }else{
                    lcd_lib_draw_stringP(3, 53, PSTR("No info available"));
                }
            }
        }
    }
}

void lcd_menu_print_select()
{
    if (!card.sdInserted)
    {
        LED_GLOW();
        lcd_lib_encoder_pos = MAIN_MENU_ITEM_POS(0);
        lcd_info_screen(lcd_menu_main);
        lcd_lib_draw_string_centerP(15, PSTR("No SD-CARD!"));
        lcd_lib_draw_string_centerP(25, PSTR("Please insert card"));
        lcd_lib_update_screen();
        card.release();
        return;
    }
    if (!card.isOk())
    {
        lcd_info_screen(lcd_menu_main);
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
            lcd_info_screen(lcd_menu_main, NULL, PSTR("OK"));
        else
            lcd_info_screen(lcd_menu_print_select, cardUpdir, PSTR("OK"));
        lcd_lib_draw_string_centerP(25, PSTR("No files found!"));
        lcd_lib_update_screen();
        lcd_clear_cache();
        return;
    }

    if (lcd_lib_button_pressed)
    {
        uint8_t selIndex = uint16_t(SELECTED_SCROLL_MENU_ITEM());
        if (selIndex == 0)
        {
            if (card.atRoot())
            {
                lcd_change_to_menu(lcd_menu_main);
            }else{
                lcd_clear_cache();
                lcd_lib_beep();
                card.updir();
            }
        }else{
            card.getfilename(selIndex - 1);
            if (!card.filenameIsDir)
            {
                //Start print
                active_extruder = 0;
                card.openFile(card.filename, true);
                if (card.isFileOpen() && !is_command_queued())
                {
                    if (led_mode == LED_MODE_WHILE_PRINTING || led_mode == LED_MODE_BLINK_ON_DONE)
                        analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
                    LCD_CACHE_ID(0) = 255;
                    if (card.longFilename[0])
                        strcpy(LCD_CACHE_FILENAME(0), card.longFilename);
                    else
                        strcpy(LCD_CACHE_FILENAME(0), card.filename);
                    LCD_CACHE_FILENAME(0)[20] = '\0';
                    if (strchr(LCD_CACHE_FILENAME(0), '.')) strchr(LCD_CACHE_FILENAME(0), '.')[0] = '\0';

                    char buffer[64];
                    card.fgets(buffer, sizeof(buffer));
                    buffer[sizeof(buffer)-1] = '\0';
                    while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                    if (strcmp_P(buffer, PSTR(";FLAVOR:UltiGCode")) != 0)
                    {
                        card.fgets(buffer, sizeof(buffer));
                        buffer[sizeof(buffer)-1] = '\0';
                        while (strlen(buffer) > 0 && buffer[strlen(buffer)-1] < ' ') buffer[strlen(buffer)-1] = '\0';
                    }
                    card.setIndex(0);

                    fanSpeed = 0;
                    feedmultiply = 100;
                    if (strcmp_P(buffer, PSTR(";FLAVOR:UltiGCode")) == 0)
                    {
                        //New style GCode flavor without start/end code.
                        // Temperature settings, filament settings, fan settings, start and end-code are machine controlled.
#if TEMP_SENSOR_BED != 0
                        target_temperature_bed = 0;
#endif
                        fanSpeedPercent = 0;
                        for(uint8_t e=0; e<EXTRUDERS; e++)
                        {
                            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1)
                                continue;
                            target_temperature[e] = 0;//material[e].temperature;
#if TEMP_SENSOR_BED != 0
                            target_temperature_bed = max(target_temperature_bed, material[e].bed_temperature);
#endif
                            fanSpeedPercent = max(fanSpeedPercent, material[e].fan_speed);
                            volume_to_filament_length[e] = 1.0 / (M_PI * (material[e].diameter / 2.0) * (material[e].diameter / 2.0));
                            extrudemultiply[e] = material[e].flow;
                        }

                        enquecommand_P(PSTR("G28"));
                        enquecommand_P(PSTR(HEATUP_POSITION_COMMAND));
                        lcd_change_to_menu(lcd_menu_print_heatup);
                    }else{
                        //Classic gcode file

                        //Set the settings to defaults so the classic GCode has full control
                        fanSpeedPercent = 100;
                        for(uint8_t e=0; e<EXTRUDERS; e++)
                        {
                            volume_to_filament_length[e] = 1.0;
                            extrudemultiply[e] = 100;
                        }

                        lcd_change_to_menu(lcd_menu_print_classic_warning, MAIN_MENU_ITEM_POS(0));
                    }
                }
            }else{
                lcd_lib_beep();
                lcd_clear_cache();
                card.chdir(card.filename);
                SELECT_SCROLL_MENU_ITEM(0);
            }
            return;//Return so we do not continue after changing the directory or selecting a file. The nrOfFiles is invalid at this point.
        }
    }
    lcd_scroll_menu(PSTR("SD CARD"), nrOfFiles+1, lcd_sd_menu_filename_callback, lcd_sd_menu_details_callback);
}

static void lcd_menu_print_heatup()
{
    lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_abort, NULL, PSTR("ABORT"));

#if TEMP_SENSOR_BED != 0
    if (current_temperature_bed > target_temperature_bed - 10)
    {
#endif
        for(uint8_t e=0; e<EXTRUDERS; e++)
        {
            if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] > 0)
                continue;
            target_temperature[e] = material[e].temperature;
        }

        if (current_temperature_bed >= target_temperature_bed - TEMP_WINDOW * 2 && !is_command_queued())
        {
            bool ready = true;
            for(uint8_t e=0; e<EXTRUDERS; e++)
                if (current_temperature[e] < target_temperature[e] - TEMP_WINDOW)
                    ready = false;

            if (ready)
            {
                doStartPrint();
                currentMenu = lcd_menu_print_printing;
            }
        }
#if TEMP_SENSOR_BED != 0
    }
#endif

    uint8_t progress = 125;
    for(uint8_t e=0; e<EXTRUDERS; e++)
    {
        if (LCD_DETAIL_CACHE_MATERIAL(e) < 1 || target_temperature[e] < 1)
            continue;
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
    lcd_lib_draw_string_center(30, LCD_CACHE_FILENAME(0));

    lcd_progressbar(progress);

    lcd_lib_update_screen();
}

static void lcd_change_to_menu_change_material_return()
{
    plan_set_e_position(current_position[E_AXIS]);
    setTargetHotend(material[active_extruder].temperature, active_extruder);
    currentMenu = lcd_menu_print_printing;
}

static void lcd_menu_print_printing()
{
    if (card.pause)
    {
        lcd_tripple_menu(PSTR("RESUME|PRINT"), PSTR("CHANGE|MATERIAL"), PSTR("TUNE"));
        if (lcd_lib_button_pressed)
        {
            if (IS_SELECTED_MAIN(0) && movesplanned() < 1)
            {
                card.pause = false;
                if (card.sdprinting)
                {
                    primed = true;
                }
                lcd_lib_beep();
            }else if (IS_SELECTED_MAIN(1) && movesplanned() < 1)
                lcd_change_to_menu_change_material(lcd_change_to_menu_change_material_return);
            else if (IS_SELECTED_MAIN(2))
                lcd_change_to_menu(lcd_menu_print_tune);
        }
    }
    else
    {
        lcd_question_screen(lcd_menu_print_tune, NULL, PSTR("TUNE"), lcd_menu_print_printing, lcd_menu_print_pause, PSTR("PAUSE"));
        uint8_t progress = card.getFilePos() / ((card.getFileSize() + 123) / 124);
        char buffer[16];
        char* c;
        switch(printing_state)
        {
        default:
            lcd_lib_draw_string_centerP(20, PSTR("Printing:"));
            lcd_lib_draw_string_center(30, LCD_CACHE_FILENAME(0));
            break;
        case PRINT_STATE_HEATING:
            lcd_lib_draw_string_centerP(20, PSTR("Heating"));
            c = int_to_string(dsp_temperature[0], buffer, PSTR("C"));
            *c++ = '/';
            c = int_to_string(target_temperature[0], c, PSTR("C"));
            lcd_lib_draw_string_center(30, buffer);
            break;
        case PRINT_STATE_HEATING_BED:
            lcd_lib_draw_string_centerP(20, PSTR("Heating buildplate"));
            c = int_to_string(dsp_temperature_bed, buffer, PSTR("C"));
            *c++ = '/';
            c = int_to_string(target_temperature_bed, c, PSTR("C"));
            lcd_lib_draw_string_center(30, buffer);
            break;
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
    }

    lcd_lib_update_screen();
}

static void lcd_menu_print_error_sd()
{
    LED_GLOW_ERROR();
    lcd_info_screen(lcd_menu_main, NULL, PSTR("RETURN TO MAIN"));

    lcd_lib_draw_string_centerP(10, PSTR("Error while"));
    lcd_lib_draw_string_centerP(20, PSTR("reading SD-card!"));
    lcd_lib_draw_string_centerP(30, PSTR("Go to:"));
    lcd_lib_draw_string_centerP(40, PSTR("ultimaker.com/ER08"));
    /*
    char buffer[12];
    strcpy_P(buffer, PSTR("Code:"));
    int_to_string(card.errorCode(), buffer+5);
    lcd_lib_draw_string_center(40, buffer);
    */

    lcd_lib_update_screen();
}

static void lcd_menu_print_error_position()
{
    LED_GLOW_ERROR();
    lcd_info_screen(lcd_menu_main, NULL, PSTR("RETURN TO MAIN"));

    lcd_lib_draw_string_centerP(15, PSTR("ERROR:"));
    lcd_lib_draw_string_centerP(25, PSTR("Tried printing out"));
    lcd_lib_draw_string_centerP(35, PSTR("of printing area"));

    lcd_lib_update_screen();
}

static void lcd_menu_print_classic_warning()
{
    lcd_question_screen(lcd_menu_print_printing, doStartPrint, PSTR("CONTINUE"), lcd_menu_print_select, NULL, PSTR("CANCEL"));

    lcd_lib_draw_string_centerP(10, PSTR("This file will"));
    lcd_lib_draw_string_centerP(20, PSTR("override machine"));
    lcd_lib_draw_string_centerP(30, PSTR("setting with setting"));
    lcd_lib_draw_string_centerP(40, PSTR("from the slicer."));

    lcd_lib_update_screen();
}

static void lcd_menu_print_abort()
{
    LED_GLOW();
    lcd_question_screen(lcd_menu_print_ready, abortPrint, PSTR("YES"), previousMenu, NULL, PSTR("NO"));

    lcd_lib_draw_string_centerP(20, PSTR("Abort the print?"));

    lcd_lib_update_screen();
}

static void postPrintReady()
{
    if (led_mode == LED_MODE_BLINK_ON_DONE)
        analogWrite(LED_PIN, 0);
}

static void lcd_menu_print_ready()
{
    if (led_mode == LED_MODE_WHILE_PRINTING)
        analogWrite(LED_PIN, 0);
    else if (led_mode == LED_MODE_BLINK_ON_DONE)
        analogWrite(LED_PIN, (led_glow << 1) * int(led_brightness_level) / 100);
    lcd_info_screen(lcd_menu_main, postPrintReady, PSTR("BACK TO MENU"));
    //unsigned long printTimeSec = (stoptime-starttime)/1000;
    if (current_temperature[0] > 60 || current_temperature_bed > 40)
    {
        lcd_lib_draw_string_centerP(15, PSTR("Printer cooling down"));

        int16_t progress = 124 - (current_temperature[0] - 60);
        if (progress < 0) progress = 0;
        if (progress > 124) progress = 124;

        if (progress < minProgress)
            progress = minProgress;
        else
            minProgress = progress;

        lcd_progressbar(progress);
        char buffer[16];
        char* c = buffer;
        for(uint8_t e=0; e<EXTRUDERS; e++)
            c = int_to_string(dsp_temperature[e], c, PSTR("C "));
#if TEMP_SENSOR_BED != 0
        int_to_string(dsp_temperature_bed, c, PSTR("C"));
#endif
        lcd_lib_draw_string_center(25, buffer);
    }else{
        currentMenu = lcd_menu_print_ready_cooled_down;
    }
    lcd_lib_update_screen();
}

static void lcd_menu_print_ready_cooled_down()
{
    if (led_mode == LED_MODE_WHILE_PRINTING)
        analogWrite(LED_PIN, 0);
    else if (led_mode == LED_MODE_BLINK_ON_DONE)
        analogWrite(LED_PIN, (led_glow << 1) * int(led_brightness_level) / 100);
    lcd_info_screen(lcd_menu_main, postPrintReady, PSTR("BACK TO MENU"));

    LED_GLOW();
    lcd_lib_draw_string_centerP(10, PSTR("Print finished"));
    lcd_lib_draw_string_centerP(30, PSTR("You can remove"));
    lcd_lib_draw_string_centerP(40, PSTR("the print."));

    lcd_lib_update_screen();
}

static char* tune_item_callback(uint8_t nr)
{
    char* c = card.longFilename;
    if (nr == 0)
        strcpy_P(c, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(c, PSTR("Abort"));
    else if (nr == 2)
        strcpy_P(c, PSTR("Speed"));
    else if (nr == 3)
        strcpy_P(c, PSTR("Temperature"));
#if EXTRUDERS > 1
    else if (nr == 4)
        strcpy_P(c, PSTR("Temperature 2"));
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == 3 + EXTRUDERS)
        strcpy_P(c, PSTR("Buildplate temp."));
#endif
    else if (nr == 3 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(c, PSTR("Fan speed"));
    else if (nr == 4 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(c, PSTR("Material flow"));
#if EXTRUDERS > 1
    else if (nr == 5 + BED_MENU_OFFSET + EXTRUDERS)
        strcpy_P(c, PSTR("Material flow 2"));
#endif
    else if (nr == 4 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(c, PSTR("Retraction"));
    else if (nr == 5 + BED_MENU_OFFSET + EXTRUDERS * 2)
        strcpy_P(c, PSTR("LED Brightness"));
    return c;
}

static void tune_item_details_callback(uint8_t nr)
{
    char* c = card.longFilename;
    if (nr == 2)
        c = int_to_string(feedmultiply, c, PSTR("%"));
    else if (nr == 3)
    {
        c = int_to_string(dsp_temperature[0], c, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature[0], c, PSTR("C"));
    }
#if EXTRUDERS > 1
    else if (nr == 4)
    {
        c = int_to_string(dsp_temperature[1], c, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature[1], c, PSTR("C"));
    }
#endif
#if TEMP_SENSOR_BED != 0
    else if (nr == 3 + EXTRUDERS)
    {
        c = int_to_string(dsp_temperature_bed, c, PSTR("C"));
        *c++ = '/';
        c = int_to_string(target_temperature_bed, c, PSTR("C"));
    }
#endif
    else if (nr == 3 + BED_MENU_OFFSET + EXTRUDERS)
        c = int_to_string(int(fanSpeed) * 100 / 255, c, PSTR("%"));
    else if (nr == 4 + BED_MENU_OFFSET + EXTRUDERS)
        c = int_to_string(extrudemultiply[0], c, PSTR("%"));
#if EXTRUDERS > 1
    else if (nr == 5 + BED_MENU_OFFSET + EXTRUDERS)
        c = int_to_string(extrudemultiply[1], c, PSTR("%"));
#endif
    else if (nr == 6 + BED_MENU_OFFSET + EXTRUDERS)
    {
        c = int_to_string(led_brightness_level, c, PSTR("%"));
        if (led_mode == LED_MODE_ALWAYS_ON ||  led_mode == LED_MODE_WHILE_PRINTING || led_mode == LED_MODE_BLINK_ON_DONE)
            analogWrite(LED_PIN, 255 * int(led_brightness_level) / 100);
    }
    else
        return;
    lcd_lib_draw_string(5, 53, card.longFilename);
}

void lcd_menu_print_tune_heatup_nozzle0()
{
    if (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM != 0)
    {
        target_temperature[0] = int(target_temperature[0]) + (lcd_lib_encoder_pos / ENCODER_TICKS_PER_SCROLL_MENU_ITEM);
        if (target_temperature[0] < 0)
            target_temperature[0] = 0;
        if (target_temperature[0] > HEATER_0_MAXTEMP - 15)
            target_temperature[0] = HEATER_0_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Nozzle temperature:"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(dsp_temperature[0]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[0]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
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
        if (target_temperature[1] > HEATER_0_MAXTEMP - 15)
            target_temperature[1] = HEATER_0_MAXTEMP - 15;
        lcd_lib_encoder_pos = 0;
    }
    if (lcd_lib_button_pressed)
        lcd_change_to_menu(previousMenu, previousEncoderPos);

    lcd_lib_clear();
    lcd_lib_draw_string_centerP(20, PSTR("Nozzle2 temperature:"));
    lcd_lib_draw_string_centerP(53, PSTR("Click to return"));
    char buffer[16];
    int_to_string(int(dsp_temperature[1]), buffer, PSTR("C/"));
    int_to_string(int(target_temperature[1]), buffer+strlen(buffer), PSTR("C"));
    lcd_lib_draw_string_center(30, buffer);
    lcd_lib_update_screen();
}
#endif

static void lcd_menu_print_tune()
{
    lcd_scroll_menu(PSTR("TUNE"), 6 + BED_MENU_OFFSET + EXTRUDERS * 2, tune_item_callback, tune_item_details_callback);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
        {
            if (card.sdprinting)
                lcd_change_to_menu(lcd_menu_print_printing);
            else
                lcd_change_to_menu(lcd_menu_print_heatup);
        }else if (IS_SELECTED_SCROLL(1))
        {
            lcd_change_to_menu(lcd_menu_print_abort);
        }else if (IS_SELECTED_SCROLL(2))
            LCD_EDIT_SETTING(feedmultiply, "Print speed", "%", 10, 1000);
        else if (IS_SELECTED_SCROLL(3))
            lcd_change_to_menu(lcd_menu_print_tune_heatup_nozzle0, 0);
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(4))
            lcd_change_to_menu(lcd_menu_print_tune_heatup_nozzle1, 0);
#endif
#if TEMP_SENSOR_BED != 0
        else if (IS_SELECTED_SCROLL(3 + EXTRUDERS))
            lcd_change_to_menu(lcd_menu_maintenance_advanced_bed_heatup, 0);//Use the maintainace heatup menu, which shows the current temperature.
#endif
        else if (IS_SELECTED_SCROLL(3 + BED_MENU_OFFSET + EXTRUDERS))
            LCD_EDIT_SETTING_BYTE_PERCENT(fanSpeed, "Fan speed", "%", 0, 100);
        else if (IS_SELECTED_SCROLL(4 + BED_MENU_OFFSET + EXTRUDERS))
            LCD_EDIT_SETTING(extrudemultiply[0], "Material flow", "%", 10, 1000);
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(5 + BED_MENU_OFFSET + EXTRUDERS))
            LCD_EDIT_SETTING(extrudemultiply[1], "Material flow 2", "%", 10, 1000);
#endif
        else if (IS_SELECTED_SCROLL(4 + BED_MENU_OFFSET + EXTRUDERS * 2))
            lcd_change_to_menu(lcd_menu_print_tune_retraction);
        else if (IS_SELECTED_SCROLL(5 + BED_MENU_OFFSET + EXTRUDERS * 2))
            LCD_EDIT_SETTING(led_brightness_level, "Brightness", "%", 0, 100);
    }
}

static char* lcd_retraction_item(uint8_t nr)
{
    if (nr == 0)
        strcpy_P(card.longFilename, PSTR("< RETURN"));
    else if (nr == 1)
        strcpy_P(card.longFilename, PSTR("Retract length"));
    else if (nr == 2)
        strcpy_P(card.longFilename, PSTR("Retract speed"));
#if EXTRUDERS > 1
    else if (nr == 3)
        strcpy_P(card.longFilename, PSTR("Extruder change len"));
#endif
    else
        strcpy_P(card.longFilename, PSTR("???"));
    return card.longFilename;
}

static void lcd_retraction_details(uint8_t nr)
{
    char buffer[16];
    if (nr == 0)
        return;
    else if(nr == 1)
        float_to_string(retract_length, buffer, PSTR("mm"));
    else if(nr == 2)
        int_to_string(retract_feedrate / 60 + 0.5, buffer, PSTR("mm/sec"));
#if EXTRUDERS > 1
    else if(nr == 3)
        int_to_string(extruder_swap_retract_length, buffer, PSTR("mm"));
#endif
    lcd_lib_draw_string(5, 53, buffer);
}

static void lcd_menu_print_tune_retraction()
{
    lcd_scroll_menu(PSTR("RETRACTION"), 3 + (EXTRUDERS > 1 ? 1 : 0), lcd_retraction_item, lcd_retraction_details);
    if (lcd_lib_button_pressed)
    {
        if (IS_SELECTED_SCROLL(0))
            lcd_change_to_menu(lcd_menu_print_tune, SCROLL_MENU_ITEM_POS(6));
        else if (IS_SELECTED_SCROLL(1))
            LCD_EDIT_SETTING_FLOAT001(retract_length, "Retract length", "mm", 0, 50);
        else if (IS_SELECTED_SCROLL(2))
            LCD_EDIT_SETTING_SPEED(retract_feedrate, "Retract speed", "mm/sec", 0, max_feedrate[E_AXIS] * 60);
#if EXTRUDERS > 1
        else if (IS_SELECTED_SCROLL(3))
            LCD_EDIT_SETTING_FLOAT001(extruder_swap_retract_length, "Extruder change", "mm", 0, 50);
#endif
    }
}

static void lcd_menu_print_pause()
{
    if (card.sdprinting && !card.pause)
    {
        if (movesplanned() > 0 && commands_queued() < BUFSIZE)
        {
            pauseRequested = false;
            card.pause = true;

            // move z up according to the current height - but minimum to z=70mm (above the gantry height)
            uint16_t zdiff = 0;
            if (current_position[Z_AXIS] < 70)
                zdiff = max(70 - floor(current_position[Z_AXIS]), 20);
            else if (current_position[Z_AXIS] < Z_MAX_POS - 60)
            {
                zdiff = 20;
            }
            else if (current_position[Z_AXIS] < Z_MAX_POS - 30)
            {
                zdiff = 2;
            }

            char buffer[32];
            sprintf_P(buffer, PSTR("M601 X5 Y5 Z%i L%i"), int(zdiff), int(END_OF_PRINT_RETRACTION));
            enquecommand(buffer);

            primed = false;
        }
        else{
            pauseRequested = true;
        }
    }
}

#endif//ENABLE_ULTILCD2
