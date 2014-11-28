#include <avr/io.h>
#include <util/delay.h>

#include "marlin.h"

#include "temperature_ADS101X.h"
#include "i2c_driver.h"

#define ADS101X_ADC_ADDRESS 0b1001000

#define ADS101X_REGISTER_DATA 0x00
#define ADS101X_REGISTER_CONFIG 0x01

#define ADS101X_CONFIG_MSB_OS _BV(7)
#define ADS101X_CONFIG_MSB_MUX(n) ((n) << 4)
#define ADS101X_CONFIG_MSB_MUX_AIN(n) ADS101X_CONFIG_MSB_MUX(4 | (n))
#define ADS101X_CONFIG_MSB_MUX_AIN0_MIN_AIN1 ADS101X_CONFIG_MSB_MUX(0)
#define ADS101X_CONFIG_MSB_MUX_AIN0_MIN_AIN3 ADS101X_CONFIG_MSB_MUX(1)
#define ADS101X_CONFIG_MSB_MUX_AIN1_MIN_AIN3 ADS101X_CONFIG_MSB_MUX(2)
#define ADS101X_CONFIG_MSB_MUX_AIN2_MIN_AIN3 ADS101X_CONFIG_MSB_MUX(3)
#define ADS101X_CONFIG_MSB_PGA(n) ((n) << 1)
#define ADS101X_CONFIG_MSB_PGA_6V114 ADS101X_CONFIG_MSB_PGA(0)
#define ADS101X_CONFIG_MSB_PGA_4V096 ADS101X_CONFIG_MSB_PGA(1)
#define ADS101X_CONFIG_MSB_PGA_2V048 ADS101X_CONFIG_MSB_PGA(2)
#define ADS101X_CONFIG_MSB_PGA_1V024 ADS101X_CONFIG_MSB_PGA(3)
#define ADS101X_CONFIG_MSB_PGA_0V512 ADS101X_CONFIG_MSB_PGA(4)
#define ADS101X_CONFIG_MSB_PGA_0V256 ADS101X_CONFIG_MSB_PGA(5)
#define ADS101X_CONFIG_MSB_MODE _BV(0)
#define ADS101X_CONFIG_LSB_DR(n) ((n) << 5)
#define ADS101X_CONFIG_LSB_DR_128SPS ADS101X_CONFIG_LSB_DR(0)
#define ADS101X_CONFIG_LSB_DR_250SPS ADS101X_CONFIG_LSB_DR(1)
#define ADS101X_CONFIG_LSB_DR_490SPS ADS101X_CONFIG_LSB_DR(2)
#define ADS101X_CONFIG_LSB_DR_920SPS ADS101X_CONFIG_LSB_DR(3)
#define ADS101X_CONFIG_LSB_DR_1600SPS ADS101X_CONFIG_LSB_DR(4)
#define ADS101X_CONFIG_LSB_DR_2400SPS ADS101X_CONFIG_LSB_DR(5)
#define ADS101X_CONFIG_LSB_DR_3300SPS ADS101X_CONFIG_LSB_DR(6)
#define ADS101X_CONFIG_LSB_COMP_MODE _BV(4)
#define ADS101X_CONFIG_LSB_COMP_POL _BV(3)
#define ADS101X_CONFIG_LSB_COMP_LAT _BV(2)
#define ADS101X_CONFIG_LSB_COMP_QUE(n) ((n) << 0)
#define ADS101X_CONFIG_LSB_COMP_QUE_ONE ADS101X_CONFIG_LSB_COMP_QUE(0)
#define ADS101X_CONFIG_LSB_COMP_QUE_TWO ADS101X_CONFIG_LSB_COMP_QUE(1)
#define ADS101X_CONFIG_LSB_COMP_QUE_FOUR ADS101X_CONFIG_LSB_COMP_QUE(2)
#define ADS101X_CONFIG_LSB_COMP_QUE_NONE ADS101X_CONFIG_LSB_COMP_QUE(3)

static i2cCommand i2c_adc_setup_command;
static uint8_t i2c_adc_setup_buffer[3];

static i2cCommand i2c_adc_pointer_command;
static uint8_t i2c_adc_pointer_buffer[1];
    
static i2cCommand i2c_adc_read_command;
static uint8_t i2c_adc_read_buffer[2];

void initTemperatureADS101X()
{
    i2cDriverCommandSetup(i2c_adc_setup_command, ADS101X_ADC_ADDRESS << 1 | i2cWriteBit, 100, i2c_adc_setup_buffer, sizeof(i2c_adc_setup_buffer));
    i2cDriverCommandSetup(i2c_adc_pointer_command, ADS101X_ADC_ADDRESS << 1 | i2cWriteBit, 100, i2c_adc_pointer_buffer, sizeof(i2c_adc_pointer_buffer));
    i2cDriverCommandSetup(i2c_adc_read_command, ADS101X_ADC_ADDRESS << 1 | i2cReadBit, 100, i2c_adc_read_buffer, sizeof(i2c_adc_read_buffer));


    /**
        Read AIN3, which is connected to a register ladder of 3.3V - 1k21kOhm -|- 205Ohm - 0V
        The result reading should be 0.478V, which is an ADC value of 956 (if the PGA is set to 1.024V FS)
     */
    i2c_adc_setup_buffer[0] = ADS101X_REGISTER_CONFIG;
    i2c_adc_setup_buffer[1] = ADS101X_CONFIG_MSB_OS | ADS101X_CONFIG_MSB_MUX_AIN(3) | ADS101X_CONFIG_MSB_PGA_1V024 | ADS101X_CONFIG_MSB_MODE;
    i2c_adc_setup_buffer[2] = ADS101X_CONFIG_LSB_DR_1600SPS | ADS101X_CONFIG_LSB_COMP_QUE_NONE;
    i2cDriverExecuteAndWait(&i2c_adc_setup_command);

    _delay_ms(1);

    i2c_adc_pointer_buffer[0] = ADS101X_REGISTER_DATA;
    i2cDriverExecuteAndWait(&i2c_adc_pointer_command);
    
    i2cDriverExecuteAndWait(&i2c_adc_read_command);
    int adc_offset = int(i2c_adc_read_buffer[0]) << 4 | int(i2c_adc_read_buffer[1]) >> 4;
}

void temperatureADS101XSetupAIN0()
{
    i2c_adc_setup_buffer[0] = ADS101X_REGISTER_CONFIG;
    i2c_adc_setup_buffer[1] = ADS101X_CONFIG_MSB_OS | ADS101X_CONFIG_MSB_MUX_AIN0_MIN_AIN3 | ADS101X_CONFIG_MSB_PGA_0V256 | ADS101X_CONFIG_MSB_MODE;
    i2c_adc_setup_buffer[2] = ADS101X_CONFIG_LSB_DR_1600SPS | ADS101X_CONFIG_LSB_COMP_QUE_NONE;
    i2cDriverPlan(&i2c_adc_setup_command);
}

void temperatureADS101XSetupAIN1()
{
    i2c_adc_setup_buffer[0] = ADS101X_REGISTER_CONFIG;
    i2c_adc_setup_buffer[1] = ADS101X_CONFIG_MSB_OS | ADS101X_CONFIG_MSB_MUX_AIN1_MIN_AIN3 | ADS101X_CONFIG_MSB_PGA_0V256 | ADS101X_CONFIG_MSB_MODE;
    i2c_adc_setup_buffer[2] = ADS101X_CONFIG_LSB_DR_1600SPS | ADS101X_CONFIG_LSB_COMP_QUE_NONE;
    i2cDriverPlan(&i2c_adc_setup_command);
}

void temperatureADS101XRequestResult()
{
    i2c_adc_pointer_buffer[0] = ADS101X_REGISTER_DATA;
    i2cDriverPlan(&i2c_adc_pointer_command);
    i2cDriverPlan(&i2c_adc_read_command);
}

int16_t temperatureADS101XGetResult()
{
    int16_t result = int(i2c_adc_read_buffer[0]) << 4 | int(i2c_adc_read_buffer[1]) >> 4;
    if (result & 0x800)//Fix two complements result.
        result |= 0xF000;
    return result;
}
