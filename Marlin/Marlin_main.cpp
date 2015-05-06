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
 */

#include "Marlin.h"

#include "planner.h"
#include "stepper.h"
#include "temperature.h"
#include "motion_control.h"
#include "watchdog.h"
#include "electronics_test.h"
#include "language.h"
#include "pins_arduino.h"
#include "i2c_driver.h"
#include "i2c_capacitance.h"
#include "led_rgbw_pca9632.h"
#include "fan_driver.h"

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
// G1000 - Store the current position/feedrate. (Used for pause&resume handling)
// G1001 - Return to stored position from G1000, with the given feedrate, and restore the original feedrate after that.

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
// M218 - set hotend offset (in mm): T<extruder_number> X<offset_on_X> Y<offset_on_Y> Z<offset_on_Z>
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
// M401 - quickstop - Abort all the planned moves. This will stop the head mid-move, so most likely the head will be out of sync with the stepper position after this.
// M600 - Pause for filament change X[pos] Y[pos] Z[relative lift] E[initial retract] L[later retract distance for removal]
// M907 - Set digital trimpot motor current using axis codes.
// M999 - Restart after being stopped by error

//Stepper Movement Variables

//===========================================================================
//=============================imported variables============================
//===========================================================================


//===========================================================================
//=============================public variables=============================
//===========================================================================
float homing_feedrate[] = HOMING_FEEDRATE;
bool axis_relative_modes[] = AXIS_RELATIVE_MODES;
int feedmultiply=100; //100->1 200->2
int saved_feedmultiply;
int extrudemultiply[EXTRUDERS]=ARRAY_BY_EXTRUDERS(100, 100, 100); //100->1 200->2
float current_position[NUM_AXIS] = { 0.0, 0.0, 0.0, 0.0 };
float add_homeing[3]={0,0,-30};
float min_pos[3] = { X_MIN_POS, Y_MIN_POS, Z_MIN_POS };
float max_pos[3] = { X_MAX_POS, Y_MAX_POS, Z_MAX_POS };
// Extruder offset, only in XY plane
#if EXTRUDERS > 1
float extruder_offset[3][EXTRUDERS] = {
#if defined(EXTRUDER_OFFSET_X) && defined(EXTRUDER_OFFSET_Y)
  EXTRUDER_OFFSET_X, EXTRUDER_OFFSET_Y, 0
#endif
};
#endif
uint8_t active_extruder = 0;
uint8_t fanSpeed=0;
uint8_t fanSpeedPercent=100;

#ifdef FWRETRACT
  bool retracted=false;
  float retract_length=4.5, retract_feedrate=25*60, retract_zlift=0.8;
#if EXTRUDERS > 1
  float extruder_swap_retract_length=16.0;
#endif
  float retract_recover_length=0, retract_recover_feedrate=25*60;
#endif

//===========================================================================
//=============================private variables=============================
//===========================================================================
const char axis_codes[NUM_AXIS] = {'X', 'Y', 'Z', 'E'};
static float destination[NUM_AXIS] = {  0.0, 0.0, 0.0, 0.0};
static float offset[3] = {0.0, 0.0, 0.0};
static bool home_all_axis = true;
static float feedrate = 1500.0, next_feedrate, saved_feedrate;
static long gcode_N[BUFSIZE], gcode_LastN, Stopped_gcode_LastN = 0;

static bool relative_mode = false;  //Determines Absolute or Relative Coordinates

static char command_buffer[BUFSIZE][MAX_CMD_SIZE];
static int command_buffer_index_read = 0;
static int command_buffer_index_write = 0;
static int command_buffer_length = 0;
//static int i = 0;
static char serial_char;
static int serial_count = 0;
static boolean comment_mode = false;
static char *strchr_pointer; // just a pointer to find chars in the cmd string like X, Y, Z, E, etc

const int sensitive_pins[] = SENSITIVE_PINS; // Sensitive pin list for M42

//static float tt = 0;
//static float bt = 0;

//Inactivity shutdown variables
static unsigned long previous_millis_cmd = 0;
static unsigned long max_inactive_time = 0;
static unsigned long stepper_inactive_time = DEFAULT_STEPPER_DEACTIVE_TIME*1000l;

unsigned long starttime=0;
unsigned long stoptime=0;

static uint8_t tmp_extruder;


uint8_t Stopped = false;

static float stored_position[NUM_AXIS];
static float stored_feedrate;

//===========================================================================
//=============================ROUTINES=============================
//===========================================================================

void get_arc_coordinates();
bool setTargetedHotend(int code);

void serial_echopair_P(const char *s_P, float v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, double v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }
void serial_echopair_P(const char *s_P, unsigned long v)
    { serialprintPGM(s_P); SERIAL_ECHO(v); }

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

//Clear all the commands in the ASCII command buffer, to make sure we have room for abort commands.
void clear_command_queue()
{
    if (command_buffer_length > 0)
    {
        command_buffer_index_write = (command_buffer_index_read + 1)%BUFSIZE;
        command_buffer_length = 1;
    }
}

//adds an command to the main command buffer
//thats really done in a non-safe way.
//needs overworking someday
void enquecommand(const char *cmd)
{
  if(command_buffer_length < BUFSIZE)
  {
    //this is dangerous if a mixing of serial and this happsens
    strcpy(&(command_buffer[command_buffer_index_write][0]),cmd);
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(command_buffer[command_buffer_index_write]);
    SERIAL_ECHOLNPGM("\"");
    command_buffer_index_write= (command_buffer_index_write + 1)%BUFSIZE;
    command_buffer_length += 1;
  }
}

void enquecommand_P(const char *cmd)
{
  if(command_buffer_length < BUFSIZE)
  {
    //this is dangerous if a mixing of serial and this happsens
    strcpy_P(&(command_buffer[command_buffer_index_write][0]),cmd);
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM("enqueing \"");
    SERIAL_ECHO(command_buffer[command_buffer_index_write]);
    SERIAL_ECHOLNPGM("\"");
    command_buffer_index_write= (command_buffer_index_write + 1)%BUFSIZE;
    command_buffer_length += 1;
  }
}

bool is_command_queued()
{
    return command_buffer_length > 0;
}

uint8_t commands_queued()
{
    return command_buffer_length;
}

void setup_killpin()
{
  #if defined(KILL_PIN) && KILL_PIN > -1
    SET_INPUT(KILL_PIN);
    WRITE(KILL_PIN,HIGH);
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

  //Read ADC14, this is connected to the main power and helps in detecting which board we have
  // Connected to 24V - 100k -|- 4k7 - GND = ADC ~221 on Ultimaker 2.0 board with 8 microsteps on the Z
  // Connected to 24V - 100k -|- 10k - GND = ADC ~447 ADC on Ultimaker 2.x board with 16 microsteps on the Z
  int main_board_power = analogRead(14);

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

  watchdog_init();
  if (main_board_power > 300)//HACKERDYHACK, set the steps per unit for Z to 400 for the 16 microstep board.
    axis_steps_per_unit[Z_AXIS] = 400;
  i2cDriverInit();
  initFans();   // Initialize the fan driver
  tp_init();    // Initialize temperature loop
  plan_init();  // Initialize planner;
  st_init();    // Initialize stepper, this enables interrupts!
#ifdef ENABLE_BED_LEVELING_PROBE
  i2cCapacitanceInit();
#endif
  ledRGBWInit();
  SERIAL_ECHO_START;
  SERIAL_ERRORLNPGM("setup done");
}


void loop()
{
  if(command_buffer_length < (BUFSIZE-1))
    get_command();
  if(command_buffer_length)
  {
    process_commands();
    if (command_buffer_length > 0)
    {
      command_buffer_length = (command_buffer_length-1);
      command_buffer_index_read = (command_buffer_index_read + 1)%BUFSIZE;
    }
  }
  //check heater every n milliseconds
  manage_heater();
  manage_inactivity();
  checkHitEndstops();
}

void get_command()
{
  while( MYSERIAL.available() > 0  && command_buffer_length < BUFSIZE) {
    serial_char = MYSERIAL.read();
    if(serial_char == '\n' ||
       serial_char == '\r' ||
       (serial_char == ':' && comment_mode == false) ||
       serial_count >= (MAX_CMD_SIZE - 1) )
    {
      if(!serial_count) { //if empty line
        comment_mode = false; //for new command
        return;
      }
      command_buffer[command_buffer_index_write][serial_count] = 0; //terminate string
      if(!comment_mode){
        comment_mode = false; //for new command
        gcode_N[command_buffer_index_write] = -1;
        if(strchr(command_buffer[command_buffer_index_write], 'N') != NULL)
        {
          strchr_pointer = strchr(command_buffer[command_buffer_index_write], 'N');
          gcode_N[command_buffer_index_write] = (strtol(&command_buffer[command_buffer_index_write][strchr_pointer - command_buffer[command_buffer_index_write] + 1], NULL, 10));
          if(gcode_N[command_buffer_index_write] != gcode_LastN+1 && (strstr_P(command_buffer[command_buffer_index_write], PSTR("M110")) == NULL) ) {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_LINE_NO);
            SERIAL_ERRORLN(gcode_LastN);
            //Serial.println(gcode_N);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          if(strchr(command_buffer[command_buffer_index_write], '*') != NULL)
          {
            byte checksum = 0;
            byte count = 0;
            while(command_buffer[command_buffer_index_write][count] != '*') checksum = checksum^command_buffer[command_buffer_index_write][count++];
            strchr_pointer = strchr(command_buffer[command_buffer_index_write], '*');

            if( (int)(strtod(&command_buffer[command_buffer_index_write][strchr_pointer - command_buffer[command_buffer_index_write] + 1], NULL)) != checksum) {
              SERIAL_ERROR_START;
              SERIAL_ERRORPGM(MSG_ERR_CHECKSUM_MISMATCH);
              SERIAL_ERRORLN(gcode_LastN);
              FlushSerialRequestResend();
              serial_count = 0;
              return;
            }
            //if no errors, continue parsing
          }
          else
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            FlushSerialRequestResend();
            serial_count = 0;
            return;
          }

          gcode_LastN = gcode_N[command_buffer_index_write];
          //if no errors, continue parsing
        }
        else  // if we don't receive 'N' but still see '*'
        {
          if((strchr(command_buffer[command_buffer_index_write], '*') != NULL))
          {
            SERIAL_ERROR_START;
            SERIAL_ERRORPGM(MSG_ERR_NO_LINENUMBER_WITH_CHECKSUM);
            SERIAL_ERRORLN(gcode_LastN);
            serial_count = 0;
            return;
          }
        }
        if((strchr(command_buffer[command_buffer_index_write], 'G') != NULL)){
          strchr_pointer = strchr(command_buffer[command_buffer_index_write], 'G');
          switch((int)((strtod(&command_buffer[command_buffer_index_write][strchr_pointer - command_buffer[command_buffer_index_write] + 1], NULL)))){
          case 0:
          case 1:
          case 2:
          case 3:
            if(Stopped) { // If printer is stopped by an error the G[0-3] codes are ignored.
              SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
            }
            break;
          default:
            break;
          }

        }
        command_buffer_index_write = (command_buffer_index_write + 1)%BUFSIZE;
        command_buffer_length += 1;
      }
      serial_count = 0; //clear buffer
    }
    else
    {
      if(serial_char == ';') comment_mode = true;
      if(!comment_mode) command_buffer[command_buffer_index_write][serial_count++] = serial_char;
    }
  }
}


float code_value()
{
  return (strtod(&command_buffer[command_buffer_index_read][strchr_pointer - command_buffer[command_buffer_index_read] + 1], NULL));
}

long code_value_long()
{
  return (strtol(&command_buffer[command_buffer_index_read][strchr_pointer - command_buffer[command_buffer_index_read] + 1], NULL, 10));
}

bool code_seen(char code)
{
  strchr_pointer = strchr(command_buffer[command_buffer_index_read], code);
  return (strchr_pointer != NULL);  //Return True if a character was found
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

XYZ_CONSTS_FROM_CONFIG(float, base_min_pos,    MIN_POS);
XYZ_CONSTS_FROM_CONFIG(float, base_max_pos,    MAX_POS);
XYZ_CONSTS_FROM_CONFIG(float, base_home_pos,   HOME_POS);
XYZ_CONSTS_FROM_CONFIG(float, max_length,      MAX_LENGTH);
XYZ_CONSTS_FROM_CONFIG(float, home_retract_mm, HOME_RETRACT_MM);
XYZ_CONSTS_FROM_CONFIG(signed char, home_dir,  HOME_DIR);

static void axis_is_at_home(int axis) {
  current_position[axis] = base_home_pos(axis) + add_homeing[axis];
  min_pos[axis] =          base_min_pos(axis);// + add_homeing[axis];
  max_pos[axis] =          base_max_pos(axis);// + add_homeing[axis];
}

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

    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = 1.5 * max_length(axis) * home_dir(axis);
    feedrate = homing_feedrate[axis];
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();
    if (!isEndstopHit())
    {
        if (axis == Z_AXIS)
        {
            current_position[axis] = 0;
            plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
            destination[axis] = -home_retract_mm(axis) * home_dir(axis) * 10.0;
            plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
            st_synchronize();

            SERIAL_ERROR_START;
            SERIAL_ERRORLNPGM("Z_ENDSTOP_ERROR: Endstop not pressed after homing down. Endstop broken?");
            Stop(STOP_REASON_Z_ENDSTOP_BROKEN_ERROR);
        }else{
            SERIAL_ERROR_START;
            SERIAL_ERRORLNPGM("XY_ENDSTOP_ERROR: Endstop not pressed after homing down. Endstop broken?");
            Stop(STOP_REASON_XY_ENDSTOP_BROKEN_ERROR);
        }
        return;
    }

    current_position[axis] = 0;
    plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
    destination[axis] = -home_retract_mm(axis) * home_dir(axis);
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
    st_synchronize();

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
    if (endstop_pressed)
    {
        SERIAL_ERROR_START;
        if (axis == Z_AXIS)
        {
            SERIAL_ERRORLNPGM("Z_ENDSTOP_ERROR: Endstop still pressed after backing off. Endstop stuck?");
            Stop(STOP_REASON_Z_ENDSTOP_STUCK_ERROR);
        }else{
            SERIAL_ERRORLNPGM("XY_ENDSTOP_ERROR: Endstop still pressed after backing off. Endstop stuck?");
            Stop(STOP_REASON_XY_ENDSTOP_STUCK_ERROR);
        }
        endstops_hit_on_purpose();
        return;
    }

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

#ifdef ENABLE_BED_LEVELING_PROBE
float probeWithCapacitiveSensor()
{
    float total_z_height = 0.0;
    float z_target = 0.0;
    float z_distance = 5.0;

    feedrate = 1;
    for(uint8_t loop_counter = 0; loop_counter < CONFIG_BED_LEVEL_PROBE_REPEAT; loop_counter++)
    {
        destination[Z_AXIS] = z_target + z_distance;
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
        st_synchronize();
        destination[Z_AXIS] = z_target - z_distance;
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate, active_extruder);
        uint16_t cnt = 0;
        long sensor_value_total = 0;
        float z_position_total = 0.0;
        
        int16_t max_diff = 0;
        int16_t max_sensor_value = 0;
        int16_t previous_sensor_value = 0;
        uint8_t steady_counter = 0;

        float sensor_value_list[6];
        float z_position_list[6];
        for(uint8_t n=0; n<6; n++)
        {
            sensor_value_list[n] = 0;
            z_position_list[n] = 0;
        }

        i2cCapacitanceStart();
        while(blocks_queued())
        {
            manage_heater();
            manage_inactivity();
            uint16_t value = 0;
            if (i2cCapacitanceDone(value) && value != 0xFFFF)
            {
                z_position_total += float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS];
                sensor_value_total += value;
                cnt++;
                if (cnt == 1000)
                {
                    z_position_total /= 1000;
                    sensor_value_total /= 1000;
                    
                    if (previous_sensor_value != 0)
                    {
                        for(uint8_t n=1; n<6; n++)
                        {
                            sensor_value_list[n - 1] = sensor_value_list[n];
                            z_position_list[n - 1] = z_position_list[n];
                        }
                        sensor_value_list[5] = sensor_value_total;
                        z_position_list[5] = z_position_total;

                        if (sensor_value_total > previous_sensor_value + max_diff / 3)
                        {
                            max_sensor_value = sensor_value_total;
                            steady_counter = 0;
                        }else{
                            steady_counter++;
                            if (steady_counter > 2)
                            {
                                quickStop();
                                current_position[X_AXIS] = destination[X_AXIS];
                                current_position[Y_AXIS] = destination[Y_AXIS];
                                current_position[Z_AXIS] = float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS];
                                plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
                            }
                        }

                        int16_t diff = sensor_value_total - previous_sensor_value;
                        if (diff > max_diff)
                            max_diff = diff;
                    }
                    previous_sensor_value = sensor_value_total;
    /*
                    MSerial.print(z_position_total);
                    MSerial.print(' ');
                    MSerial.print(float(sensor_value_total) / 100.0f);
                    MSerial.print(' ');
                    MSerial.print(float(max_sensor_value) / 100.0f);
                    MSerial.print(' ');
                    MSerial.print(float(max_diff) / 100.0f);
                    MSerial.println();
    */
                    cnt = 0;
                    sensor_value_total = 0;
                    z_position_total = 0;
                }
                i2cCapacitanceStart();
            }
        }
    /*
        MSerial.println();
        for(uint8_t n=0; n<6; n++)
        {
            MSerial.print(z_position_list[n]);
            MSerial.print(' ');
            MSerial.print(float(sensor_value_list[n]) / 100.0f);
            MSerial.println();
        }
    */
        //Solve the line-line intersection to get the proper position where the bed was hit. At index [1] we are sure to be above the bed. At index[2] we are already on the bed. So the intersection point is somewhere between [1] and [2].
        float f = float((sensor_value_list[2]-(sensor_value_list[3]-sensor_value_list[2]))-sensor_value_list[1])/float((sensor_value_list[1]-sensor_value_list[0])-(sensor_value_list[3]-sensor_value_list[2]));
        if (f > 0.0 && f < 1.0)
        {
            float z_height = z_position_list[1] + (z_position_list[2] - z_position_list[1]) * f;
            z_target = z_height;
            z_distance = 1.0;
            total_z_height += z_height;
        }else{
            loop_counter --;
        }
    }
    return total_z_height / CONFIG_BED_LEVEL_PROBE_REPEAT;
}
#endif//ENABLE_BED_LEVELING_PROBE

void process_commands()
{
  unsigned long codenum; //throw away variable

  if(code_seen('G'))
  {
    switch((int)code_value())
    {
    case 0: // G0 -> G1
    case 1: // G1
      if(Stopped == false) {
        get_coordinates(); // For X Y Z E F
        prepare_move();
      }
      break;
    case 2: // G2  - CW ARC
      if(Stopped == false) {
        get_arc_coordinates();
        prepare_arc_move(true);
      }
      break;
    case 3: // G3  - CCW ARC
      if(Stopped == false) {
        get_arc_coordinates();
        prepare_arc_move(false);
      }
      break;
    case 4: // G4 dwell
      codenum = 0;
      if(code_seen('P')) codenum = code_value(); // milliseconds to wait
      if(code_seen('S')) codenum = code_value() * 1000; // seconds to wait

      st_synchronize();
      codenum += millis();  // keep track of when we started waiting
      previous_millis_cmd = millis();
      while(millis()  < codenum ){
        manage_heater();
        manage_inactivity();
      }
      break;
      #ifdef FWRETRACT
      case 10: // G10 retract
      if(!retracted)
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS];
        #if EXTRUDERS > 1
        if (code_seen('S') && code_value_long() == 1)
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
        prepare_move();
        feedrate = oldFeedrate;
      }

      break;
      case 11: // G11 retract_recover
      if(retracted)
      {
        destination[X_AXIS]=current_position[X_AXIS];
        destination[Y_AXIS]=current_position[Y_AXIS];
        destination[Z_AXIS]=current_position[Z_AXIS];
        destination[E_AXIS]=current_position[E_AXIS]+retract_recover_length;
        float oldFeedrate = feedrate;
        feedrate=retract_recover_feedrate;
        retracted=false;
        prepare_move();
        feedrate = oldFeedrate;
      }
      break;
      #endif //FWRETRACT
    case 28: //G28 Home all Axis one at a time
      saved_feedrate = feedrate;
      saved_feedmultiply = feedmultiply;
      feedmultiply = 100;
      previous_millis_cmd = millis();

      enable_endstops(true);

      for(int8_t i=0; i < NUM_AXIS; i++) {
        destination[i] = current_position[i];
      }
          feedrate = 0.0;

          home_all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2])));

      #if Z_HOME_DIR > 0                      // If homing away from BED do Z first
      #if defined(QUICK_HOME)
      if(home_all_axis)
      {
        current_position[X_AXIS] = 0; current_position[Y_AXIS] = 0; current_position[Z_AXIS] = 0;

        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);

        destination[X_AXIS] = 1.5 * X_MAX_LENGTH * X_HOME_DIR;
        destination[Y_AXIS] = 1.5 * Y_MAX_LENGTH * Y_HOME_DIR;
        destination[Z_AXIS] = 1.5 * Z_MAX_LENGTH * Z_HOME_DIR;
        feedrate = homing_feedrate[X_AXIS];
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        st_synchronize();
        endstops_hit_on_purpose();

        axis_is_at_home(X_AXIS);
        axis_is_at_home(Y_AXIS);
        axis_is_at_home(Z_AXIS);
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
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
      if((home_all_axis) || (code_seen(axis_codes[Z_AXIS]))) {
        HOMEAXIS(Z);
      }
      #endif

      #if defined(QUICK_HOME)
      if((home_all_axis)||( code_seen(axis_codes[X_AXIS]) && code_seen(axis_codes[Y_AXIS])) )  //first diagonal move
      {
        current_position[X_AXIS] = 0;current_position[Y_AXIS] = 0;

        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        destination[X_AXIS] = 1.5 * X_MAX_LENGTH * X_HOME_DIR;destination[Y_AXIS] = 1.5 * Y_MAX_LENGTH * Y_HOME_DIR;
        feedrate = homing_feedrate[X_AXIS];
        if(homing_feedrate[Y_AXIS]<feedrate)
          feedrate =homing_feedrate[Y_AXIS];
        plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
        st_synchronize();

        axis_is_at_home(X_AXIS);
        axis_is_at_home(Y_AXIS);
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
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

      if((home_all_axis) || (code_seen(axis_codes[X_AXIS])))
      {
        HOMEAXIS(X);
      }

      if((home_all_axis) || (code_seen(axis_codes[Y_AXIS]))) {
        HOMEAXIS(Y);
      }

      #if Z_HOME_DIR < 0                      // If homing towards BED do Z last
      if((home_all_axis) || (code_seen(axis_codes[Z_AXIS]))) {
        HOMEAXIS(Z);
      }
      #endif

      if(code_seen(axis_codes[X_AXIS]))
      {
        if(code_value_long() != 0) {
          current_position[X_AXIS]=code_value()+add_homeing[0];
        }
      }

      if(code_seen(axis_codes[Y_AXIS])) {
        if(code_value_long() != 0) {
          current_position[Y_AXIS]=code_value()+add_homeing[1];
        }
      }

      if(code_seen(axis_codes[Z_AXIS])) {
        if(code_value_long() != 0) {
          current_position[Z_AXIS]=code_value()+add_homeing[2];
        }
      }
      plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);

      #ifdef ENDSTOPS_ONLY_FOR_HOMING
        enable_endstops(false);
      #endif

      feedrate = saved_feedrate;
      feedmultiply = saved_feedmultiply;
      previous_millis_cmd = millis();
      endstops_hit_on_purpose();
      break;
#ifdef ENABLE_BED_LEVELING_PROBE
    case 29://G29 - Automatic bed leveling with probing.
      {
          planner_bed_leveling_factor[X_AXIS] = 0.0;
          planner_bed_leveling_factor[Y_AXIS] = 0.0;
          
          destination[Z_AXIS] = CONFIG_BED_LEVELING_Z_HEIGHT;
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
          destination[X_AXIS] = CONFIG_BED_LEVELING_POINT1_X;
          destination[Y_AXIS] = CONFIG_BED_LEVELING_POINT1_Y;
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[X_AXIS], active_extruder);
          st_synchronize();
          float height_1 = probeWithCapacitiveSensor();

          destination[Z_AXIS] = CONFIG_BED_LEVELING_Z_HEIGHT;
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
          destination[X_AXIS] = CONFIG_BED_LEVELING_POINT2_X;
          destination[Y_AXIS] = CONFIG_BED_LEVELING_POINT2_Y;
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[X_AXIS], active_extruder);
          st_synchronize();
          float height_2 = probeWithCapacitiveSensor();

          destination[Z_AXIS] = CONFIG_BED_LEVELING_Z_HEIGHT;
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
          destination[X_AXIS] = CONFIG_BED_LEVELING_POINT3_X;
          destination[Y_AXIS] = CONFIG_BED_LEVELING_POINT3_Y;
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[X_AXIS], active_extruder);
          st_synchronize();
          float height_3 = probeWithCapacitiveSensor();
          destination[Z_AXIS] = height_3;
          //Position the head at exactly the height, so we can use this as Z0 later.
          plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[X_AXIS], active_extruder);
          st_synchronize();

          MSerial.println(height_1, 5);
          MSerial.println(height_2, 5);
          MSerial.println(height_3, 5);
          
          //Set the X and Y skew factors for how skewed the bed is (this assumes the leveling points are 1 in the back, and 2 at the front)
          planner_bed_leveling_factor[X_AXIS] = (height_3 - height_2) / (CONFIG_BED_LEVELING_POINT3_X - CONFIG_BED_LEVELING_POINT2_X);
          planner_bed_leveling_factor[Y_AXIS] = ((height_2 + height_3) / 2.0 - height_1) / (CONFIG_BED_LEVELING_POINT3_Y - CONFIG_BED_LEVELING_POINT1_Y);
          
          MSerial.println(planner_bed_leveling_factor[X_AXIS], 5);
          MSerial.println(planner_bed_leveling_factor[Y_AXIS], 5);
          
          //Correct the Z position. So Z0 is always on top of the bed. We are currently positioned at point 3, on top of the bed.
          destination[Z_AXIS] = 0.0;
          plan_set_position(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS]);
      }
      break;
    case 30: // G30 Probe Z at current position and report result.
      destination[Z_AXIS] = probeWithCapacitiveSensor();
      plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], homing_feedrate[Z_AXIS], active_extruder);
      MSerial.println(destination[Z_AXIS]);
      break;
#endif
    case 90: // G90
      relative_mode = false;
      break;
    case 91: // G91
      relative_mode = true;
      break;
    case 92: // G92
      if(!code_seen(axis_codes[E_AXIS]))
        st_synchronize();
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) {
           if(i == E_AXIS) {
             current_position[i] = code_value();
             plan_set_e_position(current_position[E_AXIS]);
           }
           else {
             current_position[i] = code_value();
             plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
           }
        }
      }
      break;
    case 1000: // G1000 - store current position&feedrate
      for(int8_t i=0; i < NUM_AXIS; i++) {
        stored_position[i] = current_position[i];
      }
      stored_feedrate = feedrate;
      break;
    case 1001: // G1001 - return to stored position from G1000
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) {
          destination[i] = current_position[i];
        }
      }
      if(code_seen('F')) {
        next_feedrate = code_value();
        if (next_feedrate > 0)
            feedrate = next_feedrate;
      }
      prepare_move();
      feedrate = stored_feedrate;
      break;
    }
  }

  else if(code_seen('M'))
  {
    switch( (int)code_value() )
    {
    case 17:
        enable_x();
        enable_y();
        enable_z();
        enable_e0();
        enable_e1();
        enable_e2();
      break;

    case 42: //M42 -Change pin status via gcode
      if (code_seen('S'))
      {
        int pin_status = code_value();
        int pin_number = LED_PIN;
        if (code_seen('P') && pin_status >= 0 && pin_status <= 255)
          pin_number = code_value();
        for(int8_t i = 0; i < (int8_t)sizeof(sensitive_pins); i++)
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
          pinMode(pin_number, OUTPUT);
          digitalWrite(pin_number, pin_status);
          analogWrite(pin_number, pin_status);
        }
      }
     break;
    case 142:
      {
	    uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		uint8_t w = 0;
        if (code_seen('R'))
          r = code_value();
        if (code_seen('G'))
          g = code_value();
        if (code_seen('B'))
          b = code_value();
        if (code_seen('W'))
          w = code_value();
	    ledRGBWUpdate(r, g, b, w);
      }
      break;
    case 104: // M104
      if(setTargetedHotend(104)){
        break;
      }
      if (code_seen('S')) setTargetHotend(code_value(), tmp_extruder);
      setWatch();
      break;
    case 140: // M140 set bed temp
      if (code_seen('S')) setTargetBed(code_value());
      break;
    case 105 : // M105
      if(setTargetedHotend(105)){
        break;
        }
      #if defined(TEMP_0_PIN) && TEMP_0_PIN > -1
        SERIAL_PROTOCOL_OK();
        SERIAL_PROTOCOLPGM(" T:");
        SERIAL_PROTOCOL_F(degHotend(tmp_extruder),1);
        SERIAL_PROTOCOLPGM(" /");
        SERIAL_PROTOCOL_F(degTargetHotend(tmp_extruder),1);
        #if defined(TEMP_BED_PIN) && TEMP_BED_PIN > -1
          SERIAL_PROTOCOLPGM(" B:");
          SERIAL_PROTOCOL_F(degBed(),1);
          SERIAL_PROTOCOLPGM(" /");
          SERIAL_PROTOCOL_F(degTargetBed(),1);
        #endif //TEMP_BED_PIN
      #else
        SERIAL_ERROR_START;
        SERIAL_ERRORLNPGM(MSG_ERR_NO_THERMISTORS);
      #endif

        SERIAL_PROTOCOLPGM(" @:");
        SERIAL_PROTOCOL(getHeaterPower(tmp_extruder));

        SERIAL_PROTOCOLPGM(" B@:");
        SERIAL_PROTOCOL(getHeaterPower(-1));

        SERIAL_PROTOCOLLN("");
      return;
      break;
    case 109:
    {// M109 - Wait for extruder heater to reach target.
      if(setTargetedHotend(109)){
        break;
      }
      if (code_seen('S')) setTargetHotend(code_value(), tmp_extruder);

      setWatch();
      codenum = millis();

      /* See if we are heating up or cooling down */
      bool target_direction = isHeatingHotend(tmp_extruder); // true if heating, false if cooling

      #ifdef TEMP_RESIDENCY_TIME
        long residencyStart;
        residencyStart = -1;
        /* continue to loop until we have reached the target temp
          _and_ until TEMP_RESIDENCY_TIME hasn't passed since we reached it */
        while((residencyStart == -1) ||
              (residencyStart >= 0 && (((unsigned int) (millis() - residencyStart)) < (TEMP_RESIDENCY_TIME * 1000UL))) ) {
      #else
        while ( target_direction ? (isHeatingHotend(tmp_extruder)) : (isCoolingHotend(tmp_extruder)&&(CooldownNoWait==false)) ) {
      #endif //TEMP_RESIDENCY_TIME
          if( (millis() - codenum) > 1000UL )
          { //Print Temp Reading and remaining time every 1 second while heating up/cooling down
            SERIAL_PROTOCOLPGM("T:");
            SERIAL_PROTOCOL_F(degHotend(tmp_extruder),1);
            SERIAL_PROTOCOLPGM(" E:");
            SERIAL_PROTOCOL((int)tmp_extruder);
            #ifdef TEMP_RESIDENCY_TIME
              SERIAL_PROTOCOLPGM(" W:");
              if(residencyStart > -1)
              {
                 codenum = ((TEMP_RESIDENCY_TIME * 1000UL) - (millis() - residencyStart)) / 1000UL;
                 SERIAL_PROTOCOLLN( codenum );
              }
              else
              {
                 SERIAL_PROTOCOLLN( "?" );
              }
            #else
              SERIAL_PROTOCOLLN("");
            #endif
            codenum = millis();
          }
          manage_heater();
          manage_inactivity();
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
        }
        starttime=millis();
        previous_millis_cmd = millis();
      }
      break;
    case 190: // M190 - Wait for bed heater to reach target.
    #if defined(TEMP_BED_PIN) && TEMP_BED_PIN > -1
        if (code_seen('S')) setTargetBed(code_value());
        codenum = millis();
        while(current_temperature_bed < target_temperature_bed - TEMP_WINDOW)
        {
          if(( millis() - codenum) > 1000 ) //Print Temp Reading every 1 second while heating up.
          {
            float tt=degHotend(active_extruder);
            SERIAL_PROTOCOLPGM("T:");
            SERIAL_PROTOCOL(tt);
            SERIAL_PROTOCOLPGM(" E:");
            SERIAL_PROTOCOL((int)active_extruder);
            SERIAL_PROTOCOLPGM(" B:");
            SERIAL_PROTOCOL_F(degBed(),1);
            SERIAL_PROTOCOLLN("");
            codenum = millis();
          }
          manage_heater();
          manage_inactivity();
        }
        previous_millis_cmd = millis();
    #endif
        break;

    #if defined(FAN_PIN) && FAN_PIN > -1
      case 106: //M106 Fan On
        if (code_seen('S')){
           fanSpeed=constrain(code_value() * fanSpeedPercent / 100,0,255);
        }
        else {
          fanSpeed = 255 * int(fanSpeedPercent) / 100;
        }
        break;
      case 107: //M107 Fan Off
        fanSpeed = 0;
        break;
    #endif //FAN_PIN
    
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
      axis_relative_modes[3] = false;
      break;
    case 83:
      axis_relative_modes[3] = true;
      break;
    case 18: //compatibility
    case 84: // M84
      if(code_seen('S')){
        stepper_inactive_time = code_value() * 1000;
      }
      else
      {
        bool all_axis = !((code_seen(axis_codes[0])) || (code_seen(axis_codes[1])) || (code_seen(axis_codes[2]))|| (code_seen(axis_codes[3])));
        if(all_axis)
        {
          st_synchronize();
          disable_e0();
          disable_e1();
          disable_e2();
          finishAndDisableSteppers();
        }
        else
        {
          st_synchronize();
          if(code_seen('X')) disable_x();
          if(code_seen('Y')) disable_y();
          if(code_seen('Z')) disable_z();
          #if ((E0_ENABLE_PIN != X_ENABLE_PIN) && (E1_ENABLE_PIN != Y_ENABLE_PIN)) // Only enable on boards that have seperate ENABLE_PINS
            if(code_seen('E')) {
              disable_e0();
              disable_e1();
              disable_e2();
            }
          #endif
        }
      }
      break;
    case 85: // M85
      if (code_seen('S')) max_inactive_time = code_value() * 1000;
      break;
    case 92: // M92
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(axis_codes[i]))
        {
          if(i == 3) { // E
            float value = code_value();
            axis_steps_per_unit[i] = value;
          }
          else {
            axis_steps_per_unit[i] = code_value();
          }
        }
      }
      break;
    case 115: // M115
      SERIAL_PROTOCOLPGM(MSG_M115_REPORT);
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

      SERIAL_PROTOCOLPGM(" count X:");
      SERIAL_PROTOCOL(float(st_get_position(X_AXIS))/axis_steps_per_unit[X_AXIS]);
      SERIAL_PROTOCOLPGM("Y:");
      SERIAL_PROTOCOL(float(st_get_position(Y_AXIS))/axis_steps_per_unit[Y_AXIS]);
      SERIAL_PROTOCOLPGM("Z:");
      SERIAL_PROTOCOL(float(st_get_position(Z_AXIS))/axis_steps_per_unit[Z_AXIS]);

      SERIAL_PROTOCOLLN("");
      break;
    case 120: // M120
      enable_endstops(false) ;
      break;
    case 121: // M121
      enable_endstops(true) ;
      break;
    case 119: // M119
    SERIAL_PROTOCOLLN(MSG_M119_REPORT);
      #if defined(X_MIN_PIN) && X_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_X_MIN);
        SERIAL_PROTOCOLLN(((READ(X_MIN_PIN)^X_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(X_MAX_PIN) && X_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_X_MAX);
        SERIAL_PROTOCOLLN(((READ(X_MAX_PIN)^X_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Y_MIN_PIN) && Y_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Y_MIN);
        SERIAL_PROTOCOLLN(((READ(Y_MIN_PIN)^Y_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Y_MAX_PIN) && Y_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Y_MAX);
        SERIAL_PROTOCOLLN(((READ(Y_MAX_PIN)^Y_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Z_MIN_PIN) && Z_MIN_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Z_MIN);
        SERIAL_PROTOCOLLN(((READ(Z_MIN_PIN)^Z_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      #if defined(Z_MAX_PIN) && Z_MAX_PIN > -1
        SERIAL_PROTOCOLPGM(MSG_Z_MAX);
        SERIAL_PROTOCOLLN(((READ(Z_MAX_PIN)^Z_ENDSTOPS_INVERTING)?MSG_ENDSTOP_HIT:MSG_ENDSTOP_OPEN));
      #endif
      break;
      //TODO: update for all axis, use for loop
    case 201: // M201
      for(int8_t i=0; i < NUM_AXIS; i++)
      {
        if(code_seen(axis_codes[i]))
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
        if(code_seen(axis_codes[i])) axis_travel_steps_per_sqr_second[i] = code_value() * axis_steps_per_unit[i];
      }
      break;
    #endif
    case 203: // M203 max feedrate mm/sec
      for(int8_t i=0; i < NUM_AXIS; i++) {
        if(code_seen(axis_codes[i])) max_feedrate[i] = code_value();
      }
      break;
    case 204: // M204 acceleration: S - normal moves;  T - filament only moves
      {
        if(code_seen('S')) acceleration = code_value() ;
        if(code_seen('T')) retract_acceleration = code_value() ;
      }
      break;
    case 205: //M205 advanced settings:  minimum travel speed S=while printing T=travel only,  B=minimum segment time X= maximum xy jerk, Z=maximum Z jerk
    {
      if(code_seen('S')) minimumfeedrate = code_value();
      if(code_seen('T')) mintravelfeedrate = code_value();
      if(code_seen('B')) minsegmenttime = code_value() ;
      if(code_seen('X')) max_xy_jerk = code_value() ;
      if(code_seen('Z')) max_z_jerk = code_value() ;
      if(code_seen('E')) max_e_jerk = code_value() ;
    }
    break;
    case 206: // M206 additional homing offset
      for(int8_t i=0; i < 3; i++)
      {
        if(code_seen(axis_codes[i])) add_homeing[i] = code_value();
      }
      break;
    #ifdef FWRETRACT
    case 207: //M207 - set retract length S[positive mm] F[feedrate mm/min] Z[additional zlift/hop]
    {
      if(code_seen('S'))
      {
        retract_length = code_value() ;
      }
      if(code_seen('F'))
      {
        retract_feedrate = code_value() ;
      }
      if(code_seen('Z'))
      {
        retract_zlift = code_value() ;
      }
    }break;
    case 208: // M208 - set retract recover length S[positive mm surplus to the M207 S*] F[feedrate mm/min]
    {
      if(code_seen('S'))
      {
        retract_recover_length = code_value() ;
      }
      if(code_seen('F'))
      {
        retract_recover_feedrate = code_value() ;
      }
    }break;
    #endif // FWRETRACT
    #if EXTRUDERS > 1
    case 218: // M218 - set hotend offset (in mm), T<extruder_number> X<offset_on_X> Y<offset_on_Y> Z<offset_on_Z>
    {
      if(setTargetedHotend(218)){
        break;
      }
      if(code_seen('X'))
      {
        extruder_offset[X_AXIS][tmp_extruder] = code_value();
      }
      if(code_seen('Y'))
      {
        extruder_offset[Y_AXIS][tmp_extruder] = code_value();
      }
      if(code_seen('Z'))
      {
        extruder_offset[Z_AXIS][tmp_extruder] = code_value();
      }
      SERIAL_ECHO_START;
      SERIAL_ECHOPGM(MSG_HOTEND_OFFSET);
      for(tmp_extruder = 0; tmp_extruder < EXTRUDERS; tmp_extruder++)
      {
         SERIAL_ECHO(" ");
         SERIAL_ECHO(extruder_offset[X_AXIS][tmp_extruder]);
         SERIAL_ECHO(",");
         SERIAL_ECHO(extruder_offset[Y_AXIS][tmp_extruder]);
         SERIAL_ECHO(",");
         SERIAL_ECHO(extruder_offset[Z_AXIS][tmp_extruder]);
      }
      SERIAL_ECHOLN("");
    }break;
    #endif
    case 220: // M220 S<factor in percent>- set speed factor override percentage
    {
      if(code_seen('S'))
      {
        feedmultiply = code_value();
      }
    }
    break;
    case 221: // M221 S<factor in percent>- set extrude factor override percentage
    {
      if(code_seen('S'))
      {
        extrudemultiply[active_extruder] = code_value() ;
      }
    }
    break;

    #ifdef PIDTEMP
    case 301: // M301
      {
        if(code_seen('P')) Kp = code_value();
        if(code_seen('I')) Ki = scalePID_i(code_value());
        if(code_seen('D')) Kd = scalePID_d(code_value());

        updatePID();
        SERIAL_PROTOCOL_OK();
        SERIAL_PROTOCOL(" p:");
        SERIAL_PROTOCOL(Kp);
        SERIAL_PROTOCOL(" i:");
        SERIAL_PROTOCOL(unscalePID_i(Ki));
        SERIAL_PROTOCOL(" d:");
        SERIAL_PROTOCOL(unscalePID_d(Kd));
        SERIAL_PROTOCOLLN("");
      }
      break;
    #endif //PIDTEMP
    #ifdef PIDTEMPBED
    case 304: // M304
      {
        if(code_seen('P')) bedKp = code_value();
        if(code_seen('I')) bedKi = scalePID_i(code_value());
        if(code_seen('D')) bedKd = scalePID_d(code_value());

        updatePID();
        SERIAL_PROTOCOL_OK();
        SERIAL_PROTOCOL(" p:");
        SERIAL_PROTOCOL(bedKp);
        SERIAL_PROTOCOL(" i:");
        SERIAL_PROTOCOL(unscalePID_i(bedKi));
        SERIAL_PROTOCOL(" d:");
        SERIAL_PROTOCOL(unscalePID_d(bedKd));
        SERIAL_PROTOCOLLN("");
      }
      break;
    #endif //PIDTEMP
    #ifdef PREVENT_DANGEROUS_EXTRUDE
    case 302: // allow cold extrudes, or set the minimum extrude temperature
    {
	  float temp = .0;
	  if (code_seen('S')) temp=code_value();
      set_extrude_min_temp(temp);
    }
    break;
	#endif
    case 303: // M303 PID autotune
    {
      float temp = 150.0;
      int e=0;
      int c=5;
      if (code_seen('E')) e=code_value();
        if (e<0)
          temp=70;
      if (code_seen('S')) temp=code_value();
      if (code_seen('C')) c=code_value();
      PID_autotune(temp, e, c);
    }
    break;
#ifdef ENABLE_BED_LEVELING_PROBE
    case 310://M310, single cap sensor read.
      {
        i2cCapacitanceStart();
        uint16_t value;
        while(!i2cCapacitanceDone(value))
        {
            manage_heater();
            manage_inactivity();
        }
        MSerial.println(value, HEX);
      }
    break;
#endif//ENABLE_BED_LEVELING_PROBE
    case 400: // M400 finish all moves
    {
      st_synchronize();
    }
    break;
    case 401: // M401 quickstop - Abort all the planned moves. This will stop the head mid-move, so most likely the head will be out of sync with the stepper position after this.
    {
      quickStop();
    }
    break;

    case 907: // M907 Set digital trimpot motor current using axis codes.
    {
      #if defined(MOTOR_CURRENT_PWM_XY_PIN) && MOTOR_CURRENT_PWM_XY_PIN > -1
        if(code_seen('X')) digipot_current(0, code_value());
      #endif
      #if defined(MOTOR_CURRENT_PWM_Z_PIN) && MOTOR_CURRENT_PWM_Z_PIN > -1
        if(code_seen('Z')) digipot_current(1, code_value());
      #endif
      #if defined(MOTOR_CURRENT_PWM_E_PIN) && MOTOR_CURRENT_PWM_E_PIN > -1
        if(code_seen('E')) digipot_current(2, code_value());
      #endif
    }
    break;
    case 999: // M999: Restart after being stopped
      Stopped = false;
      gcode_LastN = Stopped_gcode_LastN;
      FlushSerialRequestResend();
    break;
    }
  }

  else if(code_seen('T'))
  {
    tmp_extruder = code_value();
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      SERIAL_ECHO("T");
      SERIAL_ECHO(tmp_extruder);
      SERIAL_ECHOLN(MSG_INVALID_EXTRUDER);
    }
    else {
      boolean make_move = false;
      if(code_seen('F')) {
        make_move = true;
        next_feedrate = code_value();
        if(next_feedrate > 0.0) {
          feedrate = next_feedrate;
        }
      }
      #if EXTRUDERS > 1
      if(tmp_extruder != active_extruder) {
        // Save current position to return to after applying extruder offset
        memcpy(destination, current_position, sizeof(destination));
        // Offset extruder (only by XY)
        int i;
        for(i = 0; i < 3; i++) {
           current_position[i] = current_position[i] -
                                 extruder_offset[i][active_extruder] +
                                 extruder_offset[i][tmp_extruder];
        }
        // Set the new active extruder and position
        active_extruder = tmp_extruder;
        plan_set_position(current_position[X_AXIS], current_position[Y_AXIS], current_position[Z_AXIS], current_position[E_AXIS]);
        // Move to the old position if 'F' was in the parameters
        if(make_move && Stopped == false) {
           prepare_move();
        }
      }
      #endif
      SERIAL_ECHO_START;
      SERIAL_ECHO(MSG_ACTIVE_EXTRUDER);
      SERIAL_PROTOCOLLN((int)active_extruder);
    }
  }
  else if (strcmp_P(command_buffer[command_buffer_index_read], PSTR("Electronics_test")) == 0)
  {
    run_electronics_test();
  }
  else
  {
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM(MSG_UNKNOWN_COMMAND);
    SERIAL_ECHO(command_buffer[command_buffer_index_read]);
    SERIAL_ECHOLNPGM("\"");
  }

  ClearToSend();
}

void FlushSerialRequestResend()
{
  SERIAL_PROTOCOLPGM(MSG_RESEND);
  SERIAL_PROTOCOLLN(gcode_LastN + 1);
  previous_millis_cmd = millis();
  SERIAL_PROTOCOLLNPGM(MSG_OK);
}

void ClearToSend()
{
  previous_millis_cmd = millis();
  SERIAL_PROTOCOL_OK_LN();
}

void get_coordinates()
{
    bool seen[4]={false,false,false,false};
    for(int8_t i=0; i < NUM_AXIS; i++)
    {
        if(code_seen(axis_codes[i]))
        {
            destination[i] = (float)code_value() + (axis_relative_modes[i] || relative_mode)*current_position[i];
            seen[i]=true;
        }
        else
        {
            destination[i] = current_position[i]; //Are these else lines really needed?
        }
    }
    if(code_seen('F'))
    {
        next_feedrate = code_value();
        if(next_feedrate > 0.0) feedrate = next_feedrate;
    }
}

void get_arc_coordinates()
{
   get_coordinates();

   if(code_seen('I')) {
     offset[0] = code_value();
   }
   else {
     offset[0] = 0.0;
   }
   if(code_seen('J')) {
     offset[1] = code_value();
   }
   else {
     offset[1] = 0.0;
   }
}

void clamp_to_software_endstops(float target[3])
{
  if (min_software_endstops) {
    if (target[X_AXIS] < min_pos[X_AXIS]) target[X_AXIS] = min_pos[X_AXIS];
    if (target[Y_AXIS] < min_pos[Y_AXIS]) target[Y_AXIS] = min_pos[Y_AXIS];
    if (target[Z_AXIS] < min_pos[Z_AXIS]) target[Z_AXIS] = min_pos[Z_AXIS];
  }

  if (max_software_endstops) {
    if (target[X_AXIS] > max_pos[X_AXIS]) target[X_AXIS] = max_pos[X_AXIS];
    if (target[Y_AXIS] > max_pos[Y_AXIS]) target[Y_AXIS] = max_pos[Y_AXIS];
    if (target[Z_AXIS] > max_pos[Z_AXIS]) target[Z_AXIS] = max_pos[Z_AXIS];
  }
}

void prepare_move()
{
  clamp_to_software_endstops(destination);

  previous_millis_cmd = millis();
  // Do not use feedmultiply for E or Z only moves
  if( (current_position[X_AXIS] == destination [X_AXIS]) && (current_position[Y_AXIS] == destination [Y_AXIS]))
  {
      plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate/60, active_extruder);
  }
  else
  {
    plan_buffer_line(destination[X_AXIS], destination[Y_AXIS], destination[Z_AXIS], destination[E_AXIS], feedrate*feedmultiply/60/100.0, active_extruder);
  }

  for(int8_t i=0; i < NUM_AXIS; i++) {
    current_position[i] = destination[i];
  }
}

void prepare_arc_move(char isclockwise) {
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

void manage_inactivity()
{
  if( (millis() - previous_millis_cmd) >  max_inactive_time )
    if(max_inactive_time)
      kill();
  if(stepper_inactive_time)  {
    if( (millis() - previous_millis_cmd) >  stepper_inactive_time )
    {
      if(blocks_queued() == false) {
        if(DISABLE_X) disable_x();
        if(DISABLE_Y) disable_y();
        if(DISABLE_Z) disable_z();
        if(DISABLE_E) {
            disable_e0();
            disable_e1();
            disable_e2();
        }
      }
    }
  }
  #if defined(KILL_PIN) && KILL_PIN > -1
    if( 0 == READ(KILL_PIN) )
      kill();
  #endif
  #if defined(SAFETY_TRIGGERED_PIN) && SAFETY_TRIGGERED_PIN > -1
  if (READ(SAFETY_TRIGGERED_PIN))
    Stop(STOP_REASON_SAFETY_TRIGGER);
  #endif
  check_axes_activity();
}

void kill()
{
  cli(); // Stop interrupts
  disable_heater();

  disable_x();
  disable_y();
  disable_z();
  disable_e0();
  disable_e1();
  disable_e2();

#if defined(PS_ON_PIN) && PS_ON_PIN > -1
  pinMode(PS_ON_PIN,INPUT);
#endif
  SERIAL_ERROR_START;
  SERIAL_ERRORLNPGM(MSG_ERR_KILLED);
  suicide();
  while(1) { /* Intentionally left empty */ } // Wait for reset
}

void Stop(uint8_t reasonNr)
{
  disable_heater();
  if(Stopped == false) {
    Stopped = reasonNr;
    Stopped_gcode_LastN = gcode_LastN; // Save last g_code for restart
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM(MSG_ERR_STOPPED);
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

bool setTargetedHotend(int code){
  tmp_extruder = active_extruder;
  if(code_seen('T')) {
    tmp_extruder = code_value();
    if(tmp_extruder >= EXTRUDERS) {
      SERIAL_ECHO_START;
      SERIAL_ECHO(MSG_INVALID_EXTRUDER);
      SERIAL_ECHOLN(tmp_extruder);
      return true;
    }
  }
  return false;
}

