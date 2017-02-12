// Tonokip RepRap firmware rewrite based off of Hydra-mmm firmware.
// License: GPL

#ifndef MARLIN_H
#define MARLIN_H

#define  FORCE_INLINE __attribute__((always_inline)) inline

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>


#include "fastio.h"
#include "Configuration.h"
#include "pins.h"

#ifndef AT90USB
#define  HardwareSerial_h // trick to disable the standard HWserial
#endif

#if (ARDUINO >= 100)
# include "Arduino.h"
#else
# include "WProgram.h"
  //Arduino < 1.0.0 does not define this, so we need to do it ourselfs
# define analogInputToDigitalPin(p) ((p) + A0)
#endif

#include "MarlinSerial.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#include "WString.h"

#ifdef AT90USB
  #define MYSERIAL Serial
#else
  #define MYSERIAL MSerial
#endif

#define SERIAL_CHAR(x) MYSERIAL.write(x)
#define SERIAL_PROTOCOLCHAR(x) SERIAL_CHAR(x)
#define SERIAL_EOL SERIAL_CHAR('\n')

#define SERIAL_PROTOCOL(x) MYSERIAL.print(x)
#define SERIAL_PROTOCOL_F(x,y) MYSERIAL.print(x,y)
#define SERIAL_PROTOCOLPGM(x) serialprintPGM(PSTR(x))
#define SERIAL_PROTOCOLLN(x) do {MYSERIAL.print(x);SERIAL_EOL;} while(0)
#define SERIAL_PROTOCOLLNPGM(x) do{serialprintPGM(PSTR(x));SERIAL_EOL;} while(0)


const char errormagic[] PROGMEM ="Error:";
const char echomagic[] PROGMEM ="echo:";
#define SERIAL_ERROR_START serialprintPGM(errormagic);
#define SERIAL_ERROR(x) SERIAL_PROTOCOL(x)
#define SERIAL_ERRORPGM(x) SERIAL_PROTOCOLPGM(x)
#define SERIAL_ERRORLN(x) SERIAL_PROTOCOLLN(x)
#define SERIAL_ERRORLNPGM(x) SERIAL_PROTOCOLLNPGM(x)

#define SERIAL_ECHO_START serialprintPGM(echomagic);
#define SERIAL_ECHO(x) SERIAL_PROTOCOL(x)
#define SERIAL_ECHOPGM(x) SERIAL_PROTOCOLPGM(x)
#define SERIAL_ECHOLN(x) SERIAL_PROTOCOLLN(x)
#define SERIAL_ECHOLNPGM(x) SERIAL_PROTOCOLLNPGM(x)

#define SERIAL_ECHOPAIR(name,value) (serial_echopair_P(PSTR(name),(value)))


void serial_echopair_P(const char *s_P, float v);
void serial_echopair_P(const char *s_P, double v);
void serial_echopair_P(const char *s_P, unsigned long v);

void serial_action_P(const char *s_P);

//things to write to serial from Programmemory. saves 400 to 2k of RAM.
FORCE_INLINE void serialprintPGM(const char *str)
{
  char ch=pgm_read_byte(str);
  while(ch)
  {
    MYSERIAL.write(ch);
    ch=pgm_read_byte(++str);
  }
}

void process_command(const char *strCmd, bool sendAck);
void process_command_P(const char *strCmd);

void idle(); // the standard idle routine calls manage_inactivity()

#if defined(X_ENABLE_PIN) && X_ENABLE_PIN > -1
  #define  enable_x() WRITE(X_ENABLE_PIN, X_ENABLE_ON)
  #define disable_x() WRITE(X_ENABLE_PIN,!X_ENABLE_ON)
#else
  #define enable_x() ;
  #define disable_x() ;
#endif

#if defined(Y_ENABLE_PIN) && Y_ENABLE_PIN > -1
  #define  enable_y() WRITE(Y_ENABLE_PIN, Y_ENABLE_ON)
  #define disable_y() WRITE(Y_ENABLE_PIN,!Y_ENABLE_ON)
#else
  #define enable_y() ;
  #define disable_y() ;
#endif

#if defined(Z_ENABLE_PIN) && Z_ENABLE_PIN > -1
  #ifdef Z_DUAL_STEPPER_DRIVERS
    #define  enable_z() { WRITE(Z_ENABLE_PIN, Z_ENABLE_ON); WRITE(Z2_ENABLE_PIN, Z_ENABLE_ON); }
    #define disable_z() { WRITE(Z_ENABLE_PIN,!Z_ENABLE_ON); WRITE(Z2_ENABLE_PIN,!Z_ENABLE_ON); }
  #else
    #define  enable_z() WRITE(Z_ENABLE_PIN, Z_ENABLE_ON)
    #define disable_z() WRITE(Z_ENABLE_PIN,!Z_ENABLE_ON)
  #endif
#else
  #define enable_z() ;
  #define disable_z() ;
#endif

#if defined(E0_ENABLE_PIN) && (E0_ENABLE_PIN > -1)
  #define enable_e0() WRITE(E0_ENABLE_PIN, E_ENABLE_ON)
  #define disable_e0() WRITE(E0_ENABLE_PIN,!E_ENABLE_ON)
#else
  #define enable_e0()  /* nothing */
  #define disable_e0() /* nothing */
#endif

#if (EXTRUDERS > 1) && defined(E1_ENABLE_PIN) && (E1_ENABLE_PIN > -1)
  #define enable_e1() WRITE(E1_ENABLE_PIN, E_ENABLE_ON)
  #define disable_e1() WRITE(E1_ENABLE_PIN,!E_ENABLE_ON)
#else
  #define enable_e1()  /* nothing */
  #define disable_e1() /* nothing */
#endif

#if (EXTRUDERS > 2) && defined(E2_ENABLE_PIN) && (E2_ENABLE_PIN > -1)
  #define enable_e2() WRITE(E2_ENABLE_PIN, E_ENABLE_ON)
  #define disable_e2() WRITE(E2_ENABLE_PIN,!E_ENABLE_ON)
#else
  #define enable_e2()  /* nothing */
  #define disable_e2() /* nothing */
#endif

#if EXTRUDERS > 1
extern unsigned char last_extruder;
extern float extruder_offset[2][EXTRUDERS];
#endif // EXTRUDERS

enum AxisEnum {X_AXIS=0, Y_AXIS=1, Z_AXIS=2, E_AXIS=3};


#ifdef DELTA
void calculate_delta(float cartesian[3]);
#endif
void kill();
#define STOP_REASON_MAXTEMP              1
#define STOP_REASON_MINTEMP              2
#define STOP_REASON_MAXTEMP_BED          3
#define STOP_REASON_HEATER_ERROR         4
#define STOP_REASON_Z_ENDSTOP_BROKEN_ERROR 5
#define STOP_REASON_Z_ENDSTOP_STUCK_ERROR  6
#define STOP_REASON_XY_ENDSTOP_BROKEN_ERROR 7
#define STOP_REASON_XY_ENDSTOP_STUCK_ERROR  8
#define STOP_REASON_SAFETY_TRIGGER       10
void Stop(uint8_t reasonNr);

bool IsStopped();
uint8_t StoppedReason();

void clear_command_queue();
void enquecommand(const char *cmd); //put an ascii command at the end of the current buffer.
void enquecommand_P(const char *cmd); //put an ascii command at the end of the current buffer, read from flash
uint8_t commands_queued();
void cmd_synchronize();
void clamp_to_software_endstops(float target[3]);

#ifdef FAST_PWM_FAN
void setPwmFrequency(uint8_t pin, int val);
#endif

#ifndef CRITICAL_SECTION_START
  #define CRITICAL_SECTION_START  unsigned char _sreg = SREG; cli();
  #define CRITICAL_SECTION_END    SREG = _sreg;
#endif //CRITICAL_SECTION_START

extern float homing_feedrate[];
extern uint8_t axis_relative_state;
extern int feedmultiply;
extern int extrudemultiply[EXTRUDERS]; // Sets extrude multiply factor (in percent)
extern float current_position[NUM_AXIS] ;
extern float add_homeing[3];
extern float min_pos[3];
extern float max_pos[3];
extern uint8_t fanSpeed;
extern uint8_t fanSpeedPercent;
#ifdef BARICUDA
extern int ValvePressure;
extern int EtoPPressure;
#endif
extern bool position_error;

#ifdef FAN_SOFT_PWM
extern unsigned char fanSpeedSoftPwm;
#endif

#ifdef FWRETRACT
extern bool autoretract_enabled;
extern bool retracted;
extern float retract_length, retract_feedrate, retract_zlift;
#if EXTRUDERS > 1
extern float extruder_swap_retract_length;
#endif
extern float retract_recover_length, retract_recover_feedrate;
#endif

extern unsigned long starttime;
extern unsigned long stoptime;

#if BUFSIZE > 8
extern uint16_t serialCmd;
#else
extern uint8_t serialCmd;
#endif // BUFSIZE

//The printing state from the main command processor. Is not zero when the command processor is in a loop waiting for a result.
extern uint8_t printing_state;
#define PRINT_STATE_NORMAL      0
#define PRINT_STATE_DWELL       1
#define PRINT_STATE_WAIT_USER   2
#define PRINT_STATE_HEATING     3
#define PRINT_STATE_HEATING_BED 4
#define PRINT_STATE_HOMING      5
#define PRINT_STATE_RECOVER     6
#define PRINT_STATE_START       7
#define PRINT_STATE_ABORT       255

// Handling multiple extruders pins
extern uint8_t active_extruder;
extern uint8_t tmp_extruder;

#if EXTRUDERS > 2
  # error Unsupported number of extruders
#elif EXTRUDERS > 1
  # define ARRAY_BY_EXTRUDERS(v1, v2, v3) { v1, v2 }
#else
  # define ARRAY_BY_EXTRUDERS(v1, v2, v3) { v1 }
#endif

extern "C"{
  int freeMemory();
}

#endif
