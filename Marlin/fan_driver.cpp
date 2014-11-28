#include "fan_driver.h"
#include "i2c_driver.h"
#include "Marlin.h"

#define I2C_FAN_ADDRESS 0b1100010

i2cCommand fan_update_command;
uint8_t led_update_buffer[4];

void initFans()
{
    i2cCommand fan_init_command;
    uint8_t led_init_buffer[10];
    
    i2cDriverCommandSetup(fan_update_command, I2C_FAN_ADDRESS << 1 | i2cWriteBit, 20, led_update_buffer, sizeof(led_update_buffer));
    i2cDriverCommandSetup(fan_init_command, I2C_FAN_ADDRESS << 1 | i2cWriteBit, 20, led_init_buffer, sizeof(led_init_buffer));

    led_init_buffer[0] = 0x80;//Write from address 0 with auto-increase.
    led_init_buffer[1] = 0x80;//MODE1
    led_init_buffer[2] = 0x1C;//MODE2
    led_init_buffer[3] = 0;//PWM0 = Hotend cooling fan
    led_init_buffer[4] = 0;//PWM1 = Fan1
    led_init_buffer[5] = 0;//PWM2 = Fan2
    led_init_buffer[6] = 0;//PWM3 = Unused H2
    led_init_buffer[7] = 0xFF;//GRPPWM
    led_init_buffer[8] = 0x00;//GRPFREQ
    led_init_buffer[9] = 0xAA;//LEDOUT
    i2cDriverExecuteAndWait(&fan_init_command);
    
    led_update_buffer[0] = 0x82;//Write from PWM0 with auto-increase
    led_update_buffer[1] = 0;
    led_update_buffer[2] = 0;
    led_update_buffer[3] = 0;

    //For the UM2 the head fan is connected to PJ6, which does not have an Arduino PIN definition. So use direct register access.
    DDRJ |= _BV(6);
}

void setCoolingFanSpeed(uint8_t fan_speed)
{
    #ifdef FAN_SOFT_PWM
    fanSpeedSoftPwm = fan_speed;
    #else
    analogWrite(FAN_PIN, fan_speed);
    #endif//!FAN_SOFT_PWM

    if (fan_update_command.finished && fan_speed != led_update_buffer[2])
    {
        led_update_buffer[2] = fan_speed;
        led_update_buffer[3] = fan_speed;
        i2cDriverPlan(&fan_update_command);
    }
}

void setHotendCoolingFanSpeed(uint8_t fan_speed)
{
    //For the UM2 the head fan is connected to PJ6, which does not have an Arduino PIN definition. So use direct register access.
    if (fan_speed)
    {
        PORTJ |= _BV(6);
    }else{
        PORTJ &=~_BV(6);
    }
    
    if (fan_update_command.finished && fan_speed != led_update_buffer[1])
    {
        led_update_buffer[1] = fan_speed;
        i2cDriverPlan(&fan_update_command);
    }
}
