/*
  temperature.c - temperature control
  Part of Marlin

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
#include "lifetime_stats.h"
#include "UltiLCD2.h"
#include "temperature.h"
#include "watchdog.h"
#include "preferences.h"
#include "tinkergnome.h"
#include "powerbudget.h"

//===========================================================================
//=============================public variables============================
//===========================================================================
uint16_t target_temperature[EXTRUDERS] = { 0 };
int current_temperature_raw[EXTRUDERS] = { 0 };
float current_temperature[EXTRUDERS] = { 0.0 };
#if TEMP_SENSOR_BED != 0
uint16_t target_temperature_bed = 0;
int current_temperature_bed_raw = 0;
float current_temperature_bed = 0.0;
#endif // TEMP_SENSOR_BED
#ifdef TEMP_SENSOR_1_AS_REDUNDANT
  int redundant_temperature_raw = 0;
  float redundant_temperature = 0.0;
#endif
#ifdef PIDTEMP
  float Kp=DEFAULT_Kp;
  float Ki=(DEFAULT_Ki*PID_dT);
  float Kd=(DEFAULT_Kd/PID_dT);
//  #ifdef PID_ADD_EXTRUSION_RATE
//    float Kc=DEFAULT_Kc;
//  #endif
#endif //PIDTEMP

#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
  float bedKp=DEFAULT_bedKp;
  float bedKi=(DEFAULT_bedKi*PID_dT);
  float bedKd=(DEFAULT_bedKd/PID_dT);
#endif //PIDTEMPBED

#ifdef FAN_SOFT_PWM
  unsigned char fanSpeedSoftPwm;
#endif

#if defined(BABYSTEPPING)
  volatile int babystepsTodo[3]={0,0,0};
#endif

//===========================================================================
//=============================private variables============================
//===========================================================================
static volatile bool temp_meas_ready = false;

#ifdef PIDTEMP
  //static cannot be external:
  static float temp_iState[EXTRUDERS] = { 0 };
  static float temp_dState[EXTRUDERS] = { 0 };
  static float pTerm[EXTRUDERS];
  static float iTerm[EXTRUDERS];
  static float dTerm[EXTRUDERS];
  //int output;
  static float pid_error[EXTRUDERS];
  static float temp_iState_min[EXTRUDERS];
  static float temp_iState_max[EXTRUDERS];
  // static float pid_input[EXTRUDERS];
  // static float pid_output[EXTRUDERS];
  static bool pid_reset[EXTRUDERS];
#endif //PIDTEMP
#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
  //static cannot be external:
  static float pTerm_bed;
  static float iTerm_bed;
  //int output;
  static float temp_iState_bed = { 0 };
  static float temp_dState_bed = { 0 };
  static float dTerm_bed;
  static float pid_error_bed;
  static float temp_iState_min_bed;
  static float temp_iState_max_bed;
//#else //PIDTEMPBED
#endif //PIDTEMPBED
#if TEMP_SENSOR_BED != 0
  static unsigned long  previous_millis_bed_heater;
#endif
  static unsigned char soft_pwm_bed;
  static unsigned char soft_pwm[EXTRUDERS];
#ifdef FAN_SOFT_PWM
  static unsigned char soft_pwm_fan;
#endif
#if (defined(EXTRUDER_0_AUTO_FAN_PIN) && EXTRUDER_0_AUTO_FAN_PIN > -1) || \
    (defined(EXTRUDER_1_AUTO_FAN_PIN) && EXTRUDER_1_AUTO_FAN_PIN > -1) || \
    (defined(EXTRUDER_2_AUTO_FAN_PIN) && EXTRUDER_2_AUTO_FAN_PIN > -1)
  static unsigned long extruder_autofan_last_check;
#endif

// Init min and max temp with extreme values to prevent false errors during startup
static int minttemp_raw[EXTRUDERS] = ARRAY_BY_EXTRUDERS( HEATER_0_RAW_LO_TEMP , HEATER_1_RAW_LO_TEMP , HEATER_2_RAW_LO_TEMP );
static int maxttemp_raw[EXTRUDERS] = ARRAY_BY_EXTRUDERS( HEATER_0_RAW_HI_TEMP , HEATER_1_RAW_HI_TEMP , HEATER_2_RAW_HI_TEMP );
static int minttemp[EXTRUDERS] = ARRAY_BY_EXTRUDERS( 0, 0, 0 );
static int maxttemp[EXTRUDERS] = ARRAY_BY_EXTRUDERS( 16383, 16383, 16383 );
//static int bed_minttemp_raw = HEATER_BED_RAW_LO_TEMP; /* No bed mintemp error implemented?!? */

static void min_temp_error(uint8_t e);
static void max_temp_error(uint8_t e);

#if (TEMP_SENSOR_BED != 0) && defined(BED_MAXTEMP)
static int bed_maxttemp_raw = HEATER_BED_RAW_HI_TEMP;
#endif

static unsigned long max_heating_start_millis[EXTRUDERS];
static float max_heating_start_temperature[EXTRUDERS];

#ifdef TEMP_SENSOR_1_AS_REDUNDANT
  static void *heater_ttbl_map[2] = {(void *)HEATER_0_TEMPTABLE, (void *)HEATER_1_TEMPTABLE };
  static uint8_t heater_ttbllen_map[2] = { HEATER_0_TEMPTABLE_LEN, HEATER_1_TEMPTABLE_LEN };
#else
  static void *heater_ttbl_map[EXTRUDERS] = ARRAY_BY_EXTRUDERS( (void *)HEATER_0_TEMPTABLE, (void *)HEATER_1_TEMPTABLE, (void *)HEATER_2_TEMPTABLE );
  static uint8_t heater_ttbllen_map[EXTRUDERS] = ARRAY_BY_EXTRUDERS( HEATER_0_TEMPTABLE_LEN, HEATER_1_TEMPTABLE_LEN, HEATER_2_TEMPTABLE_LEN );
#endif

static float analog2temp(int raw, uint8_t e);
#if TEMP_SENSOR_BED != 0
static float analog2tempBed(int raw);
#endif // TEMP_SENSOR_BED
static void updateTemperaturesFromRawValues();

#ifdef WATCH_TEMP_PERIOD
int watch_start_temp[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0,0,0);
unsigned long watchmillis[EXTRUDERS] = ARRAY_BY_EXTRUDERS(0,0,0);
#endif //WATCH_TEMP_PERIOD

#ifndef SOFT_PWM_SCALE
#define SOFT_PWM_SCALE 0
#endif

#ifdef HEATER_0_USES_MAX6675
static int read_max6675();
#endif
//===========================================================================
//=============================   functions      ============================
//===========================================================================

void PID_autotune(float temp, int extruder, int ncycles, autotuneFunc_t pCallback)
{
  float input = 0.0;
  int cycles=0;
  bool heating = true;

  unsigned long temp_millis = millis();
  unsigned long t1=temp_millis;
  unsigned long t2=temp_millis;
  long t_high = 0;
  long t_low = 0;

  long bias, d;
  float Ku, Tu;
  float Kp, Ki, Kd;
  float max = 0, min = 10000;

  // initialize pid coefficients to make the compiler happy...
  Kp = Ki = Kd = 0.0f;

  if ((extruder > EXTRUDERS)
  #if (TEMP_BED_PIN <= -1)
       ||(extruder < 0)
  #endif
       ){
          SERIAL_PROTOCOLLNPGM("PID Autotune failed. Bad extruder number.");
          if (pCallback)
          {
              pCallback(AUTOTUNE_BAD_EXTRUDER, cycles, Kp, Ki, Kd);
          }
          return;
        }

  SERIAL_PROTOCOLLNPGM("PID Autotune start");

  disable_all_heaters(); // switch off all heaters.

  if (extruder<0)
  {
     soft_pwm_bed = (MAX_BED_POWER>>1);
     bias = d = (MAX_BED_POWER>>1);
   }
   else
   {
     soft_pwm[extruder] = (PID_MAX>>1);
     bias = d = (PID_MAX>>1);
  }

 for(;;) {

    if(temp_meas_ready == true) { // temp sample ready
      updateTemperaturesFromRawValues();

#if TEMP_SENSOR_BED != 0
      input = (extruder<0)?current_temperature_bed:current_temperature[extruder];
#else
      input = (extruder<0)?0:current_temperature[extruder];
#endif
      max=max(max,input);
      min=min(min,input);
      if(heating == true && input > temp) {
        if(millis() - t2 > 5000) {
          heating=false;
          if (extruder<0)
            soft_pwm_bed = (bias - d) >> 1;
          else
            soft_pwm[extruder] = (bias - d) >> 1;
          t1=millis();
          t_high=t1 - t2;
          max=temp;
        }
      }
      if(heating == false && input < temp) {
        if(millis() - t1 > 5000) {
          heating=true;
          t2=millis();
          t_low=t2 - t1;
          if(cycles > 0) {
            bias += (d*(t_high - t_low))/(t_low + t_high);
            bias = constrain(bias, 20 ,(extruder<0?(MAX_BED_POWER):(PID_MAX))-20);
            if(bias > (extruder<0?(MAX_BED_POWER):(PID_MAX))/2) d = (extruder<0?(MAX_BED_POWER):(PID_MAX)) - 1 - bias;
            else d = bias;

            SERIAL_PROTOCOLPGM(" bias: "); SERIAL_PROTOCOL(bias);
            SERIAL_PROTOCOLPGM(" d: "); SERIAL_PROTOCOL(d);
            SERIAL_PROTOCOLPGM(" min: "); SERIAL_PROTOCOL(min);
            SERIAL_PROTOCOLPGM(" max: "); SERIAL_PROTOCOLLN(max);
            if(cycles > 2) {
              Ku = (4.0*d)/(3.14159*(max-min)/2.0);
              Tu = ((float)(t_low + t_high)/1000.0);
              SERIAL_PROTOCOLPGM(" Ku: "); SERIAL_PROTOCOL(Ku);
              SERIAL_PROTOCOLPGM(" Tu: "); SERIAL_PROTOCOLLN(Tu);
              Kp = 0.6*Ku;
              Ki = 2*Kp/Tu;
              Kd = Kp*Tu/8;
              SERIAL_PROTOCOLLNPGM(" Classic PID ");
              SERIAL_PROTOCOLPGM(" Kp: "); SERIAL_PROTOCOLLN(Kp);
              SERIAL_PROTOCOLPGM(" Ki: "); SERIAL_PROTOCOLLN(Ki);
              SERIAL_PROTOCOLPGM(" Kd: "); SERIAL_PROTOCOLLN(Kd);
              /*
              Kp = 0.33*Ku;
              Ki = Kp/Tu;
              Kd = Kp*Tu/3;
              SERIAL_PROTOCOLLNPGM(" Some overshoot ")
              SERIAL_PROTOCOLPGM(" Kp: "); SERIAL_PROTOCOLLN(Kp);
              SERIAL_PROTOCOLPGM(" Ki: "); SERIAL_PROTOCOLLN(Ki);
              SERIAL_PROTOCOLPGM(" Kd: "); SERIAL_PROTOCOLLN(Kd);
              Kp = 0.2*Ku;
              Ki = 2*Kp/Tu;
              Kd = Kp*Tu/3;
              SERIAL_PROTOCOLLNPGM(" No overshoot ")
              SERIAL_PROTOCOLPGM(" Kp: "); SERIAL_PROTOCOLLN(Kp);
              SERIAL_PROTOCOLPGM(" Ki: "); SERIAL_PROTOCOLLN(Ki);
              SERIAL_PROTOCOLPGM(" Kd: "); SERIAL_PROTOCOLLN(Kd);
              */
            }
          }
          if (extruder<0)
            soft_pwm_bed = (bias + d) >> 1;
          else
            soft_pwm[extruder] = (bias + d) >> 1;
          cycles++;
          min=temp;
        }
      }
    }
    if(input > (temp + 20)) {
      SERIAL_PROTOCOLLNPGM("PID Autotune failed! Temperature too high");
      if (pCallback)
      {
          pCallback(AUTOTUNE_TEMP_HIGH, cycles, Kp, Ki, Kd);
      }
      return;
    }
    if(millis() - temp_millis > 2000) {
      int p;
      if (extruder<0){
        p=soft_pwm_bed;
        SERIAL_PROTOCOLPGM("ok B:");
      }else{
        p=soft_pwm[extruder];
        SERIAL_PROTOCOLPGM("ok T:");
      }

      SERIAL_PROTOCOL(input);
      SERIAL_PROTOCOLPGM(" @:");
      SERIAL_PROTOCOLLN(p);

      temp_millis = millis();
    }
    if(((millis() - t1) + (millis() - t2)) > (10L*60L*1000L*2L)) {
      SERIAL_PROTOCOLLNPGM("PID Autotune failed! timeout");
      if (pCallback)
      {
          pCallback(AUTOTUNE_TIMEOUT, cycles, Kp, Ki, Kd);
      }
      return;
    }
    if(cycles > ncycles) {
      SERIAL_PROTOCOLLNPGM("PID Autotune finished! Put the Kp, Ki and Kd constants into Configuration.h");
      if (pCallback)
      {
          pCallback(AUTOTUNE_OK, cycles, Kp, Ki, Kd);
      }
      return;
    }
    lcd_update();
    lifetime_stats_tick();
    if (pCallback)
    {
        if (!pCallback(0, cycles, Kp, Ki, Kd))
        {
            // aborted by user
            SERIAL_PROTOCOLLNPGM("PID Autotune aborted.");
            break;
        }
    }
  }
}

void updatePID()
{
#ifdef PIDTEMP
  for(int e = 0; e < EXTRUDERS; ++e) {
     temp_iState_max[e] = PID_INTEGRAL_DRIVE_MAX / Ki;
  }
#endif
#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
  if (pidTempBed())
  {
    temp_iState_max_bed = PID_INTEGRAL_DRIVE_MAX / bedKi;
  }
#endif
}

int getHeaterPower(int heater)
{
	if (heater<0)
		return soft_pwm_bed;
  return soft_pwm[heater];
}

#if (defined(EXTRUDER_0_AUTO_FAN_PIN) && EXTRUDER_0_AUTO_FAN_PIN > -1) || \
    (defined(EXTRUDER_1_AUTO_FAN_PIN) && EXTRUDER_1_AUTO_FAN_PIN > -1) || \
    (defined(EXTRUDER_2_AUTO_FAN_PIN) && EXTRUDER_2_AUTO_FAN_PIN > -1)

  #if defined(FAN_PIN) && FAN_PIN > -1
    #if EXTRUDER_0_AUTO_FAN_PIN == FAN_PIN
       #error "You cannot set EXTRUDER_0_AUTO_FAN_PIN equal to FAN_PIN"
    #endif
    #if EXTRUDER_1_AUTO_FAN_PIN == FAN_PIN
       #error "You cannot set EXTRUDER_1_AUTO_FAN_PIN equal to FAN_PIN"
    #endif
    #if EXTRUDER_2_AUTO_FAN_PIN == FAN_PIN
       #error "You cannot set EXTRUDER_2_AUTO_FAN_PIN equal to FAN_PIN"
    #endif
  #endif

void setExtruderAutoFanState(int pin, bool state)
{
  unsigned char newFanSpeed = (state != 0) ? EXTRUDER_AUTO_FAN_SPEED : 0;
  // this idiom allows both digital and PWM fan outputs (see M42 handling).
  pinMode(pin, OUTPUT);
  digitalWrite(pin, newFanSpeed);
  analogWrite(pin, newFanSpeed);
}

void checkExtruderAutoFans()
{
  uint8_t fanState = 0;

  // which fan pins need to be turned on?
  #if defined(EXTRUDER_0_AUTO_FAN_PIN) && EXTRUDER_0_AUTO_FAN_PIN > -1
    if (current_temperature[0] > EXTRUDER_AUTO_FAN_TEMPERATURE)
      fanState |= 1;
  #endif
  #if defined(EXTRUDER_1_AUTO_FAN_PIN) && EXTRUDER_1_AUTO_FAN_PIN > -1 && EXTRUDERS > 1
    if (current_temperature[1] > EXTRUDER_AUTO_FAN_TEMPERATURE)
    {
      if (EXTRUDER_1_AUTO_FAN_PIN == EXTRUDER_0_AUTO_FAN_PIN)
        fanState |= 1;
      else
        fanState |= 2;
    }
  #endif
  #if defined(EXTRUDER_2_AUTO_FAN_PIN) && EXTRUDER_2_AUTO_FAN_PIN > -1 && EXTRUDERS > 2
    if (current_temperature[2] > EXTRUDER_AUTO_FAN_TEMPERATURE)
    {
      if (EXTRUDER_2_AUTO_FAN_PIN == EXTRUDER_0_AUTO_FAN_PIN)
        fanState |= 1;
      else if (EXTRUDER_2_AUTO_FAN_PIN == EXTRUDER_1_AUTO_FAN_PIN)
        fanState |= 2;
      else
        fanState |= 4;
    }
  #endif

  // update extruder auto fan states
  #if defined(EXTRUDER_0_AUTO_FAN_PIN) && EXTRUDER_0_AUTO_FAN_PIN > -1
    setExtruderAutoFanState(EXTRUDER_0_AUTO_FAN_PIN, (fanState & 1) != 0);
  #endif
  #if defined(EXTRUDER_1_AUTO_FAN_PIN) && EXTRUDER_1_AUTO_FAN_PIN > -1 && EXTRUDERS > 1
    if (EXTRUDER_1_AUTO_FAN_PIN != EXTRUDER_0_AUTO_FAN_PIN)
      setExtruderAutoFanState(EXTRUDER_1_AUTO_FAN_PIN, (fanState & 2) != 0);
  #endif
  #if defined(EXTRUDER_2_AUTO_FAN_PIN) && EXTRUDER_2_AUTO_FAN_PIN > -1 && EXTRUDERS > 2
    if (EXTRUDER_2_AUTO_FAN_PIN != EXTRUDER_0_AUTO_FAN_PIN
        && EXTRUDER_2_AUTO_FAN_PIN != EXTRUDER_1_AUTO_FAN_PIN)
      setExtruderAutoFanState(EXTRUDER_2_AUTO_FAN_PIN, (fanState & 4) != 0);
  #endif
}

#endif // any extruder auto fan pins set

static unsigned char limit_power(uint16_t wattage, unsigned char pwm, uint16_t &budget)
{
    if (pwm)
    {
        if (budget && wattage)
        {
            uint16_t power = (float(pwm) / 0x7f) * wattage;
            if (power > budget)
            {
                pwm = (float(budget) / wattage) * 0x7f;
                budget = 0;
            }
            else
            {
                budget -= power;
            }
        }
        else
        {
            pwm = 0;
        }
    }
    return pwm;
}

void manage_heater()
{
  if(temp_meas_ready != true)   //better readability
    return;

  updateTemperaturesFromRawValues();

  #ifdef HEATER_0_USES_MAX6675
  if (current_temperature[0] > 1023 || current_temperature[0] > maxttemp[0])
  {
	  max_temp_error(0);
  }
  if (current_temperature[0] == 0  || current_temperature[0] < minttemp[0])
  {
	  min_temp_error(0);
  }
  #endif

  uint16_t budget = power_budget;
  {
      // reduce power budget on axis activity
      if (block_buffer_head != block_buffer_tail)
      {
          block_t *block = &block_buffer[block_buffer_tail];
          uint16_t budget_part = constrain(8, 0, power_budget >> 4);
          uint8_t  counter = 0x01;
          if(block->steps_x != 0) counter <<= 0x01;
          if(block->steps_y != 0) counter <<= 0x01;
          if(block->steps_z != 0) counter <<= 0x01;
          if(block->steps_e != 0) counter <<= 0x01;
          while (counter >>= 1) budget -= budget_part;
      }
  }

  float pid_input;
  float pid_output;
  int target_temp;

  for(uint8_t e = 0; e < EXTRUDERS; ++e)
  {
    target_temp = (printing_state == PRINT_STATE_RECOVER) ? recover_temperature[e] : target_temperature[e];
  #ifdef PIDTEMP
    pid_input = current_temperature[e];

    #ifndef PID_OPENLOOP
        pid_error[e] = target_temp - pid_input;
        if(pid_error[e] > PID_FUNCTIONAL_RANGE) {
          pid_output = BANG_MAX;
          pid_reset[e] = true;
        }
        else if(pid_error[e] < -PID_FUNCTIONAL_RANGE || target_temp == 0) {
          pid_output = 0;
          pid_reset[e] = true;
        }
        else {
          if(pid_reset[e] == true) {
            temp_iState[e] = 0.0;
            pid_reset[e] = false;
          }
          #if EXTRUDERS > 1
            pTerm[e] = (e ? pid2[0] : Kp) * pid_error[e];
          #else
            pTerm[e] = Kp * pid_error[e];
          #endif
          temp_iState[e] += pid_error[e];
          temp_iState[e] = constrain(temp_iState[e], temp_iState_min[e], temp_iState_max[e]);
          #if EXTRUDERS > 1
            iTerm[e] = (e ? pid2[1] : Ki) * temp_iState[e];
          #else
            iTerm[e] = Ki * temp_iState[e];
          #endif

          //K1 defined in Configuration.h in the PID settings
          #define K2 (1.0-K1)
          #if EXTRUDERS > 1
            dTerm[e] = ((e ? pid2[2] : Kd) * (pid_input - temp_dState[e]))*K2 + (K1 * dTerm[e]);
          #else
            dTerm[e] = (Kd * (pid_input - temp_dState[e]))*K2 + (K1 * dTerm[e]);
          #endif
          pid_output = constrain(pTerm[e] + iTerm[e] - dTerm[e], 0, PID_MAX);
        }
        temp_dState[e] = pid_input;
    #else
        pid_output = constrain(target_temp, 0, PID_MAX);
    #endif //PID_OPENLOOP
    #ifdef PID_DEBUG
    SERIAL_ECHO_START;
    SERIAL_ECHOPGM(" PIDDEBUG ");
    SERIAL_ECHO(e);
    SERIAL_ECHOPGM(": Input ");
    SERIAL_ECHO(pid_input);
    SERIAL_ECHOPGM(" Output ");
    SERIAL_ECHO(pid_output);
    SERIAL_ECHOPGM(" pTerm ");
    SERIAL_ECHO(pTerm[e]);
    SERIAL_ECHOPGM(" iTerm ");
    SERIAL_ECHO(iTerm[e]);
    SERIAL_ECHOPGM(" dTerm ");
    SERIAL_ECHOLN(dTerm[e]);
    #endif //PID_DEBUG
  #else /* PID off */
    pid_output = 0;
    if(current_temperature[e] < target_temp) {
      pid_output = PID_MAX;
    }
  #endif

    // Check if temperature is within the correct range
    if((current_temperature[e] > minttemp[e]) && (current_temperature[e] < maxttemp[e]))
    {
      soft_pwm[e] = limit_power(power_extruder[e], (int)pid_output >> 1, budget);
    }
    else {
      soft_pwm[e] = 0;
    }

    #ifdef WATCH_TEMP_PERIOD
    if(watchmillis[e] && millis() - watchmillis[e] > WATCH_TEMP_PERIOD)
    {
        if(degHotend(e) < watch_start_temp[e] + WATCH_TEMP_INCREASE)
        {
            cooldownHotend(e);
            LCD_MESSAGEPGM("Heating failed");
            SERIAL_ECHO_START;
            SERIAL_ECHOLNPGM("Heating failed");
        }else{
            watchmillis[e] = 0;
        }
    }
    #endif
    #ifdef TEMP_SENSOR_1_AS_REDUNDANT
      if(fabs(current_temperature[0] - redundant_temperature) > MAX_REDUNDANT_TEMP_SENSOR_DIFF) {
        disable_all_heaters();
        if(IsStopped() == false) {
          SERIAL_ERROR_START;
          SERIAL_ERRORLNPGM("Extruder switched off. Temperature difference between temp sensors is too high !");
          LCD_ALERTMESSAGEPGM("Err: REDUNDANT TEMP ERROR");
        }
        #ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
          Stop();
        #endif
      }
    #endif
    if ((heater_check_time) && (soft_pwm[e] == (PID_MAX >> 1)))
    {
        if (current_temperature[e] - max_heating_start_temperature[e] > heater_check_temp)
        {
            max_heating_start_millis[e] = 0;
        }
        if (max_heating_start_millis[e] == 0)
        {
            max_heating_start_millis[e] = millis();
            max_heating_start_temperature[e] = current_temperature[e];
        }
        if (millis() > max_heating_start_millis[e] + heater_check_time*1000UL)
        {
            //Did not heat up MAX_HEATING_TEMPERATURE_INCREASE in MAX_HEATING_CHECK_MILLIS while the PID was at the maximum.
            //Potential problems could be that the heater is not working, or the temperature sensor is not measuring what the heater is heating.
            disable_all_heaters();
            Stop(STOP_REASON_HEATER_ERROR);
        }
    }else{
        max_heating_start_millis[e] = 0;
    }
  } // End extruder for loop

  #if (defined(EXTRUDER_0_AUTO_FAN_PIN) && EXTRUDER_0_AUTO_FAN_PIN > -1) || \
      (defined(EXTRUDER_1_AUTO_FAN_PIN) && EXTRUDER_1_AUTO_FAN_PIN > -1) || \
      (defined(EXTRUDER_2_AUTO_FAN_PIN) && EXTRUDER_2_AUTO_FAN_PIN > -1)
  if(millis() - extruder_autofan_last_check > 2500)  // only need to check fan state very infrequently
  {
    checkExtruderAutoFans();
    extruder_autofan_last_check = millis();
  }
  #endif

  {
    //For the UM2 the head fan is connected to PJ6, which does not have an Arduino PIN definition. So use direct register access.
    DDRJ |= _BV(6);
    if (current_temperature[0] > EXTRUDER_AUTO_FAN_TEMPERATURE
#if EXTRUDERS > 1
        || current_temperature[1] > EXTRUDER_AUTO_FAN_TEMPERATURE
#endif
#if EXTRUDERS > 2
        || current_temperature[2] > EXTRUDER_AUTO_FAN_TEMPERATURE
#endif
        )
    {
        PORTJ |= _BV(6);
    }else{
        PORTJ &=~_BV(6);
    }
  }

  #if TEMP_SENSOR_BED != 0

  if (!pidTempBed())
  {
    if(millis() - previous_millis_bed_heater < BED_CHECK_INTERVAL)
    {
      soft_pwm_bed = soft_pwm_bed ? limit_power(power_buildplate, MAX_BED_POWER>>1, budget) : 0;
      return;
    }
    previous_millis_bed_heater = millis();
  }

  #ifdef PIDTEMPBED
  if (pidTempBed())
  {
    pid_input = current_temperature_bed;

    #ifndef PID_OPENLOOP
		  pid_error_bed = target_temperature_bed - pid_input;
		  pTerm_bed = bedKp * pid_error_bed;
		  temp_iState_bed += pid_error_bed;
		  temp_iState_bed = constrain(temp_iState_bed, temp_iState_min_bed, temp_iState_max_bed);
		  iTerm_bed = bedKi * temp_iState_bed;

		  //K1 defined in Configuration.h in the PID settings
		  #define K2 (1.0-K1)
		  dTerm_bed= (bedKd * (pid_input - temp_dState_bed))*K2 + (K1 * dTerm_bed);
		  temp_dState_bed = pid_input;

		  pid_output = constrain(pTerm_bed + iTerm_bed - dTerm_bed, 0, MAX_BED_POWER);

    #else
      pid_output = constrain(target_temperature_bed, 0, MAX_BED_POWER);
    #endif //PID_OPENLOOP

	  if((current_temperature_bed > BED_MINTEMP) && (current_temperature_bed < BED_MAXTEMP))
	  {
	    soft_pwm_bed = limit_power(power_buildplate, (int)pid_output >> 1, budget);
	  }
	  else {
	    soft_pwm_bed = 0;
	  }
  }
  else // printbed bang-bang mode
  #endif//!PIDTEMPBED
  {
    #if !defined(BED_LIMIT_SWITCHING)
      // Check if temperature is within the correct range
      if((current_temperature_bed > BED_MINTEMP) && (current_temperature_bed < BED_MAXTEMP))
      {
        if(current_temperature_bed >= target_temperature_bed)
        {
          soft_pwm_bed = 0;
        }
        else
        {
          soft_pwm_bed = limit_power(power_buildplate, MAX_BED_POWER>>1, budget);
        }
      }
      else
      {
        soft_pwm_bed = 0;
        WRITE(HEATER_BED_PIN,LOW);
      }
    #else //#ifdef BED_LIMIT_SWITCHING
      // Check if temperature is within the correct band
      if((current_temperature_bed > BED_MINTEMP) && (current_temperature_bed < BED_MAXTEMP))
      {
        if(current_temperature_bed > target_temperature_bed + BED_HYSTERESIS)
        {
          soft_pwm_bed = 0;
        }
        else if(current_temperature_bed <= target_temperature_bed - BED_HYSTERESIS)
        {
          soft_pwm_bed = limit_power(power_buildplate, MAX_BED_POWER>>1, budget);
        }
      }
      else
      {
        soft_pwm_bed = 0;
        WRITE(HEATER_BED_PIN,LOW);
      }
    #endif
  }
  #endif
}

#define PGM_RD_W(x)   (short)pgm_read_word(&x)
// Derived from RepRap FiveD extruder::getTemperature()
// For hot end temperature measurement.
static float analog2temp(int raw, uint8_t e) {
#ifdef TEMP_SENSOR_1_AS_REDUNDANT
  if(e > EXTRUDERS)
#else
  if(e >= EXTRUDERS)
#endif
  {
      SERIAL_ERROR_START;
      SERIAL_ERROR((int)e);
      SERIAL_ERRORLNPGM(" - Invalid extruder number !");
      kill();
  }
  #ifdef HEATER_0_USES_MAX6675
    if (e == 0)
    {
      return 0.25 * raw;
    }
  #endif

  if(heater_ttbl_map[e] != NULL)
  {
    float celsius = 0;
    uint8_t i;
    short (*tt)[][2] = (short (*)[][2])(heater_ttbl_map[e]);

    for (i=1; i<heater_ttbllen_map[e]; i++)
    {
      if (PGM_RD_W((*tt)[i][0]) > raw)
      {
        celsius = PGM_RD_W((*tt)[i-1][1]) +
          (raw - PGM_RD_W((*tt)[i-1][0])) *
          (float)(PGM_RD_W((*tt)[i][1]) - PGM_RD_W((*tt)[i-1][1])) /
          (float)(PGM_RD_W((*tt)[i][0]) - PGM_RD_W((*tt)[i-1][0]));
        break;
      }
    }

    // Overflow: Set to last value in the table
    if (i == heater_ttbllen_map[e]) celsius = PGM_RD_W((*tt)[i-1][1]);

    return celsius;
  }
  return ((raw * ((5.0 * 100.0) / 1024.0) / OVERSAMPLENR) * TEMP_SENSOR_AD595_GAIN) + TEMP_SENSOR_AD595_OFFSET;
}

#if TEMP_SENSOR_BED != 0
// Derived from RepRap FiveD extruder::getTemperature()
// For bed temperature measurement.
static float analog2tempBed(int raw) {
  #ifdef BED_USES_THERMISTOR
    float celsius = 0;
    byte i;

    for (i=1; i<BEDTEMPTABLE_LEN; i++)
    {
      if (PGM_RD_W(BEDTEMPTABLE[i][0]) > raw)
      {
        celsius  = PGM_RD_W(BEDTEMPTABLE[i-1][1]) +
          (raw - PGM_RD_W(BEDTEMPTABLE[i-1][0])) *
          (float)(PGM_RD_W(BEDTEMPTABLE[i][1]) - PGM_RD_W(BEDTEMPTABLE[i-1][1])) /
          (float)(PGM_RD_W(BEDTEMPTABLE[i][0]) - PGM_RD_W(BEDTEMPTABLE[i-1][0]));
        break;
      }
    }

    // Overflow: Set to last value in the table
    if (i == BEDTEMPTABLE_LEN) celsius = PGM_RD_W(BEDTEMPTABLE[i-1][1]);

    return celsius;
  #elif defined BED_USES_AD595
    return ((raw * ((5.0 * 100.0) / 1024.0) / OVERSAMPLENR) * TEMP_SENSOR_AD595_GAIN) + TEMP_SENSOR_AD595_OFFSET;
  #else
    return 0;
  #endif
}
#endif // TEMP_SENSOR_BED

/* Called to get the raw values into the the actual temperatures. The raw values are created in interrupt context,
    and this function is called from normal context as it is too slow to run in interrupts and will block the stepper routine otherwise */
static void updateTemperaturesFromRawValues()
{
#ifdef HEATER_0_USES_MAX6675
	current_temperature_raw[0] = read_max6675();
#endif

    for(uint8_t e=0;e<EXTRUDERS;++e)
    {
        current_temperature[e] = analog2temp(current_temperature_raw[e], e);
    }
    #if TEMP_SENSOR_BED != 0
      current_temperature_bed = analog2tempBed(current_temperature_bed_raw);
    #endif
    #ifdef TEMP_SENSOR_1_AS_REDUNDANT
      redundant_temperature = analog2temp(redundant_temperature_raw, 1);
    #endif
    //Reset the watchdog after we know we have a temperature measurement.
    watchdog_reset();

    CRITICAL_SECTION_START;
    temp_meas_ready = false;
    CRITICAL_SECTION_END;
}

void tp_init()
{
#if (MOTHERBOARD == 80) && ((TEMP_SENSOR_0==-1)||(TEMP_SENSOR_1==-1)||(TEMP_SENSOR_2==-1)||(TEMP_SENSOR_BED==-1))
  //disable RUMBA JTAG in case the thermocouple extension is plugged on top of JTAG connector
  MCUCR=(1<<JTD);
  MCUCR=(1<<JTD);
#endif

  // Finish init of mult extruder arrays
  for(uint8_t e = 0; e < EXTRUDERS; ++e) {
    // populate with the first value
    maxttemp[e] = maxttemp[0];
#ifdef PIDTEMP
    temp_iState_min[e] = 0.0;
    temp_iState_max[e] = PID_INTEGRAL_DRIVE_MAX / Ki;
#endif //PIDTEMP
  }
#if defined(PIDTEMPBED) && (TEMP_SENSOR_BED != 0)
  temp_iState_min_bed = 0.0;
  temp_iState_max_bed = PID_INTEGRAL_DRIVE_MAX / bedKi;
#endif //PIDTEMPBED

  #if defined(HEATER_0_PIN) && (HEATER_0_PIN > -1)
    SET_OUTPUT(HEATER_0_PIN);
  #endif
  #if defined(HEATER_1_PIN) && (HEATER_1_PIN > -1)
    SET_OUTPUT(HEATER_1_PIN);
  #endif
  #if defined(HEATER_2_PIN) && (HEATER_2_PIN > -1)
    SET_OUTPUT(HEATER_2_PIN);
  #endif
  #if defined(HEATER_BED_PIN) && (HEATER_BED_PIN > -1)
    SET_OUTPUT(HEATER_BED_PIN);
  #endif
  #if defined(FAN_PIN) && (FAN_PIN > -1)
    SET_OUTPUT(FAN_PIN);
    #ifdef FAST_PWM_FAN
    setPwmFrequency(FAN_PIN, 1); // No prescaling. Pwm frequency = F_CPU/256/8
    #endif
    #ifdef FAN_SOFT_PWM
    soft_pwm_fan = fanSpeedSoftPwm / 2;
    #endif
  #endif

  #ifdef HEATER_0_USES_MAX6675
    #ifndef SDSUPPORT
      SET_OUTPUT(MAX_SCK_PIN);
      WRITE(MAX_SCK_PIN,0);

      SET_OUTPUT(MAX_MOSI_PIN);
      WRITE(MAX_MOSI_PIN,1);

      SET_INPUT(MAX_MISO_PIN);
      WRITE(MAX_MISO_PIN,1);
    #else
	  pinMode(SS_PIN, OUTPUT);
	  digitalWrite(SS_PIN, HIGH);
    #endif

    SET_OUTPUT(MAX6675_SS);
    WRITE(MAX6675_SS,1);
  #endif

  // Set analog inputs
  ADCSRA = 1<<ADEN | 1<<ADSC | 1<<ADIF | 0x07;
  // Disable the digital inputs that are shared with the ADC. This will reduce current consumption.
  DIDR0 = 0;
  #ifdef DIDR2
    DIDR2 = 0;
  #endif
  #if defined(TEMP_0_PIN) && (TEMP_0_PIN > -1)
    #if TEMP_0_PIN < 8
       DIDR0 |= 1 << TEMP_0_PIN;
    #else
       DIDR2 |= 1<<(TEMP_0_PIN - 8);
    #endif
  #endif
  #if defined(TEMP_1_PIN) && (TEMP_1_PIN > -1)
    #if TEMP_1_PIN < 8
       DIDR0 |= 1<<TEMP_1_PIN;
    #else
       DIDR2 |= 1<<(TEMP_1_PIN - 8);
    #endif
  #endif
  #if defined(TEMP_2_PIN) && (TEMP_2_PIN > -1)
    #if TEMP_2_PIN < 8
       DIDR0 |= 1 << TEMP_2_PIN;
    #else
       DIDR2 |= 1<<(TEMP_2_PIN - 8);
    #endif
  #endif
  #if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1)
    #if TEMP_BED_PIN < 8
       DIDR0 |= 1<<TEMP_BED_PIN;
    #else
       DIDR2 |= 1<<(TEMP_BED_PIN - 8);
    #endif
  #endif

  // Use timer0 for temperature measurement
  // Interleave temperature interrupt with millies interrupt
  OCR0B = 128;
  TIMSK0 |= (1<<OCIE0B);

#ifdef HEATER_0_MINTEMP
  minttemp[0] = HEATER_0_MINTEMP;
  while(analog2temp(minttemp_raw[0], 0) < HEATER_0_MINTEMP) {
#if HEATER_0_RAW_LO_TEMP < HEATER_0_RAW_HI_TEMP
    minttemp_raw[0] += OVERSAMPLENR;
#else
    minttemp_raw[0] -= OVERSAMPLENR;
#endif
  }
#endif //MINTEMP
#ifdef HEATER_0_MAXTEMP
  maxttemp[0] = HEATER_0_MAXTEMP;
  while(analog2temp(maxttemp_raw[0], 0) > HEATER_0_MAXTEMP) {
#if HEATER_0_RAW_LO_TEMP < HEATER_0_RAW_HI_TEMP
    maxttemp_raw[0] -= OVERSAMPLENR;
#else
    maxttemp_raw[0] += OVERSAMPLENR;
#endif
  }
#endif //MAXTEMP

#if (EXTRUDERS > 1) && defined(HEATER_1_MINTEMP)
  minttemp[1] = HEATER_1_MINTEMP;
  while(analog2temp(minttemp_raw[1], 1) < HEATER_1_MINTEMP) {
#if HEATER_1_RAW_LO_TEMP < HEATER_1_RAW_HI_TEMP
    minttemp_raw[1] += OVERSAMPLENR;
#else
    minttemp_raw[1] -= OVERSAMPLENR;
#endif
  }
#endif // MINTEMP 1
#if (EXTRUDERS > 1) && defined(HEATER_1_MAXTEMP)
  maxttemp[1] = HEATER_1_MAXTEMP;
  while(analog2temp(maxttemp_raw[1], 1) > HEATER_1_MAXTEMP) {
#if HEATER_1_RAW_LO_TEMP < HEATER_1_RAW_HI_TEMP
    maxttemp_raw[1] -= OVERSAMPLENR;
#else
    maxttemp_raw[1] += OVERSAMPLENR;
#endif
  }
#endif //MAXTEMP 1

#if (EXTRUDERS > 2) && defined(HEATER_2_MINTEMP)
  minttemp[2] = HEATER_2_MINTEMP;
  while(analog2temp(minttemp_raw[2], 2) < HEATER_2_MINTEMP) {
#if HEATER_2_RAW_LO_TEMP < HEATER_2_RAW_HI_TEMP
    minttemp_raw[2] += OVERSAMPLENR;
#else
    minttemp_raw[2] -= OVERSAMPLENR;
#endif
  }
#endif //MINTEMP 2
#if (EXTRUDERS > 2) && defined(HEATER_2_MAXTEMP)
  maxttemp[2] = HEATER_2_MAXTEMP;
  while(analog2temp(maxttemp_raw[2], 2) > HEATER_2_MAXTEMP) {
#if HEATER_2_RAW_LO_TEMP < HEATER_2_RAW_HI_TEMP
    maxttemp_raw[2] -= OVERSAMPLENR;
#else
    maxttemp_raw[2] += OVERSAMPLENR;
#endif
  }
#endif //MAXTEMP 2

#if (TEMP_SENSOR_BED != 0) && defined(BED_MINTEMP)
  /* No bed MINTEMP error implemented?!? */ /*
  while(analog2tempBed(bed_minttemp_raw) < BED_MINTEMP) {
#if HEATER_BED_RAW_LO_TEMP < HEATER_BED_RAW_HI_TEMP
    bed_minttemp_raw += OVERSAMPLENR;
#else
    bed_minttemp_raw -= OVERSAMPLENR;
#endif
  }
  */
#endif //BED_MINTEMP
#if (TEMP_SENSOR_BED != 0) && defined(BED_MAXTEMP)
  while(analog2tempBed(bed_maxttemp_raw) > BED_MAXTEMP) {
#if HEATER_BED_RAW_LO_TEMP < HEATER_BED_RAW_HI_TEMP
    bed_maxttemp_raw -= OVERSAMPLENR;
#else
    bed_maxttemp_raw += OVERSAMPLENR;
#endif
  }
#endif //BED_MAXTEMP
}

void setWatch()
{
#ifdef WATCH_TEMP_PERIOD
  for (int e = 0; e < EXTRUDERS; e++)
  {
    if(degHotend(e) < degTargetHotend(e) - (WATCH_TEMP_INCREASE * 2))
    {
      watch_start_temp[e] = degHotend(e);
      watchmillis[e] = millis();
    }
  }
#endif
}


void disable_all_heaters()
{
  for(uint8_t i=0; i<EXTRUDERS; ++i)
    cooldownHotend(i);
  #if TEMP_SENSOR_BED != 0
    setTargetBed(0);
  #endif
  #if defined(TEMP_0_PIN) && TEMP_0_PIN > -1
  target_temperature[0]=0;
  soft_pwm[0]=0;
   #if defined(HEATER_0_PIN) && HEATER_0_PIN > -1
     WRITE(HEATER_0_PIN,LOW);
   #endif
  #endif

  #if defined(TEMP_1_PIN) && TEMP_1_PIN > -1 && EXTRUDERS > 1
    target_temperature[1]=0;
    soft_pwm[1]=0;
    #if defined(HEATER_1_PIN) && HEATER_1_PIN > -1
      WRITE(HEATER_1_PIN,LOW);
    #endif
  #endif

  #if defined(TEMP_2_PIN) && TEMP_2_PIN > -1 && EXTRUDERS > 2
    target_temperature[2]=0;
    soft_pwm[2]=0;
    #if defined(HEATER_2_PIN) && HEATER_2_PIN > -1
      WRITE(HEATER_2_PIN,LOW);
    #endif
  #endif

  #if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1) && (TEMP_SENSOR_BED != 0)
    target_temperature_bed=0;
    soft_pwm_bed=0;
    #ifdef PIDTEMPBED
    pTerm_bed = 0;
    iTerm_bed = 0;
    #endif
    #if defined(HEATER_BED_PIN) && HEATER_BED_PIN > -1
      WRITE(HEATER_BED_PIN,LOW);
    #endif
  #endif
}

void max_temp_error(uint8_t e)
{
  disable_all_heaters();
  if(IsStopped() == false) {
    SERIAL_ERROR_START;
    SERIAL_ERRORLN((int)e);
    SERIAL_ERRORLNPGM(": Extruder switched off. MAXTEMP triggered !");
    LCD_ALERTMESSAGEPGM("Err: MAXTEMP");
  }
  #ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
  Stop(STOP_REASON_MAXTEMP);
  #endif
}

void min_temp_error(uint8_t e)
{
  disable_all_heaters();
  if(IsStopped() == false) {
    SERIAL_ERROR_START;
    SERIAL_ERRORLN((int)e);
    SERIAL_ERRORLNPGM(": Extruder switched off. MINTEMP triggered !");
    LCD_ALERTMESSAGEPGM("Err: MINTEMP");
  }
  #ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
  Stop(STOP_REASON_MINTEMP);
  #endif
}

void bed_max_temp_error(void)
{
#if HEATER_BED_PIN > -1
  WRITE(HEATER_BED_PIN, 0);
#endif
  if(IsStopped() == false) {
    SERIAL_ERROR_START;
    SERIAL_ERRORLNPGM("Temperature heated bed switched off. MAXTEMP triggered !!");
    LCD_ALERTMESSAGEPGM("Err: MAXTEMP BED");
  }
  #ifndef BOGUS_TEMPERATURE_FAILSAFE_OVERRIDE
  Stop(STOP_REASON_MAXTEMP_BED);
  #endif
}

#ifdef HEATER_0_USES_MAX6675
#define MAX6675_HEAT_INTERVAL 250
long max6675_previous_millis = -MAX6675_HEAT_INTERVAL;
int max6675_temp = 4000;

static int read_max6675()
{
  if (millis() - max6675_previous_millis < MAX6675_HEAT_INTERVAL)
    return max6675_temp;

  max6675_previous_millis = millis();
  max6675_temp = 0;

  #ifdef	PRR
    PRR &= ~(1<<PRSPI);
  #elif defined PRR0
    PRR0 &= ~(1<<PRSPI);
  #endif

  SPCR = (1<<MSTR) | (1<<SPE) | (1<<SPR0);

  // enable TT_MAX6675
  WRITE(MAX6675_SS, 0);

  // ensure 100ns delay - a bit extra is fine
  asm("nop");//50ns on 20Mhz, 62.5ns on 16Mhz
  asm("nop");//50ns on 20Mhz, 62.5ns on 16Mhz

  // read MSB
  SPDR = 0;
  for (;(SPSR & (1<<SPIF)) == 0;);
  max6675_temp = SPDR;
  max6675_temp <<= 8;

  // read LSB
  SPDR = 0;
  for (;(SPSR & (1<<SPIF)) == 0;);
  max6675_temp |= SPDR;

  // disable TT_MAX6675
  WRITE(MAX6675_SS, 1);


  if (max6675_temp & 4)
  {
    // thermocouple open
    max6675_temp = 4000;
  }
  else
  {
    max6675_temp = max6675_temp >> 3;
  }

  return max6675_temp;
}
#endif

// Timer 0 is shared with millies
ISR(TIMER0_COMPB_vect)
{
  //these variables are only accesible from the ISR, but static, so they don't lose their value
  static unsigned char temp_count = 0;
  static unsigned long raw_temp_0_value = 0;
#if EXTRUDERS > 1
  static unsigned long raw_temp_1_value = 0;
#endif
#if EXTRUDERS > 2
  static unsigned long raw_temp_2_value = 0;
#endif
#if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1)
  static unsigned long raw_temp_bed_value = 0;
#endif
  static unsigned char temp_state = 5;
  static unsigned char pwm_count = (1 << SOFT_PWM_SCALE);
  static unsigned char soft_pwm_0;
  #if EXTRUDERS > 1
  static unsigned char soft_pwm_1;
  #endif
  #if EXTRUDERS > 2
  static unsigned char soft_pwm_2;
  #endif
  #if HEATER_BED_PIN > -1
  static unsigned char soft_pwm_b;
  #endif

  if (pwm_count == 0)
  {
    soft_pwm_0 = soft_pwm[0];
    if (soft_pwm_0 > 0)
    {
      WRITE(HEATER_0_PIN, 1);
    }
    #if EXTRUDERS > 1
    soft_pwm_1 = soft_pwm[1];
    if (soft_pwm_1 > 0)
    {
      WRITE(HEATER_1_PIN, 1);
    }
    #endif
    #if EXTRUDERS > 2
    soft_pwm_2 = soft_pwm[2];
    if (soft_pwm_2 > 0)
    {
      WRITE(HEATER_2_PIN, 1);
    }
    #endif
    #if defined(HEATER_BED_PIN) && HEATER_BED_PIN > -1
    soft_pwm_b = soft_pwm_bed;
    #endif
    #ifdef FAN_SOFT_PWM
    soft_pwm_fan = fanSpeedSoftPwm >> 1;
    if(soft_pwm_fan > 0) WRITE(FAN_PIN,1);
    #endif
  }
  if(soft_pwm_0 <= pwm_count) WRITE(HEATER_0_PIN,0);
  #if EXTRUDERS > 1
  if(soft_pwm_1 <= pwm_count) WRITE(HEATER_1_PIN,0);
  #endif
  #if EXTRUDERS > 2
  if(soft_pwm_2 <= pwm_count) WRITE(HEATER_2_PIN,0);
  #endif
  #if defined(HEATER_BED_PIN) && HEATER_BED_PIN > -1
  // bed is reverse of other heaters - nozzle heaters typically turn on at pwm_count=0 and typically
  // turns off before pwm_count gets to 127.  But the bed typically does not turn on until pwm_count
  // is part way through the cycle and turns off when pwm_count gets back to 0.  This minimizes overlap
  // when bed and nozzles are both on to reduce the load variation on the power supply (probably not necessary
  // but possibly it lengthens the life of the capacitor inside the power brick).
  if (pwm_count > (0x7f-soft_pwm_b) ) // reverse timing
  {
    WRITE(HEATER_BED_PIN, 1);
  }
  else
  {
    WRITE(HEATER_BED_PIN, 0);
  }
  #endif
  #ifdef FAN_SOFT_PWM
  if(soft_pwm_fan <= pwm_count) WRITE(FAN_PIN,0);
  #endif

  pwm_count += (1 << SOFT_PWM_SCALE);
  pwm_count &= 0x7f;

  switch(temp_state) {
    case 1: // Measure TEMP_0
      #if defined(TEMP_0_PIN) && (TEMP_0_PIN > -1)
        raw_temp_0_value += ADC;
      #endif
      // Prepare TEMP_BED
      #if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1)
        #if TEMP_BED_PIN > 7
          ADCSRB = 1<<MUX5;
        #else
          ADCSRB = 0;
        #endif
        ADMUX = ((1 << REFS0) | (TEMP_BED_PIN & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      #endif
      lcd_buttons_update();
      temp_state = 2;
      break;
    case 2: // Measure TEMP_BED
      #if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1)
        raw_temp_bed_value += ADC;
      #endif
      // Prepare TEMP_1
      #if defined(TEMP_1_PIN) && (TEMP_1_PIN > -1) && EXTRUDERS > 1
        #if TEMP_1_PIN > 7
          ADCSRB = 1<<MUX5;
        #else
          ADCSRB = 0;
        #endif
        ADMUX = ((1 << REFS0) | (TEMP_1_PIN & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      #endif
      lcd_buttons_update();
      temp_state = 3;
      break;
    case 3: // Measure TEMP_1
      #if defined(TEMP_1_PIN) && (TEMP_1_PIN > -1) && EXTRUDERS > 1
        raw_temp_1_value += ADC;
      #endif
      // Prepare TEMP_2
      #if defined(TEMP_2_PIN) && (TEMP_2_PIN > -1) && EXTRUDERS > 2
        #if TEMP_2_PIN > 7
          ADCSRB = 1<<MUX5;
        #else
          ADCSRB = 0;
        #endif
        ADMUX = ((1 << REFS0) | (TEMP_2_PIN & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      #endif
      lcd_buttons_update();
      temp_state = 4;
      break;
    case 4: // Measure TEMP_2
      #if defined(TEMP_2_PIN) && (TEMP_2_PIN > -1) && EXTRUDERS > 2
        raw_temp_2_value += ADC;
      #endif
      temp_count++;
      //Fall trough to state 0
    case 0: // Prepare TEMP_0
      #if defined(TEMP_0_PIN) && (TEMP_0_PIN > -1)
        #if TEMP_0_PIN > 7
          ADCSRB = 1<<MUX5;
        #else
          ADCSRB = 0;
        #endif
        ADMUX = ((1 << REFS0) | (TEMP_0_PIN & 0x07));
        ADCSRA |= 1<<ADSC; // Start conversion
      #endif
      lcd_buttons_update();
      temp_state = 1;
      break;
    case 5: //Startup, delay initial temp reading a tiny bit so the hardware can settle.
      temp_state = 0;
      break;
//    default:
//      SERIAL_ERROR_START;
//      SERIAL_ERRORLNPGM("Temp measurement error!");
//      break;
  }

  if(temp_count >= OVERSAMPLENR) // 8 ms * 16 = 128ms.
  {
    if (!temp_meas_ready) //Only update the raw values if they have been read. Else we could be updating them during reading.
    {
#ifndef HEATER_0_USES_MAX6675
      current_temperature_raw[0] = raw_temp_0_value;
#endif
#if EXTRUDERS > 1
      current_temperature_raw[1] = raw_temp_1_value;
#endif
#ifdef TEMP_SENSOR_1_AS_REDUNDANT
      redundant_temperature_raw = raw_temp_1_value;
#endif
#if EXTRUDERS > 2
      current_temperature_raw[2] = raw_temp_2_value;
#endif
#if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1) && (TEMP_SENSOR_BED != 0)
      current_temperature_bed_raw = raw_temp_bed_value;
#endif
    }

    temp_meas_ready = true;
    temp_count = 0;
    raw_temp_0_value = 0;
#if EXTRUDERS > 1
    raw_temp_1_value = 0;
#endif
#if EXTRUDERS > 2
    raw_temp_2_value = 0;
#endif
#if defined(TEMP_BED_PIN) && (TEMP_BED_PIN > -1)
    raw_temp_bed_value = 0;
#endif

#if HEATER_0_RAW_LO_TEMP > HEATER_0_RAW_HI_TEMP
    if(current_temperature_raw[0] <= maxttemp_raw[0]) {
#else
    if(current_temperature_raw[0] >= maxttemp_raw[0]) {
#endif
#ifndef HEATER_0_USES_MAX6675
       max_temp_error(0);
#endif
    }
#if HEATER_0_RAW_LO_TEMP > HEATER_0_RAW_HI_TEMP
    if(current_temperature_raw[0] >= minttemp_raw[0]) {
#else
    if(current_temperature_raw[0] <= minttemp_raw[0]) {
#endif
#ifndef HEATER_0_USES_MAX6675
       min_temp_error(0);
#endif
    }
#if EXTRUDERS > 1
#if HEATER_1_RAW_LO_TEMP > HEATER_1_RAW_HI_TEMP
    if(current_temperature_raw[1] <= maxttemp_raw[1]) {
#else
    if(current_temperature_raw[1] >= maxttemp_raw[1]) {
#endif
        max_temp_error(1);
    }
#if HEATER_1_RAW_LO_TEMP > HEATER_1_RAW_HI_TEMP
    if(current_temperature_raw[1] >= minttemp_raw[1]) {
#else
    if(current_temperature_raw[1] <= minttemp_raw[1]) {
#endif
        min_temp_error(1);
    }
#endif
#if EXTRUDERS > 2
#if HEATER_2_RAW_LO_TEMP > HEATER_2_RAW_HI_TEMP
    if(current_temperature_raw[2] <= maxttemp_raw[2]) {
#else
    if(current_temperature_raw[2] >= maxttemp_raw[2]) {
#endif
        max_temp_error(2);
    }
#if HEATER_2_RAW_LO_TEMP > HEATER_2_RAW_HI_TEMP
    if(current_temperature_raw[2] >= minttemp_raw[2]) {
#else
    if(current_temperature_raw[2] <= minttemp_raw[2]) {
#endif
        min_temp_error(2);
    }
#endif

  /* No bed MINTEMP error? */
#if defined(BED_MAXTEMP) && (TEMP_SENSOR_BED != 0)
# if HEATER_BED_RAW_LO_TEMP > HEATER_BED_RAW_HI_TEMP
    if(current_temperature_bed_raw <= bed_maxttemp_raw) {
#else
    if(current_temperature_bed_raw >= bed_maxttemp_raw) {
#endif
       target_temperature_bed = 0;
       bed_max_temp_error();
    }
#endif
  }
#if defined(BABYSTEPPING)
  for(uint8_t axis=0; axis<3; ++axis)
  {
    int curTodo=babystepsTodo[axis]; //get rid of volatile for performance

    if(curTodo>0)
    {
      babystep(axis,/*fwd*/true);
      --babystepsTodo[axis]; //less to do next time
    }
    else if(curTodo<0)
    {
      babystep(axis,/*fwd*/false);
      ++babystepsTodo[axis]; //less to do next time
    }
  }
#endif //BABYSTEPPING

}

#ifdef PIDTEMP
// Apply the scale factors to the PID values


float scalePID_i(float i)
{
	return i*PID_dT;
}

float unscalePID_i(float i)
{
	return i/PID_dT;
}

float scalePID_d(float d)
{
    return d/PID_dT;
}

float unscalePID_d(float d)
{
	return d*PID_dT;
}

void set_maxtemp(uint8_t e, int maxTemp)
{
#ifdef HEATER_0_MAXTEMP
  if (e == 0)
  {
      maxttemp[0] = maxTemp;
      maxttemp_raw[0] = HEATER_0_RAW_HI_TEMP;
      while(analog2temp(maxttemp_raw[0], 0) > maxTemp) {
#if HEATER_0_RAW_LO_TEMP < HEATER_0_RAW_HI_TEMP
        maxttemp_raw[0] -= OVERSAMPLENR;
#else
        maxttemp_raw[0] += OVERSAMPLENR;
#endif
      }
  }
#endif //MAXTEMP

#if (EXTRUDERS > 1) && defined(HEATER_1_MAXTEMP)
  if (e == 1)
  {
      maxttemp[1] = maxTemp;
      maxttemp_raw[1] = HEATER_1_RAW_HI_TEMP;
      while(analog2temp(maxttemp_raw[1], 1) > maxTemp) {
    #if HEATER_1_RAW_LO_TEMP < HEATER_1_RAW_HI_TEMP
        maxttemp_raw[1] -= OVERSAMPLENR;
    #else
        maxttemp_raw[1] += OVERSAMPLENR;
    #endif
      }
  }
#endif //MAXTEMP 1

#if (EXTRUDERS > 2) && defined(HEATER_2_MAXTEMP)
  if (e > 1)
  {
      maxttemp[2] = maxTemp;
      maxttemp_raw[2] = HEATER_2_RAW_HI_TEMP;
      while(analog2temp(maxttemp_raw[2], 2) > maxTemp) {
    #if HEATER_2_RAW_LO_TEMP < HEATER_2_RAW_HI_TEMP
        maxttemp_raw[2] -= OVERSAMPLENR;
    #else
        maxttemp_raw[2] += OVERSAMPLENR;
    #endif
      }
  }
#endif //MAXTEMP 2
}

int get_maxtemp(uint8_t e)
{
    return maxttemp[e];
}

void setTargetHotend(const uint16_t &celsius, uint8_t extruder) {
  target_temperature[extruder] = celsius;
  if (target_temperature[extruder] >= HEATER_0_MAXTEMP - 15)
    target_temperature[extruder] = HEATER_0_MAXTEMP - 15;
}

void cooldownHotend(uint8_t extruder)
{
    target_temperature[extruder] = 0;
}

#if (TEMP_SENSOR_BED != 0) && defined(BED_MAXTEMP)
void setTargetBed(const uint16_t &celsius)
{
  target_temperature_bed = celsius;
  if (target_temperature_bed > BED_MAXTEMP - 15)
    target_temperature_bed = BED_MAXTEMP - 15;
}
#endif


#endif //PIDTEMP


