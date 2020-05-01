/* -*- c++ -*- */

/*
 Reprap firmware based on Sprinter and grbl.
 Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 This firmware is a mashup between Sprinter and grbl.
  (https://github.com/kliment/Sprinter)
  (https://github.com/simen/grbl/tree)

 It has preliminary support for Matthew Roberts advance algorithm
    http://reprap.org/pipermail/reprap-dev/2011-May/003323.html
 */

#include "Marlin.h"

#include "ultralcd.h"
#include "UltiLCD2.h"
#include "planner.h"
#include "stepper.h"
#include "temperature.h"
#include "motion_control.h"
#include "cardreader.h"
#include "watchdog.h"
#include "ConfigurationStore.h"
#include "lifetime_stats.h"
#include "electronics_test.h"
#include "language.h"
#include "pins_arduino.h"
#include "tinkergnome.h"
#include "powerbudget.h"
#include "machinesettings.h"
#include "filament_sensor.h"
#include "preferences.h"

#if NUM_SERVOS > 0
#include "Servo.h"
#endif

#if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
#include <SPI.h>
#endif

#define VERSION_STRING  "1.0.0"

// look here for descriptions of gcodes: http://linuxcnc.org/handbook/gcode/g-code.html
// http://objects.reprap.org/wiki/Mendel_User_Manual:_RepRapGCodes

//Implemented Codes
//-------------------
// G0  -> G1
// G1  - Coordinated Movement X Y Z E
// G2  - CW ARC
// G3  - CCW ARC
// G4  - Dwell S<seconds> or P<milliseconds>
// G10 - retract filament according to settings of M207
// G11 - retract recover filament according to settings of M208
// G28 - Home all Axis
// G90 - Use Absolute Coordinates
// G91 - Use Relative Coordinates
// G92 - Set current position to coordinates given

//RepRap M Codes
// M0   - Unconditional stop - Wait for user to press a button on the LCD (Only if ULTRA_LCD is enabled)
// M1   - Same as M0
// M104 - Set extruder target temp
// M105 - Read current temp
// M106 - Fan on
// M107 - Fan off
// M109 - Wait for extruder current temp to reach target temp.
// M114 - Display current position

//Custom M Codes
// M17  - Enable/Power all stepper motors
// M18  - Disable all stepper motors; same as M84
// M20  - List SD card
// M21  - Init SD card
// M22  - Release SD card
// M23  - Select SD file (M23 filename.g)
// M24  - Start/resume SD print
// M25  - Pause SD print
// M26  - Set SD position in bytes (M26 S12345)
// M27  - Report SD print status
// M28  - Start SD write (M28 filename.g)
// M29  - Stop SD write
// M30  - Delete file from SD (M30 filename.g)
// M31  - Output time since last M109 or SD card start to serial
// M42  - Change pin status via gcode Use M42 Px Sy to set pin x to value y, when omitting Px the onboard led will be used.
// M80  - Turn on Power Supply
// M81  - Turn off Power Supply
// M82  - Set E codes absolute (default)
// M83  - Set E codes relative while in Absolute Coordinates (G90) mode
// M84  - Disable steppers until next move,
//        or use S<seconds> to specify an inactivity timeout, after which the steppers will be disabled.  S0 to disable the timeout.
// M85  - Set inactivity shutdown timer with parameter S<seconds>. To disable set zero (default)
// M92  - Set axis_steps_per_unit - same syntax as G92
// M114 - Output current position to serial port
// M115 - Capabilities string
// M117 - display message
// M119 - Output Endstop status to serial port
// M126 - Solenoid Air Valve Open (BariCUDA support by jmil)
// M127 - Solenoid Air Valve Closed (BariCUDA vent to atmospheric pressure by jmil)
// M128 - EtoP Open (BariCUDA EtoP = electricity to air pressure transducer by jmil)
// M129 - EtoP Closed (BariCUDA EtoP = electricity to air pressure transducer by jmil)
// M140 - Set bed target temp
// M190 - Wait for bed current temp to reach target temp.
// M200 - Set filament diameter
// M201 - Set max acceleration in units/s^2 for print moves (M201 X1000 Y1000)
// M202 - Set max acceleration in units/s^2 for travel moves (M202 X1000 Y1000) Unused in Marlin!!
// M203 - Set maximum feedrate that your machine can sustain (M203 X200 Y200 Z300 E10000) in mm/sec
// M204 - Set default acceleration: S normal moves T filament only moves (M204 S3000 T7000) im mm/sec^2  also sets minimum segment time in ms (B20000) to prevent buffer underruns and M20 minimum feedrate
// M205 -  advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk, E=maximum E jerk
// M206 - set additional homeing offset
// M207 - set retract length S[positive mm] F[feedrate mm/sec] Z[additional zlift/hop]
// M208 - set recover=unretract length S[positive mm surplus to the M207 S*] F[feedrate mm/sec]
// M209 - S<1=true/0=false> enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
// M218 - set hotend offset (in mm): T<extruder_number> X<offset_on_X> Y<offset_on_Y>
// M220 S<factor in percent>- set speed factor override percentage
// M221 S<factor in percent>- set extrude factor override percentage
// M240 - Trigger a camera to take a photograph
// M280 - set servo position absolute. P: servo index, S: angle or microseconds
// M300 - Play beepsound S<frequency Hz> P<duration ms>
// M301 - Set PID parameters P I and D
// M302 - Allow cold extrudes, or set the minimum extrude S<temperature>.
// M303 - PID relay autotune S<temperature> sets the target temperature. (default target temperature = 150C)
// M304 - Set bed PID parameters P I and D
// M400 - Finish all moves
// M401 - Cancel as many moves as possible
// M500 - stores paramters in EEPROM
// M501 - reads parameters from EEPROM (if you need reset them after you changed them temporarily).
// M502 - reverts to the default "factory settings".  You still need to store them in EEPROM afterwards if you want to.
// M503 - print the current settings (from memory not from eeprom)
// M540 - Use S[0|1] to enable or disable the stop SD card print on endstop hit (requires ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED)
// M600 - Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
// M907 - Set digital trimpot motor current using axis codes.
// M908 - Control digital trimpot directly.
// M350 - Set microstepping mode.
// M351 - Toggle MS1 MS2 pins directly.
// M923 - Select file and start printing directly (can be used from other SD file)
// M928 - Start SD logging (M928 filename.g) - ended by M29
// M999 - Restart after being stopped by error

//Stepper Movement Variables

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================
#ifdef SDSUPPORT
CardReader card;
#endif
float homing_feedrate[] = HOMING_FEEDRATE;
int feedmultiply=100; //100->1 200->2
int extrudemultiply[EXTRUDERS]=ARRAY_BY_EXTRUDERS(100, 100, 100); //100->1 200->2
float current_position[NUM_AXIS] = { 0.0, 0.0, 0.0, 0.0 };
float add_homing[3]={0,0,0};
float min_pos[3] = { X_MIN_POS, Y_MIN_POS, Z_MIN_POS };
float max_pos[3] = { X_MAX_POS, Y_MAX_POS, Z_MAX_POS };
// Extruder offset, only in XY plane
#if EXTRUDERS > 1
float extruder_offset[2][EXTRUDERS] = {
#if defined(EXTRUDER_OFFSET_X) && defined(EXTRUDER_OFFSET_Y)
  EXTRUDER_OFFSET_X, EXTRUDER_OFFSET_Y
#endif
};
#endif
uint8_t active_extruder = 0;
uint8_t tmp_extruder = 0;
uint8_t fanSpeed=0;
uint8_t fanSpeedPercent=100;

MachineSettings machinesettings;

#ifdef SERVO_ENDSTOPS
  int servo_endstops[] = SERVO_ENDSTOPS;
  int servo_endstop_angles[] = SERVO_ENDSTOP_ANGLES;
#endif
#ifdef BARICUDA
int ValvePressure=0;
int EtoPPressure=0;
#endif
bool position_error;

#ifdef FWRETRACT
  bool autoretract_enabled=false;
  bool retracted=false;
  float retract_length=4.5, retract_feedrate=25*60, retract_zlift=0.8;
#if EXTRUDERS > 1
  float extruder_swap_retract_length=16.0;
#endif
  float retract_recover_length=0, retract_recover_feedrate=25*60;
#endif

uint8_t printing_state;

//===========================================================================
//=============================private variables=============================
//===========================================================================
const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};
static float destination[NUM_AXIS] = {  0.0, 0.0, 0.0, 0.0};
#ifdef DELTA
static float delta[3] = {0.0, 0.0, 0.0};
#endif
static float offset[3] = {0.0, 0.0, 0.0};
static bool home_all_axis = true;
static float feedrate = 1500.0;
static long gcode_LastN, Stopped_gcode_LastN = 0;

// static bool relative_mode = false;  //Determines Absolute or Relative Coordinates
#define RELATIVE_MODE 128
uint8_t axis_relative_state = 0;

static char cmd_line_buffer[MAX_CMD_SIZE] = {'\0'};
static char cmdbuffer[BUFSIZE][MAX_CMD_SIZE] = {'\0'};
#if BUFSIZE > 8
uint16_t serialCmd = 0;
#else
uint8_t serialCmd = 0;
#endif // BUFSIZE
static uint8_t bufindr = 0;
static uint8_t bufindw = 0;
static uint8_t buflen = 0;
static int serial_count = 0;
static bool comment_mode = false;
static char *strchr_pointer = 0; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

const int sensitive_pins[] = SENSITIVE_PINS; // Sensitive pin list for M42

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
static unsigned long max_inactive_time = 0;
#if DISABLE_X || DISABLE_Y || DISABLE_Z || DISABLE_E
static unsigned long stepper_inactive_time = DEFAULT_STEPPER_DEACTIVE_TIME*1000l;
#endif

unsigned long starttime=0;
unsigned long stoptime=0;

static uint8_t Stopped = 0x0;

#if NUM_SERVOS > 0
  Servo servos[NUM_SERVOS];
#endif

#define roundTemperature(x) ((x)>0?(uint16_t)((x)+0.5):0)

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================
static void manage_inactivity();
static void get_coordinates(const char *cmd);
static void get_arc_coordinates(const char *cmd);
static bool setTargetedHotend(const char *cmd, int code);
static void prepare_arc_move(char isclockwise);
static void prepare_move(const char *cmd);
static void get_command();
static void FlushSerialRequestResend();
static void ClearToSend();


void serial_echopair_P(const char *s_P, float v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, double v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, unsigned long v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, unsigned int v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_action_P(const char *s_P)
    { serialprintPGM(PSTR("//action:")); serialprintPGM(s_P); SERIAL_EOL; }

extern "C"{
  extern unsigned int __bss_end;
  extern unsigned int __heap_start;
  extern void *__brkval;

  int freeMemory() {
    int free_memory;

    if((int)__brkval == 0)
      free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
      free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
  }
}

/**
 * Once a new command is in the ring buffer, call this to commit it
 */
static void commit_command(bool isSerialCmd)
{
  ++buflen;
  if (isSerialCmd)
  {
    //set serial flag for new command
    serialCmd |= (1 << bufindw);
  }
  else
  {
    //clear serial flag
    serialCmd &= ~(1 << bufindw);
  }
  ++bufindw;
  bufindw &= BUFMASK;
}

/**
 * Once the current command is interpreted, remove it from the ring buffer
 */
static void remove_command()
{
    serialCmd &= ~(1 << bufindr);
    ++bufindr;
    bufindr &= BUFMASK;
    --buflen;
}

//Clear all the commands in the ASCII command buffer
void clear_command_queue()
{
    while (buflen)
    {
        remove_command();
    }
    bufindw = bufindr = 0;
    serialCmd = 0;
}

static void next_command()
{
  #ifdef SDSUPPORT
    if(card.saving())
    {
        if(strstr_P(cmdbuffer[bufindr], PSTR("M29")) == NULL)
        {
          card.write_command(cmdbuffer[bufindr]);
          if(card.logging())
          {
            process_command(cmdbuffer[bufindr], serialCmd & (1 << bufindr));
          }
          else
          {
            SERIAL_PROTOCOLLNPGM(MSG_OK);
          }
        }
        else
        {
          card.closefile();
          SERIAL_PROTOCOLLNPGM(MSG_FILE_SAVED);
        }
    }
    else
    {
    process_command(cmdbuffer[bufindr], serialCmd & (1 << bufindr));
    }
  #else
    process_command(cmdbuffer[bufindr], serialCmd & (1 << bufindr));
  #endif //SDSUPPORT

    if (buflen)
    {
        remove_command();
    }
}

static void prepareenque()
{
    while (buflen >= BUFSIZE)
    {
        next_command();
        checkHitEndstops();
        idle();
    }
}

static void finishenque()
{
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(cmdbuffer[bufindw]);
    SERIAL_ECHOLNPGM("\"");
    commit_command(false);
}

//adds an command to the main command buffer
//thats really done in a non-safe way.
//needs overworking someday
void enquecommand(const char *cmd)
{
    prepareenque();
    //this is dangerous if a mixing of serial and this happens
    strcpy(cmdbuffer[bufindw], cmd);
    finishenque();
}

void enquecommand_P(const char *cmd)
{
    prepareenque();
    //this is dangerous if a mixing of serial and this happens
    strcpy_P(cmdbuffer[bufindw], cmd);
    finishenque();
}

uint8_t commands_queued()
{
    return buflen;
}

void cmd_synchronize()
{
  while(buflen)
  {
    // process next command
    next_command();
    idle();
    checkHitEndstops();
  }
}

void setup_killpin()
{
  #if defined(KILL_PIN) && KILL_PIN > -1
    SET_INPUT(KILL_PIN);
    WRITE(KILL_PIN,HIGH);
  #endif
}

void setup_photpin()
{
  #if defined(PHOTOGRAPH_PIN) && PHOTOGRAPH_PIN > -1
    SET_OUTPUT(PHOTOGRAPH_PIN);
    WRITE(PHOTOGRAPH_PIN, LOW);
  #endif
}

void setup_powerhold()
{
  #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
    SET_OUTPUT(SUICIDE_PIN);
    WRITE(SUICIDE_PIN, HIGH);
  #endif
  #if defined(PS_ON_PIN) && PS_ON_PIN > -1
    SET_OUTPUT(PS_ON_PIN);
    WRITE(PS_ON_PIN, PS_ON_AWAKE);
  #endif
}

void suicide()
{
  #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
    SET_OUTPUT(SUICIDE_PIN);
    WRITE(SUICIDE_PIN, LOW);
  #endif
}

void servo_init()
{
  #if (NUM_SERVOS >= 1) && defined(SERVO0_PIN) && (SERVO0_PIN > -1)
    servos[0].attach(SERVO0_PIN);
  #endif
  #if (NUM_SERVOS >= 2) && defined(SERVO1_PIN) && (SERVO1_PIN > -1)
    servos[1].attach(SERVO1_PIN);
  #endif
  #if (NUM_SERVOS >= 3) && defined(SERVO2_PIN) && (SERVO2_PIN > -1)
    servos[2].attach(SERVO2_PIN);
  #endif
  #if (NUM_SERVOS >= 4) && defined(SERVO3_PIN) && (SERVO3_PIN > -1)
    servos[3].attach(SERVO3_PIN);
  #endif
  #if (NUM_SERVOS >= 5)
    #error "TODO: enter initalisation code for more servos"
  #endif

  // Set position of Servo Endstops that are defined
  #ifdef SERVO_ENDSTOPS
  for(int8_t i = 0; i < 3; i++)
  {
    if(servo_endstops[i] > -1) {
      servos[servo_endstops[i]].write(servo_endstop_angles[i * 2 + 1]);
    }
  }
  #endif
}

void setup()
{
  setup_killpin();
  setup_powerhold();
  MYSERIAL.begin(BAUDRATE);
  SERIAL_PROTOCOLLNPGM("start");
  SERIAL_ECHO_START;

  // Check startup - does nothing if bootloader sets MCUSR to 0
  byte mcu = MCUSR;
  if(mcu & 1) SERIAL_ECHOLNPGM(MSG_POWERUP);
  if(mcu & 2) SERIAL_ECHOLNPGM(MSG_EXTERNAL_RESET);
  if(mcu & 4) SERIAL_ECHOLNPGM(MSG_BROWNOUT_RESET);
  if(mcu & 8) SERIAL_ECHOLNPGM(MSG_WATCHDOG_RESET);
  if(mcu & 32) SERIAL_ECHOLNPGM(MSG_SOFTWARE_RESET);
  MCUSR=0;

  SERIAL_ECHOPGM(MSG_MARLIN);
  SERIAL_ECHOLNPGM(VERSION_STRING);
  #ifdef STRING_VERSION_CONFIG_H
    #ifdef STRING_CONFIG_H_AUTHOR
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_CONFIGURATION_VER);
      SERIAL_ECHOPGM(STRING_VERSION_CONFIG_H);
      SERIAL_ECHOPGM(MSG_AUTHOR);
      SERIAL_ECHOLNPGM(STRING_CONFIG_H_AUTHOR);
      SERIAL_ECHOPGM("Compiled: ");
      SERIAL_ECHOLNPGM(__DATE__);
    #endif
  #endif
  SERIAL_ECHO_START;
  SERIAL_ECHOPGM(MSG_FREE_MEMORY);
  SERIAL_ECHO(freeMemory());
  SERIAL_ECHOPGM(MSG_PLANNER_BUFFER_BYTES);
  SERIAL_ECHOLN((int)sizeof(block_t)*BLOCK_BUFFER_SIZE);
  serialCmd = 0;

  // loads data from EEPROM if available else uses defaults (and resets step acceleration rate)
  Config_RetrieveSettings();
  PowerBudget_RetrieveSettings();
  lifetime_stats_init();
  tp_init();    // Initialize temperature loop
  plan_init();  // Initialize planner;
  filament_sensor_init(); // Initialize filament sensor
  watchdog_init();
  st_init();    // Initialize stepper, this enables interrupts!
  setup_photpin();
  servo_init();

  lcd_init();

  #if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1
    SET_OUTPUT(CONTROLLERFAN_PIN); //Set pin used for driver cooling fan
  #endif
}

void loop()
{
  if (printing_state == PRINT_STATE_ABORT)
  {
    abortPrint(true);
  }
  #ifdef SDSUPPORT
  card.checkautostart(false);
  #endif
  if(buflen)
  {
    // process next command
    next_command();
  }
  if(buflen < BUFSIZE)
  {
    // get next command
    get_command();
  }
  // manage heater and inactivity
  checkHitEndstops();
  idle();
}

FORCE_INLINE float code_value()
{
  return (strtod(strchr_pointer + 1, NULL));
}

FORCE_INLINE long code_value_long()
{
  return (strtol(strchr_pointer + 1, NULL, 10));
}

static bool code_seen(const char *cmd, char code)
{
  strchr_pointer = strchr(cmd, code);
  return (strchr_pointer != NULL);  //Return True if a character was found
}

/**
 * Copy a command directly into the main command buffer, from RAM.
 * Returns true if successfully adds the command
 */
static bool insertcommand(const char* cmd, bool isSerialCmd)
{
    if (*cmd == ';' || buflen >= BUFSIZE)
    {
        return false;
    }
    strcpy(cmdbuffer[bufindw], cmd);
    commit_command(isSerialCmd);
    return true;
}

static void gcode_line_error(const char* err, bool doFlush) {
  SERIAL_ERROR_START;
  serialprintPGM(err);
  SERIAL_ERRORLN(gcode_LastN);
  if (doFlush) FlushSerialRequestResend();
  serial_count = 0;
}

inline void get_serial_commands()
{
  long gcode_N;
  while( buflen < BUFSIZE && MYSERIAL.available() > 0)
  {
    char serial_char = MYSERIAL.read();
    /**
     * If the character ends the line
     */
    if (serial_char == '\n' || serial_char == '\r')
    {
      comment_mode = false; // end of line == end of comment
      if (!serial_count) continue; // skip empty lines

      cmd_line_buffer[serial_count] = 0; // terminate string
      serial_count = 0; //reset buffer

      char* command = cmd_line_buffer;
      while (*command == ' ') command++; // skip any leading spaces
      char* npos = (*command == 'N') ? command : NULL; // Require the N parameter to start the line
      char* apos = strchr(command, '*');

      if (npos) {

        boolean M110 = strstr_P(command, PSTR("M110")) != NULL;

        if (M110) {
          char* n2pos = strchr(command + 4, 'N');
          if (n2pos) npos = n2pos;
        }

        gcode_N = strtol(npos + 1, NULL, 10);

        if (gcode_N != gcode_LastN + 1 && !M110) {
          gcode_line_error(PSTR(MSG_ERR_LINE_NO), true);
          return;
        }

        if (apos) {
          byte checksum = 0, count = 0;
          while (command[count] != '*') checksum ^= command[count++];

          if (strtol(apos + 1, NULL, 10) != checksum) {
            gcode_line_error(PSTR(MSG_ERR_CHECKSUM_MISMATCH), true);
            return;
          }
          // if no errors, continue parsing
        }
        else {
          gcode_line_error(PSTR(MSG_ERR_NO_CHECKSUM), true);
          return;
        }

        gcode_LastN = gcode_N;
        // if no errors, continue parsing
      }
      else if (apos) { // No '*' without 'N'
        gcode_line_error(PSTR(MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM), false);
        return;
      }

      // Movement commands alert when stopped
      if (IsStopped()) {
        char* gpos = strchr(command, 'G');
        if (gpos) {
          int codenum = strtol(gpos + 1, NULL, 10);
          switch (codenum) {
            case 0:
            case 1:
            case 2:
            case 3:
              SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
              LCD_MESSAGEPGM(MSG_STOPPED);
              break;
          }
        }
      }

      // Add the command to the queue
#ifdef ENABLE_ULTILCD2
      // no printing screen for unrelated commands
      bool isSerialCmd = true;
      char* cmdpos = strchr(command, 'M');
      if (cmdpos)
      {
        if (++cmdpos)
        {
          int codenum = strtol(cmdpos, NULL, 10);
          switch (codenum) {
            case 20:
            case 21:
            case 22:
            case 27:
            case 105:
              isSerialCmd = false;
              break;
          }
        }
      }
      insertcommand(command, isSerialCmd);
#else
      insertcommand(command, true);
#endif

    }
    else if (serial_count >= MAX_CMD_SIZE - 1) {
      // Keep fetching, but ignore normal characters beyond the max length
      // The command will be injected when EOL is reached
    }
    else if (serial_char == '\\') {  // Handle escapes
      if (MYSERIAL.available() > 0) {
        // if we have one more character, copy it over
        serial_char = MYSERIAL.read();
        if (!comment_mode) cmd_line_buffer[serial_count++] = serial_char;
      }
      // otherwise do nothing
    }
    else { // it's not a newline, carriage return or escape char
      if (serial_char == ';') comment_mode = true;
      if (!comment_mode) cmd_line_buffer[serial_count++] = serial_char;
    }
  }
}

#ifdef SDSUPPORT
inline void get_sdcard_commands()
{
    if (!card.sdprinting() || card.pause() || (printing_state == PRINT_STATE_ABORT)) return;

    uint16_t sd_count = 0;
    char sd_char = '\0';
    static uint32_t endOfLineFilePosition = 0;

    bool card_eof = card.eof();
    while (buflen < BUFSIZE && !card_eof)
    {
        int16_t n = card.get();
        if (card.errorCode())
        {
            if (!card.sdInserted())
            {
                card.release();
                return;
            }

            //On an error, reset the error, reset the file position and try again.
            card.clearError();
            //Screw it, if we are near the end of a file with an error, act if the file is finished. Hopefully preventing the hang at the end.
            if (endOfLineFilePosition > card.getFileSize() - 512)
                card.stopPrinting();
            else
                card.setIndex(endOfLineFilePosition);

            return;
        }

        if (n<=0)
        {
            card_eof = true;
            sd_char = '\0';
        }
        else
        {
            card_eof = card.eof();
            sd_char = (char)n;
        }
        if (card_eof
            || sd_char == '\n' || sd_char == '\r'
            || ((sd_char == '#' || sd_char == ':') && !comment_mode))
        {
            if (card_eof) {
                SERIAL_PROTOCOLLNPGM(MSG_FILE_PRINTED);

                stoptime=millis();
                char time[30];
                unsigned long t=(stoptime-starttime)/1000;
                int minutes=(t/60)%60;
                int hours=t/60/60;
                sprintf_P(time, PSTR("%i hours %i minutes"),hours, minutes);
                SERIAL_ECHO_START;
                SERIAL_ECHOLN(time);
                lcd_setstatus(time);

                card.printingHasFinished();
                card.checkautostart(true);
            }

            comment_mode = false; //for new command

            if (!sd_count) continue; //skip empty lines

            cmd_line_buffer[sd_count] = '\0'; //terminate string
            sd_count = 0; //clear buffer
            endOfLineFilePosition = card.getFilePos();

            insertcommand(cmd_line_buffer, false);
        }
        else if (sd_count < MAX_CMD_SIZE - 1)
        {
            if (sd_char == ';')
            {
                comment_mode = true;
            }
            else if (!comment_mode)
            {
                // add char to line buffer
                cmd_line_buffer[sd_count++] = sd_char;
            }
        }
        /**
         * Keep fetching, but ignore normal characters beyond the max length
         * The command will be injected when EOL is reached
         */
    }
}
#endif //SDSUPPORT

static void get_command()
{
    if (printing_state != PRINT_STATE_ABORT)
    {
          get_serial_commands();
        #ifdef SDSUPPORT
          get_sdcard_commands();
        #endif //SDSUPPORT
    }
}

#define DEFINE_PGM_READ_ANY(type, reader)       \
    static inline type pgm_read_any(const type *p)  \
    { return pgm_read_##reader##_near(p); }

DEFINE_PGM_READ_ANY(float,       float);
DEFINE_PGM_READ_ANY(signed char, byte);

#define XYZ_CONSTS_FROM_CONFIG(type, array, CONFIG) \
static const PROGMEM type array##_P[3] =        \
    { X_##CONFIG, Y_##CONFIG, Z_##CONFIG };     \
static inline type array(int axis)          \
    { return pgm_read_any(&array##_P[axis]); }

//XYZ_CONSTS_FROM_CONFIG(float, base_min_pos,    MIN_POS);
//XYZ_CONSTS_FROM_CONFIG(float, base_max_pos,    MAX_POS);
//XYZ_CONSTS_FROM_CONFIG(float, base_home_pos,   HOME_POS);
//XYZ_CONSTS_FROM_CONFIG(float, max_length,      MAX_LENGTH);
XYZ_CONSTS_FROM_CONFIG(float, home_retract_mm, HOME_RETRACT_MM);
XYZ_CONSTS_FROM_CONFIG(signed char, home_dir,  HOME_DIR);

static void axis_is_at_home(int axis)
{
    float baseHomePos;
#ifdef BED_CENTER_AT_0_0
    float maxLength = max_pos[axis] - min_pos[axis];
#endif
    if (axis == Z_AXIS)
    {
        if (home_dir(axis) == -1)
        {
            baseHomePos = min_pos[axis];
        }
        else
        {
            baseHomePos = max_pos[axis];
        }
    }
    else
    {
        if (home_dir(axis) == -1)
        {
            #ifdef BED_CENTER_AT_0_0
                baseHomePos = maxLength * -0.5;
            #else
                baseHomePos = min_pos[axis];
            #endif
        }
        else
        {
            #ifdef BED_CENTER_AT_0_0
                baseHomePos = maxLength * 0.5;
            #else
                baseHomePos = max_pos[axis];
            #endif
        }
    }

    current_position[axis] = baseHomePos + add_homing[axis];
#if (EXTRUDERS > 1)
    if (axis <= Y_AXIS)
    {
        current_position[axis] += extruder_offset[axis][active_extruder];
    }
#endif
    // min_pos[axis] =          base_min_pos(axis);// + add_homing[axis];
    // max_pos[axis] =          base_max_pos(axis);// + add_homing[axis];
}

// Move the given axis to the home position.
// Movement is in two parts:
// 1) A quick move until the endstop switch is reached.
// 2) A short backup and then slow move to home for accurately determining the home position.
static void homeaxis(int axis) {
#define HOMEAXIS_DO(LETTER) \
  ((LETTER##_MIN_PIN > -1 && LETTER##_HOME_DIR==-1) || (LETTER##_MAX_PIN > -1 && LETTER##_HOME_DIR==1))
  if (axis==X_AXIS ? HOMEAXIS_DO(X) :
      axis==Y_AXIS ? HOMEAXIS_DO(Y) :
      axis==Z_AXIS ? HOMEAXIS_DO(Z) :
      0) {

    // Engage Servo endstop if enabled
    #ifdef SERVO_ENDSTOPS
      if (servo_endstops[axis] > -1) servos[servo_endstops[axis]].write(servo_endstop_angles[axis * 2]);
    #endif

    // Do a fast run to the home position.
    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
    destination[axis] = 1.5 * (max_pos[axis]-min_pos[axis]) * home_dir(axis);
    feedrate = homing_feedrate[axis];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
    if (!isEndstopHit())
    {
        if (axis == Z_AXIS)
        {
            //Move the bed upwards, as most likely something is stuck under the bed when we cannot reach the endstop.
            // We need to move up, as the Z screw is quite strong and will lodge the bed into a position where it is hard to remove by hand.
            current_position[axis] = 0;
            plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
            destination[axis] = -5 * home_dir(axis);
            plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
            //Disable the motor power so the bed can be moved by hand
            finishAndDisableSteppers();

            SERIAL_ERROR_START;
            SERIAL_ERRORLNPGM("Endstop not pressed after homing down. Endstop broken?");
            Stop(STOP_REASON_Z_ENDSTOP_BROKEN_ERROR);
        }else{
            SERIAL_ERROR_START;
            SERIAL_ERRORLNPGM("Endstop not pressed after homing down. Endstop broken?");
            Stop(STOP_REASON_XY_ENDSTOP_BROKEN_ERROR);
        }
        return;
    }

    // Move back a little bit.
    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
    destination[axis] = -home_retract_mm(axis) * home_dir(axis);
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();

    // Verify the endstop switch isn't stuck.
    bool endstop_pressed = false;
    switch(axis)
    {
    case X_AXIS:
        #if defined(X_MIN_PIN) && X_MIN_PIN > -1 && X_HOME_DIR == -1
        endstop_pressed = (READ(X_MIN_PIN) != X_ENDSTOPS_INVERTING);
        #endif
        #if defined(X_MAX_PIN) && X_MAX_PIN > -1 && X_HOME_DIR == 1
        endstop_pressed = (READ(X_MAX_PIN) != X_ENDSTOPS_INVERTING);
        #endif
        break;
    case Y_AXIS:
        #if defined(Y_MIN_PIN) && Y_MIN_PIN > -1 && Y_HOME_DIR == -1
        endstop_pressed = (READ(Y_MIN_PIN) != Y_ENDSTOPS_INVERTING);
        #endif
        #if defined(Y_MAX_PIN) && Y_MAX_PIN > -1 && Y_HOME_DIR == 1
        endstop_pressed = (READ(Y_MAX_PIN) != Y_ENDSTOPS_INVERTING);
        #endif
        break;
    case Z_AXIS:
        #if defined(Z_MIN_PIN) && Z_MIN_PIN > -1 && Z_HOME_DIR == -1
        endstop_pressed = (READ(Z_MIN_PIN) != Z_ENDSTOPS_INVERTING);
        #endif
        #if defined(Z_MAX_PIN) && Z_MAX_PIN > -1 && Z_HOME_DIR == 1
        endstop_pressed = (READ(Z_MAX_PIN) != Z_ENDSTOPS_INVERTING);
        #endif
        break;
    }
    if (endstop_pressed && axis == Z_AXIS)  // Temporarily verify Z-axis only until we know what incorrectly triggers the X- and Y-switches errors.
    {
        SERIAL_ERROR_START;
        SERIAL_ERRORLNPGM("Endstop still pressed after backing off. Endstop stuck?");
        if (axis == Z_AXIS)
            Stop(STOP_REASON_Z_ENDSTOP_STUCK_ERROR);
        else
            Stop(STOP_REASON_XY_ENDSTOP_STUCK_ERROR);
        endstops_hit_on_purpose();
        return;
    }

    // Do a second run at the home position, but now slower to be more accurate.
    destination[axis] = 2*home_retract_mm(axis) * home_dir(axis);
    feedrate = homing_feedrate[axis]/3;
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();

    axis_is_at_home(axis);
    destination[axis] = current_position[axis];
    feedrate = 0.0;
    endstops_hit_on_purpose();

    // Retract Servo endstop if enabled
    #ifdef SERVO_ENDSTOPS
      if (servo_endstops[axis] > -1) servos[servo_endstops[axis]].write(servo_endstop_angles[axis * 2 + 1]);
    #endif
  }
}
#define HOMEAXIS(LETTER) homeaxis(LETTER##_AXIS)

#if (TEMP_SENSOR_0 != 0) || (TEMP_SENSOR_BED != 0) || defined(HEATER_0_USES_MAX6675)
  static void print_heaterstates()
  {
    #if (TEMP_SENSOR_0 != 0) || defined(HEATER_0_USES_MAX6675)
      SERIAL_PROTOCOLPGM(" T:");
      SERIAL_PROTOCOL_F(degHotend(tmp_extruder), 1);
      SERIAL_PROTOCOLPGM(" /");
      SERIAL_PROTOCOL_F(degTargetHotend(tmp_extruder), 1);
    #endif
    #if (TEMP_SENSOR_BED != 0)
      SERIAL_PROTOCOLPGM(" B:");
      SERIAL_PROTOCOL_F(degBed(), 1);
      SERIAL_PROTOCOLPGM(" /");
      SERIAL_PROTOCOL_F(degTargetBed(), 1);
    #endif
    #if EXTRUDERS > 1
      int8_t index;
      for (int8_t e = 0; e < EXTRUDERS; ++e) {
        index = (swapExtruders() ? e^0x01 : e);
        SERIAL_PROTOCOLPGM(" T");
        SERIAL_PROTOCOL(e);
        SERIAL_PROTOCOLCHAR(':');
        SERIAL_PROTOCOL_F(degHotend(index), 1);
        SERIAL_PROTOCOLPGM(" /");
        SERIAL_PROTOCOL_F(degTargetHotend(index), 1);
      }
    #endif
    #if (TEMP_SENSOR_BED != 0)
      SERIAL_PROTOCOLPGM(" B@:");
      SERIAL_PROTOCOL(getHeaterPower(-1));
    #endif
    SERIAL_PROTOCOLPGM(" @:");
    SERIAL_PROTOCOL(getHeaterPower(tmp_extruder));
    #if EXTRUDERS > 1
      for (int8_t e = 0; e < EXTRUDERS; ++e) {
        index = (swapExtruders() ? e^0x01 : e);
        SERIAL_PROTOCOLPGM(" @");
        SERIAL_PROTOCOL(e);
        SERIAL_PROTOCOLCHAR(':');
        SERIAL_PROTOCOL(getHeaterPower(index));
      }
    #endif
  }
#endif

/**
 * M105: Read hot end and bed temperature
 */
inline void gcode_M105(const char *cmd)
{
  if (setTargetedHotend(cmd, 105)) return;
  #if (TEMP_SENSOR_0 != 0) || (TEMP_SENSOR_BED != 0) || defined(HEATER_0_USES_MAX6675)
    SERIAL_PROTOCOLPGM(MSG_OK);
    print_heaterstates();
    SERIAL_EOL;
  #else // !HAS_TEMP_0 && !HAS_TEMP_BED
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_ERR_NO_THERMISTORS);
  #endif
}

/**
 * G92: Set current position to given X Y Z E
 */
inline void gcode_G92(const char *cmd)
{
  // bool didE = code_seen(cmd, axis_codes[E_AXIS]);
  // if (!didE) st_synchronize();

  bool didXYZ = false;
  bool didE   = false;
  for(uint8_t i=0; i < NUM_AXIS; ++i)
  {
    if (code_seen(cmd, axis_codes[i]))
    {
      current_position[i] = code_value();
      if (i == E_AXIS)
      {
         didE = true;
      }
      else
      {
        didXYZ = true;
      }
    }
  }
  if (didXYZ)
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
  else if (didE)
    plan_set_e_position(current_position[E_AXIS], active_extruder, false);
}

static char * truncate_checksum(char *str)
{
    if (*str)
    {
        char *starpos = strchr(str, '*');
        if(starpos)
        {
            *starpos='\0';
        }
        return starpos;
    }
    return 0;
}

void process_command(const char *strCmd, bool sendAck)
{
  unsigned long codenum; //throw away variable

  if ((printing_state != PRINT_STATE_RECOVER) && (printing_state != PRINT_STATE_START) && (printing_state != PRINT_STATE_ABORT))
    printing_state = PRINT_STATE_NORMAL;

  if(code_seen(strCmd, 'G'))
  {
    switch((int)code_value())
    {
    case 0: // G0 -> G1
    case 1: // G1
      if(!Stopped) {
        get_coordinates(strCmd); // For X Y Z E F
        prepare_move(strCmd);
        if (sendAck) ClearToSend();
        return;
      }
      //break;
    case 2: // G2  - CW ARC
      if(!Stopped) {
        get_arc_coordinates(strCmd);
        prepare_arc_move(true);
        if (sendAck) ClearToSend();
        return;
      }
    case 3: // G3  - CCW ARC
      if(!Stopped) {
        get_arc_coordinates(strCmd);
        prepare_arc_move(false);
        if (sendAck) ClearToSend();
        return;
      }
    case 4: // G4 dwell
      if (printing_state == PRINT_STATE_RECOVER)
        break;

      serial_action_P(PSTR("pause"));
      LCD_MESSAGEPGM(MSG_DWELL);
      codenum = 0;
      if(code_seen(strCmd, 'P')) codenum = code_value(); // milliseconds to wait
      if(code_seen(strCmd, 'S')) codenum = code_value() * 1000; // seconds to wait

      st_synchronize();
      codenum += millis();  // keep track of when we started waiting
      previous_millis_cmd = millis();
      printing_state = PRINT_STATE_DWELL;
      while(millis() < codenum )
      {
          idle();
      }
      serial_action_P(PSTR("resume"));

      break;
      #ifdef FWRETRACT
      case 10: // G10 retract
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      if(!retracted)
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS];
        #if EXTRUDERS > 1
        if (code_seen(strCmd, 'S') && code_value_long() == 1)
            destination[E_AXIS]=current_position[E_AXIS]-extruder_swap_retract_length/volume_to_filament_length[active_extruder];
        else
            destination[E_AXIS]=current_position[E_AXIS]-retract_length/volume_to_filament_length[active_extruder];
        #else
        destination[E_AXIS]=current_position[E_AXIS]-retract_length/volume_to_filament_length[active_extruder];
        #endif
        float oldFeedrate = feedrate;
        feedrate=retract_feedrate;
        retract_recover_length = current_position[E_AXIS]-destination[E_AXIS];//Set the recover length to whatever distance we retracted so we recover properly.
        retracted=true;
        prepare_move(strCmd);
        feedrate = oldFeedrate;
      }

      break;
      case 11: // G11 retract_recover
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      if(retracted)
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS];
        destination[E_AXIS]=current_position[E_AXIS]+retract_recover_length;
        float oldFeedrate = feedrate;
        feedrate=retract_recover_feedrate;
        retracted=false;
        prepare_move(strCmd);
        feedrate = oldFeedrate;
      }
      break;
      #endif //FWRETRACT
    case 28: //G28 Home all Axis one at a time
      if ((printing_state == PRINT_STATE_RECOVER) || (printing_state == PRINT_STATE_HOMING))
      {
          break;
      }

      if ((printing_state != PRINT_STATE_START) && (printing_state != PRINT_STATE_ABORT))
      {
        printing_state = PRINT_STATE_HOMING;
      }

      st_synchronize();

      {
        int   saved_feedmultiply = feedmultiply;
        float saved_feedrate = feedrate;
        feedmultiply = 100;
        previous_millis_cmd = millis();

        enable_endstops(true);

        memcpy(destination, current_position, sizeof(destination));
        feedrate = 0.0;

#ifdef DELTA
          // A delta can only safely home all axis at the same time
          // all axis have to home at the same time

          // Move all carriages up together until the first endstop is hit.
          current_position[X_AXIS] = 0;
          current_position[Y_AXIS] = 0;
          current_position[Z_AXIS] = 0;
          plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);

          destination[X_AXIS] = 3 * AXIS_LENGTH(Z_AXIS);
          destination[Y_AXIS] = 3 * AXIS_LENGTH(Z_AXIS);
          destination[Z_AXIS] = 3 * AXIS_LENGTH(Z_AXIS);
          feedrate = 1.732 * homing_feedrate[X_AXIS];
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
          st_synchronize();
          endstops_hit_on_purpose();

          current_position[X_AXIS] = destination[X_AXIS];
          current_position[Y_AXIS] = destination[Y_AXIS];
          current_position[Z_AXIS] = destination[Z_AXIS];

          // take care of back off and rehome now we are all at the top
          HOMEAXIS(X);
          HOMEAXIS(Y);
          HOMEAXIS(Z);

          calculate_delta(current_position);
          plan_set_position(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS], current_position[E_AXIS], active_extruder, true);

#else // NOT DELTA

          home_all_axis = !((code_seen(strCmd, axis_codes[X_AXIS])) || (code_seen(strCmd, axis_codes[Y_AXIS])) || (code_seen(strCmd, axis_codes[Z_AXIS])));

          #if Z_HOME_DIR > 0                      // If homing away from BED do Z first
          #if defined(QUICK_HOME)
          if(home_all_axis)
          {
            current_position[X_AXIS] = 0; current_position[Y_AXIS] = 0; current_position[Z_AXIS] = 0;

            plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);

            destination[X_AXIS] = 1.5 * AXIS_LENGTH(X_AXIS) * X_HOME_DIR;
            destination[Y_AXIS] = 1.5 * AXIS_LENGTH(Y_AXIS) * Y_HOME_DIR;
            destination[Z_AXIS] = 1.5 * AXIS_LENGTH(Z_AXIS) * Z_HOME_DIR;
            feedrate = homing_feedrate[X_AXIS];
            plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
            st_synchronize();
            endstops_hit_on_purpose();

            axis_is_at_home(X_AXIS);
            axis_is_at_home(Y_AXIS);
            axis_is_at_home(Z_AXIS);
            plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
            destination[X_AXIS] = current_position[X_AXIS];
            destination[Y_AXIS] = current_position[Y_AXIS];
            destination[Z_AXIS] = current_position[Z_AXIS];
            plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
            feedrate = 0.0;
            st_synchronize();
            endstops_hit_on_purpose();

            current_position[X_AXIS] = destination[X_AXIS];
            current_position[Y_AXIS] = destination[Y_AXIS];
            current_position[Z_AXIS] = destination[Z_AXIS];
          }
          #endif
          if((home_all_axis) || (code_seen(strCmd, axis_codes[Z_AXIS]))) {
            HOMEAXIS(Z);
          }
          #endif

          #if defined(QUICK_HOME)
          if((home_all_axis)||( code_seen(strCmd, axis_codes[X_AXIS]) && code_seen(strCmd, axis_codes[Y_AXIS])) )  //first diagonal move
          {
            current_position[X_AXIS] = 0;current_position[Y_AXIS] = 0;

            plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
            destination[X_AXIS] = 1.5 * AXIS_LENGTH(X_AXIS) * X_HOME_DIR;
            destination[Y_AXIS] = 1.5 * AXIS_LENGTH(Y_AXIS) * Y_HOME_DIR;
            feedrate = homing_feedrate[X_AXIS];
            if(homing_feedrate[Y_AXIS]<feedrate)
              feedrate =homing_feedrate[Y_AXIS];
            plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
            st_synchronize();

            axis_is_at_home(X_AXIS);
            axis_is_at_home(Y_AXIS);
            plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
            destination[X_AXIS] = current_position[X_AXIS];
            destination[Y_AXIS] = current_position[Y_AXIS];
            plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
            feedrate = 0.0;
            st_synchronize();
            endstops_hit_on_purpose();

            current_position[X_AXIS] = destination[X_AXIS];
            current_position[Y_AXIS] = destination[Y_AXIS];
            current_position[Z_AXIS] = destination[Z_AXIS];
          }
          #endif

          if((home_all_axis) || (code_seen(strCmd, axis_codes[X_AXIS])))
          {
            HOMEAXIS(X);
          }

          if((home_all_axis) || (code_seen(strCmd, axis_codes[Y_AXIS]))) {
            HOMEAXIS(Y);
          }

          #if Z_HOME_DIR < 0                      // If homing towards BED do Z last
          if((home_all_axis) || (code_seen(strCmd, axis_codes[Z_AXIS]))) {
            HOMEAXIS(Z);
          }
          #endif

          if(code_seen(strCmd, axis_codes[X_AXIS]))
          {
            if(code_value_long() != 0) {
              current_position[X_AXIS]=code_value()+add_homing[X_AXIS];
            }
          }

          if(code_seen(strCmd, axis_codes[Y_AXIS])) {
            if(code_value_long() != 0) {
              current_position[Y_AXIS]=code_value()+add_homing[Y_AXIS];
            }
          }

          if(code_seen(strCmd, axis_codes[Z_AXIS])) {
            if(code_value_long() != 0) {
              current_position[Z_AXIS]=code_value()+add_homing[Z_AXIS];
            }
          }
          plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
    #endif // DELTA

      #ifdef ENDSTOPS_ONLY_FOR_HOMING
        enable_endstops(false);
      #endif

        feedrate = saved_feedrate;
        feedmultiply = saved_feedmultiply;
        previous_millis_cmd = millis();
        endstops_hit_on_purpose();
      }
      break;
    case 90: // G90
      // relative_mode = false;
      axis_relative_state &= ~RELATIVE_MODE;
      break;
    case 91: // G91
      // relative_mode = true;
      axis_relative_state |= RELATIVE_MODE;
      break;
    case 92: // G92
      gcode_G92(strCmd);
      break;
    }
  }

  else if(code_seen(strCmd, 'M'))
  {
    switch( (int)code_value() )
    {
#ifdef ULTIPANEL
    case 0: // M0 - Unconditional stop - Wait for user button press on LCD
    case 1: // M1 - Conditional stop - Wait for user button press on LCD
    {
      if ((printing_state == PRINT_STATE_RECOVER) || (printing_state == PRINT_STATE_ABORT))
        break;

      printing_state = PRINT_STATE_WAIT_USER;
      LCD_MESSAGEPGM(MSG_USERWAIT);

      serial_action_P(PSTR("paused"));

      codenum = 0;
      if(code_seen(strCmd, 'P')) codenum = code_value(); // milliseconds to wait
      if(code_seen(strCmd, 'S')) codenum = code_value() * 1000; // seconds to wait

      st_synchronize();
      previous_millis_cmd = millis();
      if (codenum > 0)
      {
        codenum += millis();  // keep track of when we started waiting
        while(millis()  < codenum && !lcd_clicked()){
          idle();
        }
      }
      else
      {
        while(!lcd_clicked())
        {
          idle();
        }
      }
      serial_action_P(PSTR("resumed"));
      LCD_MESSAGEPGM(MSG_RESUMING);
    }
    break;
#endif
#ifdef ENABLE_ULTILCD2
    case 0: // M0 - Unconditional stop - Wait for user button press on LCD
    case 1: // M1 - Conditional stop - Wait for user button press on LCD
    {
        if (printing_state == PRINT_STATE_RECOVER)
          break;

        card.pauseSDPrint();
        serial_action_P(PSTR("paused"));
        while(card.pause())
        {
          idle();
        }
        plan_set_e_position(current_position[E_AXIS], active_extruder, true);
        serial_action_P(PSTR("resumed"));
    }
    break;
#endif
    case 17:
        if (printing_state == PRINT_STATE_RECOVER)
          break;
        LCD_MESSAGEPGM(MSG_NO_MOVE);
        enable_x();
        enable_y();
        enable_z();
        enable_e0();
        enable_e1();
        enable_e2();
      break;

#ifdef SDSUPPORT
    case 20: // M20 - list SD card
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      SERIAL_PROTOCOLLNPGM(MSG_BEGIN_FILE_LIST);
      card.ls();
      SERIAL_PROTOCOLLNPGM(MSG_END_FILE_LIST);
      ClearToSend();
      return;
    case 21: // M21 - init SD card
      if (printing_state == PRINT_STATE_RECOVER)
        break;

      card.initsd();
      ClearToSend();
      return;
    case 22: //M22 - release SD card
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      card.release();
      ClearToSend();
      return;
    case 23: //M23 - Select file
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      strchr_pointer += 4;
      truncate_checksum(strchr_pointer);
      card.openFile(strchr_pointer, true);
      break;
    case 24: //M24 - Start SD print
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      card.startFileprint();
      starttime=millis();
      stoptime=starttime;
      break;
    case 25: //M25 - Pause SD print
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      //card.pauseSDPrint();
      card.closefile();
      break;
    case 26: //M26 - Set SD index
      if(card.isOk() && code_seen(strCmd, 'S')) {
        card.setIndex(code_value_long());
      }
      break;
    case 27: //M27 - Get SD status
      card.getStatus();
      ClearToSend();
      return;
    case 28: //M28 - Start SD write
      strchr_pointer += 4;
      if(truncate_checksum(strchr_pointer)){
        char* npos = strchr(strCmd, 'N');
        strchr_pointer = strchr(npos,' ') + 1;
      }
      card.openFile(strchr_pointer, false);
      break;
    case 29: //M29 - Stop SD write
      //processed in write to file routine above
      //card,saving = false;
      break;
    case 30: //M30 <filename> Delete File
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      if (card.isOk()){
        card.closefile();
        strchr_pointer += 4;
        if(truncate_checksum(strchr_pointer)){
          char* npos = strchr(strCmd, 'N');
          strchr_pointer = strchr(npos,' ') + 1;
        }
        card.removeFile(strchr_pointer);
      }
      break;
    case 923: //M923 - Select file and start printing
      strchr_pointer += 5;
      truncate_checksum(strchr_pointer);
      card.openFile(strchr_pointer,true);
      card.startFileprint();
      starttime=millis();
      stoptime=starttime;
      break;
    case 928: //M928 - Start SD write
      strchr_pointer += 5;
      if(truncate_checksum(strchr_pointer)){
        char* npos = strchr(strCmd, 'N');
        strchr_pointer = strchr(npos,' ') + 1;
      }
      card.openLogFile(strchr_pointer);
      break;

#endif //SDSUPPORT

    case 31: //M31 take time since the start of the SD print or an M109 command
      {
      stoptime=millis();
      char time[30];
      unsigned long t=(stoptime-starttime)/1000;
      int sec,min;
      min=t/60;
      sec=t%60;
      sprintf_P(time, PSTR("%i min, %i sec"), min, sec);
      SERIAL_ECHO_START;
      SERIAL_ECHOLN(time);
      lcd_setstatus(time);
      autotempShutdown();
      }
      break;
    case 42: //M42 -Change pin status via gcode
      if (code_seen(strCmd, 'S'))
      {
        int pin_status = code_value();
        int pin_number = LED_PIN;
        if (code_seen(strCmd, 'P') && pin_status >= 0 && pin_status <= 255)
          pin_number = code_value();

        for(uint8_t i = 0; i < COUNT(sensitive_pins); ++i)
        {
          if (sensitive_pins[i] == pin_number)
          {
            pin_number = -1;
            break;
          }
        }
      #if defined(FAN_PIN) && FAN_PIN > -1
        if (pin_number == FAN_PIN)
          fanSpeed = pin_status;
      #endif
        if (pin_number > -1)
        {
          analogWrite(pin_number, pin_status);
        }
      }
     break;
    case 104: // M104
      if(setTargetedHotend(strCmd, 104)){
        break;
      }
      if (code_seen(strCmd, 'S'))
      {
        float newTemperature = code_value();
        setTargetHotend(roundTemperature(newTemperature), tmp_extruder);
      }
      if (printing_state != PRINT_STATE_RECOVER)
      {
        setWatch();
      }
      break;
    case 140: // M140 set bed temp
#if TEMP_SENSOR_BED != 0
      if (code_seen(strCmd, 'S')) setTargetBed(code_value());
#endif // TEMP_SENSOR_BED
      break;
    case 105 : // M105
        gcode_M105(strCmd);
        return; // "ok" already printed
      break;
    case 109:
    {// M109 - Wait for extruder heater to reach target.
      if (printing_state == PRINT_STATE_ABORT)
      {
        break;
      }
      if(setTargetedHotend(strCmd, 109))
	  {
        break;
      }
      #ifdef AUTOTEMP
        autotemp_enabled=false;
      #endif
      if (code_seen(strCmd, 'S'))
      {
        float newTemperature = code_value();
        setTargetHotend(roundTemperature(newTemperature), tmp_extruder);
      }

      #ifdef AUTOTEMP
        if (code_seen(strCmd, 'S')) autotemp_min=code_value();
        if (code_seen(strCmd, 'B')) autotemp_max=code_value();
        if (code_seen(strCmd, 'F'))
        {
          autotemp_factor=code_value();
          autotemp_enabled=true;
        }
      #endif
      if (printing_state == PRINT_STATE_RECOVER)
          break;

      printing_state = PRINT_STATE_HEATING;
      LCD_MESSAGEPGM(MSG_HEATING);

      setWatch();
      codenum = millis();

      /* See if we are heating up or cooling down */
      bool target_direction = isHeatingHotend(tmp_extruder); // true if heating, false if cooling

      #ifdef TEMP_RESIDENCY_TIME
        long residencyStart = -1;
        /* continue to loop until we have reached the target temp
          _and_ until TEMP_RESIDENCY_TIME hasn't passed since we reached it */
        while((residencyStart == -1) ||
              (residencyStart >= 0 && (((unsigned int) (millis() - residencyStart)) < (TEMP_RESIDENCY_TIME * 1000UL))) )
        {
      #else
        while ( target_direction ? (isHeatingHotend(tmp_extruder)) : (isCoolingHotend(tmp_extruder)&&(CooldownNoWait==false)) )
        {
      #endif //TEMP_RESIDENCY_TIME
          if( (millis() - codenum) > 1000UL )
          { //Print Temp Reading and remaining time every 1 second while heating up/cooling down
            #if (TEMP_SENSOR_0 != 0) || (TEMP_SENSOR_BED != 0) || defined(HEATER_0_USES_MAX6675)
              print_heaterstates();
            #endif
            #ifdef TEMP_RESIDENCY_TIME
              SERIAL_PROTOCOLPGM(" W:");
              if(residencyStart > -1)
              {
                 codenum = ((TEMP_RESIDENCY_TIME * 1000UL) - (millis() - residencyStart)) / 1000UL;
                 SERIAL_PROTOCOLLN( codenum );
              }
              else
              {
                 SERIAL_PROTOCOLLNPGM( "?" );
              }
            #else
              SERIAL_EOL;
            #endif
            codenum = millis();
          }
          idle();
        #ifdef TEMP_RESIDENCY_TIME
            /* start/restart the TEMP_RESIDENCY_TIME timer whenever we reach target temp for the first time
              or when current temp falls outside the hysteresis after target temp was reached */
          if ((residencyStart == -1 &&  target_direction && (degHotend(tmp_extruder) >= (degTargetHotend(tmp_extruder)-TEMP_WINDOW))) ||
              (residencyStart == -1 && !target_direction && (degHotend(tmp_extruder) <= (degTargetHotend(tmp_extruder)+TEMP_WINDOW))) ||
              (residencyStart > -1 && labs(degHotend(tmp_extruder) - degTargetHotend(tmp_extruder)) > TEMP_HYSTERESIS && (!target_direction || !CooldownNoWait)) )
          {
            residencyStart = millis();
          }
        #endif //TEMP_RESIDENCY_TIME
          if (printing_state != PRINT_STATE_HEATING)
          {
              // print aborted
              break;
          }
        }
        LCD_MESSAGEPGM(MSG_HEATING_COMPLETE);
        previous_millis_cmd = millis();
      }
      break;
    case 190: // M190 - Wait for bed heater to reach target.
    #if defined(TEMP_BED_PIN) && TEMP_BED_PIN > -1 && TEMP_SENSOR_BED != 0
        if (code_seen(strCmd, 'S')) setTargetBed(code_value());

        if ((printing_state == PRINT_STATE_RECOVER) || (printing_state == PRINT_STATE_ABORT))
            break;

        printing_state = PRINT_STATE_HEATING_BED;
        LCD_MESSAGEPGM(MSG_BED_HEATING);

        codenum = millis();
        unsigned long m;

        #if EXTRUDERS > 1
        // set targeted hotend for serial output
        tmp_extruder = active_extruder;
        if (swapExtruders() && (tmp_extruder < 2))
        {
          tmp_extruder ^= 0x01;
        }
        #endif // EXTRUDERS

        while(current_temperature_bed < degTargetBed() - TEMP_WINDOW)
        {
          m = millis();
          if((m - codenum) > 1000 ) //Print Temp Reading every 1 second while heating up.
          {
            codenum = m;
            // float tt=degHotend(active_extruder);
          #if (TEMP_SENSOR_0 != 0) || (TEMP_SENSOR_BED != 0) || defined(HEATER_0_USES_MAX6675)
            print_heaterstates();
            SERIAL_EOL;
          #endif
          }
          idle();
          if (printing_state != PRINT_STATE_HEATING_BED)
          {
              // print aborted
              break;
          }
        }
        LCD_MESSAGEPGM(MSG_BED_DONE);
        previous_millis_cmd = millis();
    #endif
        break;

    #if defined(FAN_PIN) && FAN_PIN > -1
      case 106: //M106 Fan On
        if (code_seen(strCmd, 'S')){
           fanSpeed=constrain((int)code_value() * fanSpeedPercent / 100, 0, 255);
        }
        else {
          fanSpeed = 255 * int(fanSpeedPercent) / 100;
        }
        break;
      case 107: //M107 Fan Off
        fanSpeed = 0;
        break;
    #endif //FAN_PIN
    #ifdef BARICUDA
      // PWM for HEATER_1_PIN
      #if defined(HEATER_1_PIN) && HEATER_1_PIN > -1
        case 126: //M126 valve open
          if (printing_state == PRINT_STATE_RECOVER)
            break;
          if (code_seen(strCmd, 'S')){
             ValvePressure=constrain((int)code_value(),0,255);
          }
          else {
            ValvePressure=255;
          }
          break;
        case 127: //M127 valve closed
          if (printing_state == PRINT_STATE_RECOVER)
            break;
          ValvePressure = 0;
          break;
      #endif //HEATER_1_PIN

      // PWM for HEATER_2_PIN
      #if defined(HEATER_2_PIN) && HEATER_2_PIN > -1
        case 128: //M128 valve open
          if (printing_state == PRINT_STATE_RECOVER)
            break;
          if (code_seen(strCmd, 'S')){
             EtoPPressure=constrain((int)code_value(),0,255);
          }
          else {
            EtoPPressure=255;
          }
          break;
        case 129: //M129 valve closed
          if (printing_state == PRINT_STATE_RECOVER)
            break;
          EtoPPressure = 0;
          break;
      #endif //HEATER_2_PIN
    #endif

    #if defined(PS_ON_PIN) && PS_ON_PIN > -1
      case 80: // M80 - ATX Power On
        SET_OUTPUT(PS_ON_PIN); //GND
        WRITE(PS_ON_PIN, PS_ON_AWAKE);
        break;
      #endif

      case 81: // M81 - ATX Power Off

      #if defined(SUICIDE_PIN) && SUICIDE_PIN > -1
        st_synchronize();
        suicide();
      #elif defined(PS_ON_PIN) && PS_ON_PIN > -1
        SET_OUTPUT(PS_ON_PIN);
        WRITE(PS_ON_PIN, PS_ON_ASLEEP);
      #endif
        break;

    case 82:
      // axis_relative_modes[E_AXIS] = false;
      axis_relative_state &= ~(1 << E_AXIS);
      break;
    case 83:
      // axis_relative_modes[E_AXIS] = true;
      axis_relative_state |= (1 << E_AXIS);
      break;
    case 18: //compatibility
    case 84: // M84
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      if(code_seen(strCmd, 'S')){
#if DISABLE_X || DISABLE_Y || DISABLE_Z || DISABLE_E
        stepper_inactive_time = code_value() * 1000;
#endif
      }
      else
      {
        bool all_axis = !((code_seen(strCmd, axis_codes[X_AXIS])) || (code_seen(strCmd, axis_codes[Y_AXIS])) || (code_seen(strCmd, axis_codes[Z_AXIS]))|| (code_seen(strCmd, axis_codes[E_AXIS])));
        if(all_axis)
        {
          finishAndDisableSteppers();
        }
        else
        {
          st_synchronize();
          if(code_seen(strCmd, 'X')) disable_x();
          if(code_seen(strCmd, 'Y')) disable_y();
          if(code_seen(strCmd, 'Z')) disable_z();
          #if ((E0_ENABLE_PIN != X_ENABLE_PIN) && (E1_ENABLE_PIN != Y_ENABLE_PIN)) // Only enable on boards that have seperate ENABLE_PINS
            if(code_seen(strCmd, 'E')) {
              disable_e0();
              disable_e1();
              disable_e2();
          #if EXTRUDERS > 1
              last_extruder = 0xFF;
          #endif
            }
          #endif
        }
      }
      break;
    case 85: // M85
      if (code_seen(strCmd, 'S')) max_inactive_time = code_value() * 1000;
      break;
    case 92: // M92
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(strCmd, axis_codes[i]))
        {
          if(i == E_AXIS)
          {
            float value = code_value();
            if(value < 20.0) {
              float factor = e_steps_per_unit(active_extruder) / value; // increase e constants if M92 E14 is given for netfab.
              max_e_jerk *= factor;
              max_feedrate[i] *= factor;
              axis_steps_per_sqr_second[i] *= factor;
#if EXTRUDERS > 1
              axis_steps_per_sqr_second[i+1] *= factor;
#endif // EXTRUDERS
            }
#if EXTRUDERS > 1
            if (active_extruder) {
                e2_steps_per_unit = value;
            }
            else{
                axis_steps_per_unit[i] = value;
            }
#else
            axis_steps_per_unit[i] = value;
#endif // EXTRUDERS
          }
          else {
            axis_steps_per_unit[i] = code_value();
          }
        }
      }
      plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);
      break;
    case 114: // M114
      SERIAL_PROTOCOLPGM("X:");
      SERIAL_PROTOCOL(current_position[X_AXIS]);
      SERIAL_PROTOCOLPGM("Y:");
      SERIAL_PROTOCOL(current_position[Y_AXIS]);
      SERIAL_PROTOCOLPGM("Z:");
      SERIAL_PROTOCOL(current_position[Z_AXIS]);
      SERIAL_PROTOCOLPGM("E:");
      SERIAL_PROTOCOL(current_position[E_AXIS]);

      SERIAL_PROTOCOLPGM(MSG_COUNT_X);
      SERIAL_PROTOCOL(float(st_get_position(X_AXIS))/axis_steps_per_unit[X_AXIS]);
      SERIAL_PROTOCOLPGM("Y:");
      SERIAL_PROTOCOL(float(st_get_position(Y_AXIS))/axis_steps_per_unit[Y_AXIS]);
      SERIAL_PROTOCOLPGM("Z:");
      SERIAL_PROTOCOL(float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS]);
      SERIAL_PROTOCOLPGM("E:");
      SERIAL_PROTOCOL(float(st_get_position(E_AXIS))/e_steps_per_unit(active_extruder));

      SERIAL_EOL;
      break;
    case 115: // M115
      SERIAL_PROTOCOLPGM(MSG_M115_REPORT);
      break;
    case 117: // M117 display message
      truncate_checksum(strchr_pointer);
      if (strlen(strchr_pointer) > 5)
      {
        lcd_setstatus(strchr_pointer+5);
      }
      else
      {
        lcd_clearstatus();
      }
      break;
    case 120: // M120
      enable_endstops(false) ;
      break;
    case 121: // M121
      enable_endstops(true) ;
      break;
    case 119: // M119
    SERIAL_PROTOCOLLNPGM(MSG_M119_REPORT);
      #if defined(X_MIN_PIN) && X_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_X_MIN);
        if (READ(X_MIN_PIN)^X_ENDSTOPS_INVERTING)
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_HIT);
        }
        else
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_OPEN);
        }
      #endif
      #if defined(X_MAX_PIN) && X_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_X_MAX);
        if (READ(X_MAX_PIN)^X_ENDSTOPS_INVERTING)
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_HIT);
        }
        else
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_OPEN);
        }
      #endif
      #if defined(Y_MIN_PIN) && Y_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Y_MIN);
        if (READ(Y_MIN_PIN)^Y_ENDSTOPS_INVERTING)
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_HIT);
        }
        else
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_OPEN);
        }
      #endif
      #if defined(Y_MAX_PIN) && Y_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Y_MAX);
        if (READ(Y_MAX_PIN)^Y_ENDSTOPS_INVERTING)
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_HIT);
        }
        else
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_OPEN);
        }
      #endif
      #if defined(Z_MIN_PIN) && Z_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Z_MIN);
        if (READ(Z_MIN_PIN)^Z_ENDSTOPS_INVERTING)
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_HIT);
        }
        else
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_OPEN);
        }
      #endif
      #if defined(Z_MAX_PIN) && Z_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Z_MAX);
        if (READ(Z_MAX_PIN)^Z_ENDSTOPS_INVERTING)
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_HIT);
        }
        else
        {
            SERIAL_PROTOCOLLNPGM(MSG_ENDSTOP_OPEN);
        }
      #endif
      break;
      //TODO: update for all axis, use for loop
    case 200: // M200 - set filament diameter
      if(setTargetedHotend(strCmd, 200)){
        break;
      }
      if(code_seen(strCmd, 'D'))
      {
          float radius = code_value() / 2;
          if (abs(radius) < 0.01f)
          {
              volume_to_filament_length[tmp_extruder] = 1.0f;
          }
          else
          {
              volume_to_filament_length[tmp_extruder] = 1.0f / (M_PI * radius * radius);
          }
      }
      break;
    case 201: // M201
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(strCmd, axis_codes[i]))
        {
          max_acceleration_units_per_sq_second[i] = code_value();
        }
      }
      // steps per sq second need to be updated to agree with the units per sq second (as they are what is used in the planner)
      reset_acceleration_rates();
      break;
    #if 0 // Not used for Sprinter/grbl gen6
    case 202: // M202
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(strCmd, axis_codes[i])) axis_travel_steps_per_sqr_second[i] = code_value() * axis_steps_per_unit[i];
      }
      break;
    #endif
    case 203: // M203 max feedrate mm/sec
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(strCmd, axis_codes[i])) max_feedrate[i] = code_value();
      }
      break;
    case 204: // M204 acceleration: S - normal moves;  T - filament only moves
      {
        if(code_seen(strCmd, 'S')) acceleration = code_value() ;
        if(code_seen(strCmd, 'T')) retract_acceleration = code_value() ;
      }
      break;
    case 205: //M205 advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk
    {
      if(code_seen(strCmd, 'S')) minimumfeedrate = code_value();
      if(code_seen(strCmd, 'T')) mintravelfeedrate = code_value();
      if(code_seen(strCmd, 'B')) minsegmenttime = code_value() ;
      if(code_seen(strCmd, 'X')) max_xy_jerk = code_value() ;
      if(code_seen(strCmd, 'Z')) max_z_jerk = code_value() ;
      if(code_seen(strCmd, 'E')) max_e_jerk = code_value() ;
    }
    break;
    case 206: // M206 additional homing offset
      for(uint8_t i=0; i < 3; ++i)
      {
        if(code_seen(strCmd, axis_codes[i]))
        {
          add_homing[i] = code_value();
        }
      }
      break;
    #ifdef FWRETRACT
    case 207: //M207 - set retract length S[positive mm] F[feedrate mm/min] Z[additional zlift/hop]
    {
      if(code_seen(strCmd, 'S'))
      {
        retract_length = code_value() ;
      }
      if(code_seen(strCmd, 'F'))
      {
        retract_feedrate = code_value() ;
      }
      if(code_seen(strCmd, 'Z'))
      {
        retract_zlift = code_value() ;
      }
    }break;
    case 208: // M208 - set retract recover length S[positive mm surplus to the M207 S*] F[feedrate mm/min]
    {
      if(code_seen(strCmd, 'S'))
      {
        retract_recover_length = code_value() ;
      }
      if(code_seen(strCmd, 'F'))
      {
        retract_recover_feedrate = code_value() ;
      }
    }break;
    case 209: // M209 - S<1=true/0=false> enable automatic retract detect if the slicer did not support G10/11: every normal extrude-only move will be classified as retract depending on the direction.
    {
      if(code_seen(strCmd, 'S'))
      {
        int t= code_value() ;
        switch(t)
        {
          case 0: autoretract_enabled=false;retracted=false;break;
          case 1: autoretract_enabled=true;retracted=false;break;
          default:
            SERIAL_ECHO_START;
            SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
            SERIAL_ECHO(strCmd);
            SERIAL_ECHOLNPGM("\"");
        }
      }

    }break;
    #endif // FWRETRACT
    #if EXTRUDERS > 1
    case 218: // M218 - set hotend offset (in mm), T<extruder_number> X<offset_on_X> Y<offset_on_Y>
    {
      if(setTargetedHotend(strCmd, 218)){
        break;
      }
      if(code_seen(strCmd, 'X'))
      {
        extruder_offset[X_AXIS][tmp_extruder] = code_value();
      }
      if(code_seen(strCmd, 'Y'))
      {
        extruder_offset[Y_AXIS][tmp_extruder] = code_value();
      }
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_HOTEND_OFFSET);
      for(tmp_extruder = 0; tmp_extruder < EXTRUDERS; tmp_extruder++)
      {
         SERIAL_ECHOPGM(" ");
         SERIAL_ECHO(extruder_offset[X_AXIS][tmp_extruder]);
         SERIAL_ECHOPGM(",");
         SERIAL_ECHO(extruder_offset[Y_AXIS][tmp_extruder]);
      }
      SERIAL_EOL;
    }break;
    #endif
    case 220: // M220 S<factor in percent>- set speed factor override percentage
    {
      if(code_seen(strCmd, 'S'))
      {
        feedmultiply = code_value() ;
      }
    }
    break;
    case 221: // M221 S<factor in percent>- set extrude factor override percentage
    {
      if(code_seen(strCmd, 'S'))
      {
        extrudemultiply[active_extruder] = code_value();
      }
    }
    break;

    #if NUM_SERVOS > 0
    case 280: // M280 - set servo position absolute. P: servo index, S: angle or microseconds
      {
        int servo_index = -1;
        int servo_position = 0;
        if (code_seen(strCmd, 'P'))
          servo_index = code_value();
        if (code_seen(strCmd, 'S')) {
          servo_position = code_value();
          if ((servo_index >= 0) && (servo_index < NUM_SERVOS)) {
            servos[servo_index].write(servo_position);
          }
          else {
            SERIAL_ECHO_START;
            SERIAL_ECHOPGM("Servo ");
            SERIAL_ECHO(servo_index);
            SERIAL_ECHOLNPGM(" out of range");
          }
        }
        else if (servo_index >= 0) {
          SERIAL_PROTOCOLPGM(MSG_OK);
          SERIAL_PROTOCOLPGM(" Servo ");
          SERIAL_PROTOCOL(servo_index);
          SERIAL_PROTOCOLPGM(": ");
          SERIAL_PROTOCOL(servos[servo_index].read());
          SERIAL_EOL;
        }
      }
      break;
    #endif // NUM_SERVOS > 0

    #if LARGE_FLASH == true && ( BEEPER > 0 || defined(ULTRALCD) || defined(ENABLE_ULTILCD2) )
    case 300: // M300
    {
      unsigned int beepS = code_seen(strCmd, 'S') ? code_value() : 110;  //frequency Hz
      unsigned int beepP = code_seen(strCmd, 'P') ? code_value() : 1000; //duration ms
      if (beepS > 0)
      {
        #if BEEPER > 0
          // don't use tone libs that might mess with our timers
          uint32_t notch = 500000 / beepS;
          if(beepP > 4000) beepP = 4000; // prevent watchdog from tripping
          uint32_t loops = ((((uint32_t) beepP) * 500) / notch);
          for(uint32_t _i=0;_i<loops;_i++) { WRITE(BEEPER, HIGH); delayMicroseconds(notch); WRITE(BEEPER, LOW); delayMicroseconds(notch); }
        #elif defined(ULTRALCD)
          lcd_buzz(beepS, beepP);
        #endif
      }
      else
      {
        delay(beepP);
      }
    }
    break;
    #endif // M300

    #ifdef PIDTEMP
    case 301: // M301
      {
        if(code_seen(strCmd, 'P'))
        {
            Kp = code_value();
        #if EXTRUDERS > 1
            if (active_extruder) pid2[0] = Kp;
        #endif // EXTRUDERS
        }

        if(code_seen(strCmd, 'I'))
        {
            Ki = scalePID_i(code_value());
        #if EXTRUDERS > 1
            if (active_extruder) pid2[1] = Ki;
        #endif // EXTRUDERS
        }

        if(code_seen(strCmd, 'D'))
        {
            Kd = scalePID_d(code_value());
        #if EXTRUDERS > 1
            if (active_extruder) pid2[2] = Kd;
        #endif // EXTRUDERS
        }

        updatePID();
        SERIAL_PROTOCOLPGM(MSG_OK);
        SERIAL_PROTOCOLPGM(" p:");
        SERIAL_PROTOCOL(Kp);
        SERIAL_PROTOCOLPGM(" i:");
        SERIAL_PROTOCOL(unscalePID_i(Ki));
        SERIAL_PROTOCOLPGM(" d:");
        SERIAL_PROTOCOL(unscalePID_d(Kd));
        SERIAL_EOL;
      }
      break;
    #endif //PIDTEMP
    #if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
    case 304: // M304
      {
        if (pidTempBed())
        {
            if(code_seen(strCmd, 'P')) bedKp = code_value();
            if(code_seen(strCmd, 'I')) bedKi = scalePID_i(code_value());
            if(code_seen(strCmd, 'D')) bedKd = scalePID_d(code_value());

            updatePID();
            SERIAL_PROTOCOLPGM(MSG_OK);
            SERIAL_PROTOCOLPGM(" p:");
            SERIAL_PROTOCOL(bedKp);
            SERIAL_PROTOCOLPGM(" i:");
            SERIAL_PROTOCOL(unscalePID_i(bedKi));
            SERIAL_PROTOCOLPGM(" d:");
            SERIAL_PROTOCOL(unscalePID_d(bedKd));
            SERIAL_EOL;
        }
      }
      break;
    #endif //PIDTEMP
    case 240: // M240  Triggers a camera by emulating a Canon RC-1 : http://www.doc-diy.net/photo/rc-1_hacked/
     {
      #if defined(PHOTOGRAPH_PIN) && PHOTOGRAPH_PIN > -1
        const uint8_t NUM_PULSES=16;
        const float PULSE_LENGTH=0.01524;
        for(int i=0; i < NUM_PULSES; i++) {
          WRITE(PHOTOGRAPH_PIN, HIGH);
          _delay_ms(PULSE_LENGTH);
          WRITE(PHOTOGRAPH_PIN, LOW);
          _delay_ms(PULSE_LENGTH);
        }
        delay(7.33);
        for(int i=0; i < NUM_PULSES; i++) {
          WRITE(PHOTOGRAPH_PIN, HIGH);
          _delay_ms(PULSE_LENGTH);
          WRITE(PHOTOGRAPH_PIN, LOW);
          _delay_ms(PULSE_LENGTH);
        }
      #endif
     }
    break;
    #ifdef PREVENT_DANGEROUS_EXTRUDE
    case 302: // allow cold extrudes, or set the minimum extrude temperature
    {
	  float temp = .0;
	  if (code_seen(strCmd, 'S')) temp=code_value();
      set_extrude_min_temp(temp);
    }
    break;
	#endif
    case 303: // M303 PID autotune
    {
      float temp = 150.0;
      int e=0;
      int c=5;
      if (code_seen(strCmd, 'E')) e=code_value();
        if (e<0)
          temp=70;
      if (code_seen(strCmd, 'S')) temp=code_value();
      if (code_seen(strCmd, 'C')) c=code_value();
      PID_autotune(temp, e, c);
    }
    break;
    case 400: // M400 finish all moves
    {
      st_synchronize();
    }
    break;
    case 401:
      quickStop();
    break;
    case 500: // M500 Store settings in EEPROM
    {
        Config_StoreSettings();
    }
    break;
    case 501: // M501 Read settings from EEPROM
    {
        Config_RetrieveSettings();
    }
    break;
    case 502: // M502 Revert to default settings
    {
        Config_ResetDefault();
    }
    break;
    case 503: // M503 print settings currently in memory
    {
        Config_PrintSettings();
    }
    break;
    #ifdef ABORT_ON_ENDSTOP_HIT_FEATURE_ENABLED
    case 540:
    {
        if(code_seen(strCmd, 'S')) abort_on_endstop_hit = code_value() > 0;
    }
    break;
    #endif
    #ifdef FILAMENTCHANGEENABLE
    case 600: //Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
    {
        if (printing_state == PRINT_STATE_RECOVER)
          break;

        float target[NUM_AXIS];
        float lastpos[NUM_AXIS];
        target[X_AXIS]=current_position[X_AXIS];
        target[Y_AXIS]=current_position[Y_AXIS];
        target[Z_AXIS]=current_position[Z_AXIS];
        target[E_AXIS]=current_position[E_AXIS];
        lastpos[X_AXIS]=current_position[X_AXIS];
        lastpos[Y_AXIS]=current_position[Y_AXIS];
        lastpos[Z_AXIS]=current_position[Z_AXIS];
        lastpos[E_AXIS]=current_position[E_AXIS];
        //retract by E
        if(code_seen(strCmd, 'E'))
        {
          target[E_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FIRSTRETRACT
            target[E_AXIS]+= FILAMENTCHANGE_FIRSTRETRACT ;
          #endif
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        //lift Z
        if(code_seen(strCmd, 'Z'))
        {
          target[Z_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_ZADD
            target[Z_AXIS]+= FILAMENTCHANGE_ZADD ;
          #endif
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        //move xy
        if(code_seen(strCmd, 'X'))
        {
          target[X_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_XPOS
            target[X_AXIS]= FILAMENTCHANGE_XPOS ;
          #endif
        }
        if(code_seen(strCmd, 'Y'))
        {
          target[Y_AXIS]= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_YPOS
            target[Y_AXIS]= FILAMENTCHANGE_YPOS ;
          #endif
        }

        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        if(code_seen(strCmd, 'L'))
        {
          target[E_AXIS]+= code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FINALRETRACT
            target[E_AXIS]+= FILAMENTCHANGE_FINALRETRACT ;
          #endif
        }

        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder);

        //finish moves
        st_synchronize();
        //disable extruder steppers so filament can be removed
        disable_e0();
        disable_e1();
        disable_e2();
    #if EXTRUDERS > 1
        last_extruder = 0xFF;
    #endif
        delay(100);
        LCD_ALERTMESSAGEPGM(MSG_FILAMENTCHANGE);
        uint8_t cnt=0;
        while(!lcd_clicked())
        {
          cnt++;
          idle();
          if(cnt==0)
          {
          #if BEEPER > 0
            SET_OUTPUT(BEEPER);

            WRITE(BEEPER,HIGH);
            delay(3);
            WRITE(BEEPER,LOW);
            delay(3);
          #else
            lcd_buzz(1000/6,100);
          #endif
          }
        }

        //return to normal
        if(code_seen(strCmd, 'L'))
        {
          target[E_AXIS]+= -code_value();
        }
        else
        {
          #ifdef FILAMENTCHANGE_FINALRETRACT
            target[E_AXIS]+=(-1)*FILAMENTCHANGE_FINALRETRACT ;
          #endif
        }
        current_position[E_AXIS]=target[E_AXIS]; //the long retract of L is compensated by manual filament feeding
        plan_set_e_position(current_position[E_AXIS] / volume_to_filament_length[active_extruder], active_extruder, true);
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //should do nothing
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], target[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //move xy back
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], lastpos[Z_AXIS], target[E_AXIS], feedrate/60, active_extruder); //move z back
        plan_buffer_line(lastpos[X_AXIS], lastpos[Y_AXIS], lastpos[Z_AXIS], lastpos[E_AXIS], feedrate/60, active_extruder); //final untretract
    }
    break;
    #endif //FILAMENTCHANGEENABLE
    #ifdef ENABLE_ULTILCD2
    case 601: //M601 Pause in UltiLCD2, X[pos] Y[pos] Z[relative lift] L[later retract distance]
    {
        if (printing_state == PRINT_STATE_RECOVER)
          break;

        card.pauseSDPrint();
        serial_action_P(PSTR("paused"));

        st_synchronize();
        float target[NUM_AXIS];
        float lastpos[NUM_AXIS];
        // preserve current position
        memcpy(lastpos, current_position, sizeof(lastpos));
        memcpy(target, current_position, sizeof(target));
        recover_height = lastpos[Z_AXIS];

        //retract
        //Set the recover length to whatever distance we retracted so we recover properly.
        retract_recover_length = retract_length/volume_to_filament_length[active_extruder];
        target[E_AXIS] -= retract_recover_length;
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], retract_feedrate/60, active_extruder);
        retracted=true;

        //lift Z
        if(code_seen(strCmd, 'Z'))
        {
          target[Z_AXIS]+= code_value();
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], homing_feedrate[Z_AXIS]/60, active_extruder);

        //move xy
        if(code_seen(strCmd, 'X'))
        {
          target[X_AXIS] = code_value();
        }
        if(code_seen(strCmd, 'Y'))
        {
          target[Y_AXIS] = code_value();
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], homing_feedrate[X_AXIS]/60, active_extruder);

        // additional retract
        float addRetractLength = 0.0f;
        bool bAddRetract = code_seen(strCmd, 'L');
        if(bAddRetract)
        {
          addRetractLength = code_value()/volume_to_filament_length[active_extruder];
          retract_recover_length += addRetractLength;
          target[E_AXIS] -= addRetractLength;
        }
        plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], retract_feedrate/60, active_extruder);

        memcpy(current_position, target, sizeof(current_position));
        memcpy(destination, current_position, sizeof(destination));

        //finish moves
        st_synchronize();
        //disable extruder steppers so filament can be removed
        disable_e0();
        disable_e1();
        disable_e2();
    #if EXTRUDERS > 1
        last_extruder = 0xFF;
    #endif
        // serial_action_P(PSTR("pause"));
        card.pauseSDPrint();
        while(card.pause()){
          idle();
          if (printing_state == PRINT_STATE_ABORT)
          {
            break;
          }
        }

        plan_set_e_position(target[E_AXIS], active_extruder, true);

        if ((printing_state != PRINT_STATE_ABORT) && (card.sdprinting() || HAS_SERIAL_CMD))
        {
            //return to normal
            if(bAddRetract)
            {
                // revert the additional retract
                target[E_AXIS] += addRetractLength;
                plan_buffer_line(target[X_AXIS], target[Y_AXIS], target[Z_AXIS], target[E_AXIS], retract_feedrate/60, active_extruder); //Move back the L feed.
            }

            memcpy(current_position, lastpos, sizeof(current_position));
            memcpy(destination, current_position, sizeof(destination));

            //move xy back
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], target[Z_AXIS], target[E_AXIS], homing_feedrate[X_AXIS]/60, active_extruder);
            //move z back
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], target[E_AXIS], homing_feedrate[Z_AXIS]/60, active_extruder);

            //final unretract
            plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], retract_feedrate/60, active_extruder);
            retracted = false;
        }
        else
        {
          memcpy(current_position, target, sizeof(current_position));
          memcpy(destination, current_position, sizeof(destination));
        }
        serial_action_P(PSTR("resumed"));
    }
    break;

    case 605: // M605 store current set values
    {
      uint8_t tmp_select;
      if (code_seen(strCmd, 'S'))
      {
        tmp_select = code_value();
        if (tmp_select>9) tmp_select=9;
      }
      else
      {
        tmp_select = 0;
      }

      machinesettings.store(tmp_select);
    }
    break;

    case 606: // M606 recall saved values
    {
      uint8_t tmp_select;
      if (code_seen(strCmd, 'S'))
      {
        tmp_select = code_value();
        if (tmp_select>9) tmp_select=9;
      }
      else
      {
        tmp_select = 0;
      }
      machinesettings.recall(tmp_select);
    }
    break;
    #endif//ENABLE_ULTILCD2

    case 907: // M907 Set digital trimpot motor current using axis codes.
    {
      #if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
        for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) digipot_current(i,code_value());
        if(code_seen(strCmd, 'B')) digipot_current(4,code_value());
        if(code_seen(strCmd, 'S')) for(int i=0;i<=4;i++) digipot_current(i,code_value());
      #endif
      #if defined(MOTOR_CURRENT_PWM_XY_PIN) && MOTOR_CURRENT_PWM_XY_PIN > -1
        if(code_seen(strCmd, 'X')) digipot_current(0, code_value());
      #endif
      #if defined(MOTOR_CURRENT_PWM_Z_PIN) && MOTOR_CURRENT_PWM_Z_PIN > -1
        if(code_seen(strCmd, 'Z')) digipot_current(1, code_value());
      #endif
      #if defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
        if(code_seen(strCmd, 'E')) digipot_current(2, code_value());
      #endif
    }
    break;
    case 908: // M908 Control digital trimpot directly.
    {
      #if defined(DIGIPOTSS_PIN) && DIGIPOTSS_PIN > -1
        uint8_t channel,current;
        if(code_seen(strCmd, 'P')) channel=code_value();
        if(code_seen(strCmd, 'S')) current=code_value();
        digitalPotWrite(channel, current);
      #endif
    }
    break;
    case 350: // M350 Set microstepping mode. Warning: Steps per unit remains unchanged. S code sets stepping mode for all drivers.
    {
      #if defined(X_MS1_PIN) && X_MS1_PIN > -1
        if(code_seen(strCmd, 'S')) for(int i=0;i<=4;i++) microstep_mode(i,code_value());
        for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_mode(i,(uint8_t)code_value());
        if(code_seen(strCmd, 'B')) microstep_mode(4,code_value());
        microstep_readings();
      #endif
    }
    break;
    case 351: // M351 Toggle MS1 MS2 pins directly, S# determines MS1 or MS2, X# sets the pin high/low.
    {
      #if defined(X_MS1_PIN) && X_MS1_PIN > -1
      if(code_seen(strCmd, 'S')) switch((int)code_value())
      {
        case 1:
          for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_ms(i,code_value(),-1);
          if(code_seen(strCmd, 'B')) microstep_ms(4,code_value(),-1);
          break;
        case 2:
          for(int i=0;i<NUM_AXIS;i++) if(code_seen(axis_codes[i])) microstep_ms(i,-1,code_value());
          if(code_seen(strCmd, 'B')) microstep_ms(4,-1,code_value());
          break;
      }
      microstep_readings();
      #endif
    }
    break;
    case 999: // M999: Restart after being stopped
      if (printing_state == PRINT_STATE_RECOVER)
        break;
      Stopped = 0x0;
      lcd_reset_alert_level();
      gcode_LastN = Stopped_gcode_LastN;
      FlushSerialRequestResend();
    break;
#ifdef ENABLE_ULTILCD2
    case 10000://M10000 - Clear the whole LCD
        if (printing_state == PRINT_STATE_RECOVER)
          break;
        lcd_lib_clear();
        break;
    case 10001://M10001 - Draw text on LCD, M10002 X0 Y0 SText (when X is left out, it will draw centered)
        {
          if (printing_state == PRINT_STATE_RECOVER)
            break;
          uint8_t x = 0, y = 0;
          if (code_seen(strCmd, 'X'))
          {
            x = code_value_long();
            if (code_seen(strCmd, 'Y')) y = code_value_long();
            if (code_seen(strCmd, 'S')) lcd_lib_draw_string(x, y, strchr_pointer + 1);
          }
          else
          {
            if (code_seen(strCmd, 'Y')) y = code_value_long();
            if (code_seen(strCmd, 'S'))
            {
              truncate_checksum(++strchr_pointer);
              lcd_lib_draw_string_center(y, strchr_pointer);
            }
          }
        }
        break;
    case 10002://M10002 - Draw inverted text on LCD, M10002 X0 Y0 SText (when X is left out, it will draw centered)
        {
          if (printing_state == PRINT_STATE_RECOVER)
            break;
          uint8_t x = 0, y = 0;
          if (code_seen(strCmd, 'X'))
          {
            x = code_value_long();
            if (code_seen(strCmd, 'Y')) y = code_value_long();
            if (code_seen(strCmd, 'S')) lcd_lib_clear_string(x, y, strchr_pointer + 1);
          }
          else
          {
            if (code_seen(strCmd, 'Y')) y = code_value_long();
            if (code_seen(strCmd, 'S'))
            {
              truncate_checksum(++strchr_pointer);
              lcd_lib_clear_string_center(y, strchr_pointer);
            }
          }
        }
        break;
    case 10003://M10003 - Draw square on LCD, M10003 X1 Y1 W10 H10
        {
        if (printing_state == PRINT_STATE_RECOVER)
          break;
        uint8_t x = 0, y = 0, w = 1, h = 1;
        if (code_seen(strCmd, 'X')) x = code_value_long();
        if (code_seen(strCmd, 'Y')) y = code_value_long();
        if (code_seen(strCmd, 'W')) w = code_value_long();
        if (code_seen(strCmd, 'H')) h = code_value_long();
        lcd_lib_set(x, y, x + w, y + h);
        }
        break;
    case 10004://M10004 - Draw filled rectangle on LCD, M10004 X1 Y1 W10 H10
        {
         uint8_t x = 0, y = 0, w = 1, h = 1;
         if (code_seen(strCmd, 'X')) x = code_value_long();
         if (code_seen(strCmd, 'Y')) y = code_value_long();
         if (code_seen(strCmd, 'W')) w = code_value_long();
         if (code_seen(strCmd, 'H')) h = code_value_long();
         lcd_lib_set(x, y, x + w, y + h);
        }
        break;
    case 10005://M10005 - Draw shaded square on LCD, M10004 X1 Y1 W10 H10
        {
        if (printing_state == PRINT_STATE_RECOVER)
          break;
        uint8_t x = 0, y = 0, w = 1, h = 1;
        if (code_seen(strCmd, 'X')) x = code_value_long();
        if (code_seen(strCmd, 'Y')) y = code_value_long();
        if (code_seen(strCmd, 'W')) w = code_value_long();
        if (code_seen(strCmd, 'H')) h = code_value_long();
        lcd_lib_draw_shade(x, y, x + w, y + h);
        }
        break;
    case 10010://M10010 - Request LCD screen button info (R:[rotation difference compared to previous request] B:[button down])
        {
            SERIAL_PROTOCOLPGM("ok R:");
            SERIAL_PROTOCOL(lcd_lib_encoder_pos);
            lcd_lib_encoder_pos = 0;
            if (lcd_lib_button_down)
                SERIAL_PROTOCOLLNPGM(" B:1");
            else
                SERIAL_PROTOCOLLNPGM(" B:0");
            return;
        }
        break;
#endif//ENABLE_ULTILCD2
    }
  }

  else if(code_seen(strCmd, 'T'))
  {
    tmp_extruder = code_value();
    if(tmp_extruder >= EXTRUDERS)
    {
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM("T");
      SERIAL_ECHO(tmp_extruder);
      SERIAL_ECHOLNPGM(MSG_INVALID_EXTRUDER);
    }
    else
    {
#if EXTRUDERS > 1
      boolean make_move = false;
      if (swapExtruders() && (tmp_extruder < 2))
      {
        tmp_extruder ^= 0x01;
      }
      #endif
      if(code_seen(strCmd, 'F')) {
#if EXTRUDERS > 1
        make_move = true;
#endif
        float next_feedrate = code_value();
        if(next_feedrate > 0.0) {
          feedrate = next_feedrate;
        }
      }
      #if EXTRUDERS > 1
      if(tmp_extruder != active_extruder) {
        // finish moves
        st_synchronize();

        // Save current position to return to after applying extruder offset
        memcpy(destination, current_position, sizeof(destination));
        // Offset extruder (only by XY)
        int i;
        for(i = 0; i < 2; i++) {
           current_position[i] = current_position[i] -
                                 extruder_offset[i][active_extruder] +
                                 extruder_offset[i][tmp_extruder];
        }
        // Set the new active extruder and position
        active_extruder = tmp_extruder;
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS], active_extruder, true);

        // Move to the old position if 'F' was in the parameters
        if((printing_state < PRINT_STATE_ABORT) && make_move && !Stopped && (IS_SD_PRINTING || commands_queued()))
        {
           prepare_move(strCmd);
        }
      }
      #endif
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_ACTIVE_EXTRUDER);
      SERIAL_PROTOCOLLN((int)active_extruder);
    }
  }
  else if (strcmp_P(strCmd, PSTR("Electronics_test")) == 0)
  {
    run_electronics_test();
  }
  else
  {
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
    SERIAL_ECHO(strCmd);
    SERIAL_ECHOLNPGM("\"");
  }

  if ((printing_state != PRINT_STATE_RECOVER) && (printing_state != PRINT_STATE_START) && (printing_state != PRINT_STATE_ABORT))
    printing_state = PRINT_STATE_NORMAL;

  // send acknowledge for serial commands
  if (sendAck)
  {
      ClearToSend();
  }
}

void process_command_P(const char *strCmd)
{
    char cmd[MAX_CMD_SIZE] = {0};
    strcpy_P(cmd, strCmd);
    process_command(cmd, false);
}

static void FlushSerialRequestResend()
{
  MYSERIAL.flush();
  SERIAL_PROTOCOLPGM(MSG_RESEND);
  SERIAL_PROTOCOLLN(gcode_LastN + 1);
  ClearToSend();
}

static void ClearToSend()
{
  previous_millis_cmd = millis();
  SERIAL_PROTOCOLLNPGM(MSG_OK);
}

static void get_coordinates(const char *cmd)
{
  #ifdef FWRETRACT
    uint8_t seen=0;
  #endif
    for(uint8_t i=0; i<NUM_AXIS; ++i)
    {
        if(code_seen(cmd, axis_codes[i]))
        {
            destination[i] = (float)code_value() + ((axis_relative_state & (1 << i)) || (axis_relative_state & RELATIVE_MODE))*current_position[i];
          #ifdef FWRETRACT
            seen |= (1 << i);
          #endif
        }
        else
        {
            destination[i] = current_position[i]; //Are these else lines really needed?
        }
    }
    if(code_seen(cmd, 'F'))
    {
        float next_feedrate = code_value();
        if(next_feedrate > 0.0)
        {
            feedrate = next_feedrate;
        }
    }
    #ifdef FWRETRACT
    if(autoretract_enabled)
    {
        if (seen == (1 << E_AXIS))
        {
            // e only move
            float echange=destination[E_AXIS]-current_position[E_AXIS];
            if(echange<-MIN_RETRACT) //retract
            {
                if(!retracted)
                {
                    destination[Z_AXIS]+=retract_zlift; //not sure why changing current_position negatively does not work.
                    //if slicer retracted by echange=-1mm and you want to retract 3mm, corrrectede=-2mm additionally
                    float correctede=-echange-retract_length;
                    //to generate the additional steps, not the destination is changed, but inversely the current position
                    current_position[E_AXIS]+=-correctede;
                    feedrate=retract_feedrate;
                    retracted=true;
                }
            }
            else if(echange>MIN_RETRACT) //retract_recover
            {
                if(retracted)
                {
                    //current_position[Z_AXIS]+=-retract_zlift;
                    //if slicer retracted_recovered by echange=+1mm and you want to retract_recover 3mm, corrrectede=2mm additionally
                    float correctede=-echange+1*retract_length+retract_recover_length; //total unretract=retract_length+retract_recover_length[surplus]
                    current_position[E_AXIS]+=correctede; //to generate the additional steps, not the destination is changed, but inversely the current position
                    feedrate=retract_recover_feedrate;
                    retracted=false;
                }
            }
        }
    }
    #endif //FWRETRACT
}

static void get_arc_coordinates(const char *cmd)
{
#ifdef SF_ARC_FIX
   uint8_t relative_state_backup = axis_relative_state;
   axis_relative_state |= RELATIVE_MODE;
#endif
   get_coordinates(cmd);
#ifdef SF_ARC_FIX
   axis_relative_state=relative_state_backup;
#endif

   if(code_seen(cmd, 'I')) {
     offset[0] = code_value();
   }
   else {
     offset[0] = 0.0;
   }
   if(code_seen(cmd, 'J')) {
     offset[1] = code_value();
   }
   else {
     offset[1] = 0.0;
   }
}

void clamp_to_software_endstops(float target[3])
{
  if (min_software_endstops) {
    if (target[X_AXIS] < min_pos[X_AXIS]) { target[X_AXIS] = min_pos[X_AXIS]; position_error = true; }
    if (target[Y_AXIS] < min_pos[Y_AXIS]) { target[Y_AXIS] = min_pos[Y_AXIS]; position_error = true; }
    if (target[Z_AXIS] < min_pos[Z_AXIS]) { target[Z_AXIS] = min_pos[Z_AXIS]; position_error = true; }
  }

  if (max_software_endstops) {
    if (target[X_AXIS] > max_pos[X_AXIS]) { target[X_AXIS] = max_pos[X_AXIS]; position_error = true; }
    if (target[Y_AXIS] > max_pos[Y_AXIS]) { target[Y_AXIS] = max_pos[Y_AXIS]; position_error = true; }
    if (target[Z_AXIS] > max_pos[Z_AXIS]) { target[Z_AXIS] = max_pos[Z_AXIS]; position_error = true; }
  }
}

#ifdef DELTA
void calculate_delta(float cartesian[3])
{
  delta[X_AXIS] = sqrt(sq(DELTA_DIAGONAL_ROD)
                       - sq(DELTA_TOWER1_X-cartesian[X_AXIS])
                       - sq(DELTA_TOWER1_Y-cartesian[Y_AXIS])
                       ) + cartesian[Z_AXIS];
  delta[Y_AXIS] = sqrt(sq(DELTA_DIAGONAL_ROD)
                       - sq(DELTA_TOWER2_X-cartesian[X_AXIS])
                       - sq(DELTA_TOWER2_Y-cartesian[Y_AXIS])
                       ) + cartesian[Z_AXIS];
  delta[Z_AXIS] = sqrt(sq(DELTA_DIAGONAL_ROD)
                       - sq(DELTA_TOWER3_X-cartesian[X_AXIS])
                       - sq(DELTA_TOWER3_Y-cartesian[Y_AXIS])
                       ) + cartesian[Z_AXIS];
  /*
  SERIAL_ECHOPGM("cartesian x="); SERIAL_ECHO(cartesian[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHO(cartesian[Y_AXIS]);
  SERIAL_ECHOPGM(" z="); SERIAL_ECHOLN(cartesian[Z_AXIS]);

  SERIAL_ECHOPGM("delta x="); SERIAL_ECHO(delta[X_AXIS]);
  SERIAL_ECHOPGM(" y="); SERIAL_ECHO(delta[Y_AXIS]);
  SERIAL_ECHOPGM(" z="); SERIAL_ECHOLN(delta[Z_AXIS]);
  */
}
#endif

static void prepare_move(const char *cmd)
{
  clamp_to_software_endstops(destination);

  previous_millis_cmd = millis();
#ifdef DELTA
  float difference[NUM_AXIS];
  for (int8_t i=0; i < NUM_AXIS; i++) {
    difference[i] = destination[i] - current_position[i];
  }
  float cartesian_mm = sqrt(sq(difference[X_AXIS]) +
                            sq(difference[Y_AXIS]) +
                            sq(difference[Z_AXIS]));
  if (cartesian_mm < 0.000001) { cartesian_mm = abs(difference[E_AXIS]); }
  if (cartesian_mm < 0.000001) { return; }
  float seconds = 6000 * cartesian_mm / feedrate / feedmultiply;
  int steps = max(1, int(DELTA_SEGMENTS_PER_SECOND * seconds));
  // SERIAL_ECHOPGM("mm="); SERIAL_ECHO(cartesian_mm);
  // SERIAL_ECHOPGM(" seconds="); SERIAL_ECHO(seconds);
  // SERIAL_ECHOPGM(" steps="); SERIAL_ECHOLN(steps);
  for (int s = 1; s <= steps; s++) {
    float fraction = float(s) / float(steps);
    for(int8_t i=0; i < NUM_AXIS; i++) {
      destination[i] = current_position[i] + difference[i] * fraction;
    }
    calculate_delta(destination);
    if (card.sdprinting && (printing_state == PRINT_STATE_RECOVER) && (destination[Z_AXIS] >= recover_height-0.01f))
    {
      recover_start_print(cmd);
    }
    else if (printing_state != PRINT_STATE_RECOVER)
    {
      plan_buffer_line(delta[X_AXIS], delta[Y_AXIS], delta[Z_AXIS],
                       destination[E_AXIS], feedrate*feedmultiply/60/100.0,
                       active_extruder);
    }
  }
#else
  if (card.sdprinting() && (printing_state == PRINT_STATE_RECOVER) && (destination[Z_AXIS] >= recover_height-0.01f))
  {
    if (current_position[E_AXIS] != destination[E_AXIS])
    {
      for(uint8_t i=0; i < NUM_AXIS; ++i) {
          recover_position[i] = current_position[i];
      }
      recover_start_print(cmd);
    }
  }
  else if (printing_state != PRINT_STATE_RECOVER)
  {
    // Do not use feedmultiply for E or Z only moves
    if( (current_position[X_AXIS] == destination [X_AXIS]) && (current_position[Y_AXIS] == destination [Y_AXIS])) {
      plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    }
    else {
      plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate*feedmultiply/60/100.0, active_extruder);
    }
  }
#endif
  memcpy(current_position, destination, sizeof(current_position));
}

static void prepare_arc_move(char isclockwise)
{
  float r = hypot(offset[X_AXIS], offset[Y_AXIS]); // Compute arc radius for mc_arc

  // Trace the arc
  mc_arc(current_position, destination, offset, X_AXIS, Y_AXIS, Z_AXIS, feedrate*feedmultiply/60/100.0, r, isclockwise, active_extruder);

  // As far as the parser is concerned, the position is now == target. In reality the
  // motion control system might still be processing the action and the real tool position
  // in any intermediate location.
  for(int8_t i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
  previous_millis_cmd = millis();
}

#if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1

#if defined(FAN_PIN)
  #if CONTROLLERFAN_PIN == FAN_PIN
    #error "You cannot set CONTROLLERFAN_PIN equal to FAN_PIN"
  #endif
#endif

unsigned long lastMotor = 0; //Save the time for when a motor was turned on last
unsigned long lastMotorCheck = 0;

void controllerFan()
{
  if ((millis() - lastMotorCheck) >= 2500) //Not a time critical function, so we only check every 2500ms
  {
    lastMotorCheck = millis();

    if(!READ(X_ENABLE_PIN) || !READ(Y_ENABLE_PIN) || !READ(Z_ENABLE_PIN)
    #if EXTRUDERS > 2
       || !READ(E2_ENABLE_PIN)
    #endif
    #if EXTRUDER > 1
       || !READ(E1_ENABLE_PIN)
    #endif
       || !READ(E0_ENABLE_PIN)) //If any of the drivers are enabled...
    {
      lastMotor = millis(); //... set time to NOW so the fan will turn on
    }

    if ((millis() - lastMotor) >= (CONTROLLERFAN_SECS*1000UL) || lastMotor == 0) //If the last time any driver was enabled, is longer since than CONTROLLERSEC...
    {
        analogWrite(CONTROLLERFAN_PIN, 0);
    }
    else
    {
        // allows digital or PWM fan output to be used (see M42 handling)
        analogWrite(CONTROLLERFAN_PIN, CONTROLLERFAN_SPEED);
    }
  }
}
#endif

/**
 * Standard idle routine keeps the machine alive
 */
void idle()
{
    static unsigned long lastSerialCommandTime = 0;

    manage_heater();
    manage_inactivity();

    lcd_update();
    lifetime_stats_tick();

    // detect serial communication
    if (commands_queued() && serialCmd)
    {
        sleep_state |= SLEEP_SERIAL_CMD;
        lastSerialCommandTime = millis();
    }
    else if ((lastSerialCommandTime>0) && ((millis() - lastSerialCommandTime) < SERIAL_CONTROL_TIMEOUT))
    {
        sleep_state |= SLEEP_SERIAL_CMD;
    }
    else if (HAS_SERIAL_CMD)
    {
        // reset printing state
        sleep_state &= ~SLEEP_SERIAL_CMD;
        printing_state = PRINT_STATE_NORMAL;
    }
}

static void manage_inactivity()
{
  checkFilamentSensor();
  manage_led_timeout();

  unsigned long m=millis();

  if(printing_state == PRINT_STATE_RECOVER)
    previous_millis_cmd=m;

  if( max_inactive_time && ((m - previous_millis_cmd) >  max_inactive_time) )
      kill();
#if DISABLE_X || DISABLE_Y || DISABLE_Z || DISABLE_E
  if(stepper_inactive_time)  {
    if( (m - previous_millis_cmd) >  stepper_inactive_time )
    {
      if(blocks_queued() == false) {
        if(DISABLE_X) disable_x();
        if(DISABLE_Y) disable_y();
        if(DISABLE_Z) disable_z();
        if(DISABLE_E) {
            disable_e0();
            disable_e1();
            disable_e2();
        #if EXTRUDERS > 1
            last_extruder = 0xFF;
        #endif
        }
      }
    }
  }
#endif
  #if defined(KILL_PIN) && KILL_PIN > -1
    if( 0 == READ(KILL_PIN) )
      kill();
  #endif
  #if defined(SAFETY_TRIGGERED_PIN) && SAFETY_TRIGGERED_PIN > -1
  if (READ(SAFETY_TRIGGERED_PIN))
    Stop(STOP_REASON_SAFETY_TRIGGER);
  #endif
  #if defined(CONTROLLERFAN_PIN) && CONTROLLERFAN_PIN > -1
    controllerFan(); //Check if fan should be turned on to cool stepper drivers down
  #endif
  #ifdef EXTRUDER_RUNOUT_PREVENT
    if( (m - previous_millis_cmd) >  EXTRUDER_RUNOUT_SECONDS*1000 )
    if(degHotend(active_extruder)>EXTRUDER_RUNOUT_MINTEMP)
    {
     bool oldstatus=READ(E0_ENABLE_PIN);
     enable_e0();
     float oldepos=current_position[E_AXIS];
     float oldedes=destination[E_AXIS];
     plan_buffer_line(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS],
                      current_position[E_AXIS]+EXTRUDER_RUNOUT_EXTRUDE*EXTRUDER_RUNOUT_ESTEPS/axis_steps_per_unit[E_AXIS],
                      EXTRUDER_RUNOUT_SPEED/60.*EXTRUDER_RUNOUT_ESTEPS/e_steps_per_unit(active_extruder), active_extruder);
     current_position[E_AXIS]=oldepos;
     destination[E_AXIS]=oldedes;
     plan_set_e_position(oldepos, active_extruder, true);
     previous_millis_cmd=millis();
     WRITE(E0_ENABLE_PIN,oldstatus);
    }
  #endif
  check_axes_activity();
}

void kill()
{
  cli(); // Stop interrupts
  disable_all_heaters();

  disable_x();
  disable_y();
  disable_z();
  disable_e0();
  disable_e1();
  disable_e2();
#if EXTRUDERS > 1
  last_extruder = 0xFF;
#endif

#if defined(PS_ON_PIN) && PS_ON_PIN > -1
  pinMode(PS_ON_PIN,INPUT);
#endif
  SERIAL_ERROR_START;
  SERIAL_ERRORLNPGM(MSG_ERR_KILLED);
  LCD_ALERTMESSAGEPGM(MSG_KILLED);
  suicide();
  while(1) { /* Intentionally left empty */ } // Wait for reset
}

void Stop(uint8_t reasonNr)
{
  disable_all_heaters();
  if(!Stopped) {
    Stopped = reasonNr;
    Stopped_gcode_LastN = gcode_LastN; // Save last g_code for restart
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
    LCD_MESSAGEPGM(MSG_STOPPED);
  }
}

bool IsStopped() { return Stopped; };
uint8_t StoppedReason() { return Stopped; };

#ifdef FAST_PWM_FAN
void setPwmFrequency(uint8_t pin, int val)
{
  val &= 0x07;
  switch(digitalPinToTimer(pin))
  {

    #if defined(TCCR0A)
    case TIMER0A:
    case TIMER0B:
//         TCCR0B &= ~(_BV(CS00) | _BV(CS01) | _BV(CS02));
//         TCCR0B |= val;
         break;
    #endif

    #if defined(TCCR1A)
    case TIMER1A:
    case TIMER1B:
//         TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
//         TCCR1B |= val;
         break;
    #endif

    #if defined(TCCR2)
    case TIMER2:
    case TIMER2:
         TCCR2 &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
         TCCR2 |= val;
         break;
    #endif

    #if defined(TCCR2A)
    case TIMER2A:
    case TIMER2B:
         TCCR2B &= ~(_BV(CS20) | _BV(CS21) | _BV(CS22));
         TCCR2B |= val;
         break;
    #endif

    #if defined(TCCR3A)
    case TIMER3A:
    case TIMER3B:
    case TIMER3C:
         TCCR3B &= ~(_BV(CS30) | _BV(CS31) | _BV(CS32));
         TCCR3B |= val;
         break;
    #endif

    #if defined(TCCR4A)
    case TIMER4A:
    case TIMER4B:
    case TIMER4C:
         TCCR4B &= ~(_BV(CS40) | _BV(CS41) | _BV(CS42));
         TCCR4B |= val;
         break;
   #endif

    #if defined(TCCR5A)
    case TIMER5A:
    case TIMER5B:
    case TIMER5C:
         TCCR5B &= ~(_BV(CS50) | _BV(CS51) | _BV(CS52));
         TCCR5B |= val;
         break;
   #endif

  }
}
#endif //FAST_PWM_FAN

static bool setTargetedHotend(const char *cmd, int code)
{
  if(code_seen(cmd, 'T'))
  {
    tmp_extruder = code_value();
    #if EXTRUDERS > 1
    if (swapExtruders() && (tmp_extruder < 2))
    {
      tmp_extruder ^= 0x01;
    }
    #endif // EXTRUDERS
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      switch(code){
        case 104:
          SERIAL_ECHOPGM(MSG_M104_INVALID_EXTRUDER);
          break;
        case 105:
          SERIAL_ECHOPGM(MSG_M105_INVALID_EXTRUDER);
          break;
        case 109:
          SERIAL_ECHOPGM(MSG_M109_INVALID_EXTRUDER);
          break;
        case 218:
          SERIAL_ECHOPGM(MSG_M218_INVALID_EXTRUDER);
          break;
      }
      SERIAL_ECHOLN(tmp_extruder);
      return true;
    }
  }
  else
  {
    tmp_extruder = active_extruder;
  }
  return false;
}
