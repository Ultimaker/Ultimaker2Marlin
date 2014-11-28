#include "i2c_driver.h"
#include "i2c_capacitance.h"

//FDC1004 4-Channel Capacitance-to-Digital Converter
static const uint8_t i2cSensorAddress = 0b1010000;

#define ADDR_MEAS_MSB(n)       (0x00 + (n) * 2)
#define ADDR_MEAS_LSB(n)       (0x01 + (n) * 2)
#define ADDR_CONF_MEAS(n)      (0x08 + (n))
#define ADDR_FDC_CONF          (0x0C)
#define ADDR_OFFSET_CAL_CIN(n) (0x0D + (n))
#define ADDR_GAIN_CAL_CIN(n)   (0x11 + (n))
#define ADDR_MANUFACTURER_ID   (0xFE)
#define ADDR_DEVICE_ID         (0xFF)

#define CONF_MSB_CHA_CIN1          (0x00)
#define CONF_MSB_CHA_CIN2          (0x20)
#define CONF_MSB_CHA_CIN3          (0x40)
#define CONF_MSB_CHA_CIN4          (0x60)

#define CONF_MSB_CHB_CIN1          (0x00)
#define CONF_MSB_CHB_CIN2          (0x04)
#define CONF_MSB_CHB_CIN3          (0x08)
#define CONF_MSB_CHB_CIN4          (0x0C)
#define CONF_MSB_CHB_CAPDAC        (0x10)
#define CONF_MSB_CHB_DISABLED      (0x1C)

#define CONF_MSB_CAPDAC_OFFSET(n)  ((n) >> 3)
#define CONF_LSB_CAPDAC_OFFSET(n)  ((n) << 5)

#define FDC_CONF_MSB_RESET         (0x80)
#define FDC_CONF_MSB_RATE_100HZ    (0x04)
#define FDC_CONF_MSB_RATE_200HZ    (0x08)
#define FDC_CONF_MSB_RATE_400HZ    (0x0C)
#define FDC_CONF_MSB_REPEAT        (0x01)
#define FDC_CONF_LSB_MEAS_EN(n)    (0x80 >> (n))
#define FDC_CONF_LSB_MEAS_DONE(n)  (0x08 >> (n))

static i2cCommand i2cSensorWriteCommand;
static uint8_t i2cSensorWriteBuffer[1];
static i2cCommand i2cSensorReadCommand;
static uint8_t i2cSensorReadBuffer[2];

void i2cCapacitanceInit()
{
    i2cCommand i2cSensorInitCommand;
    uint8_t i2cSensorInitBuffer[3];

    i2cDriverCommandSetup(i2cSensorWriteCommand, i2cSensorAddress << 1 | i2cWriteBit, 50, i2cSensorWriteBuffer, sizeof(i2cSensorWriteBuffer));
    i2cDriverCommandSetup(i2cSensorReadCommand, i2cSensorAddress << 1 | i2cReadBit, 50, i2cSensorReadBuffer, sizeof(i2cSensorReadBuffer));
    
    i2cDriverCommandSetup(i2cSensorInitCommand, i2cSensorAddress << 1 | i2cWriteBit, 50, i2cSensorInitBuffer, sizeof(i2cSensorInitBuffer));
    
    i2cSensorInitBuffer[0] = ADDR_FDC_CONF;
    i2cSensorInitBuffer[1] = FDC_CONF_MSB_RATE_400HZ | FDC_CONF_MSB_REPEAT;
    i2cSensorInitBuffer[2] = FDC_CONF_LSB_MEAS_EN(0);
    i2cDriverExecuteAndWait(&i2cSensorInitCommand);
    
    i2cSensorWriteBuffer[0] = ADDR_MEAS_MSB(0);
}

void i2cCapacitanceStart()
{
    if (!i2cSensorWriteCommand.finished || !i2cSensorReadCommand.finished)
        return;
    i2cDriverPlan(&i2cSensorWriteCommand);
    i2cDriverPlan(&i2cSensorReadCommand);
}

bool i2cCapacitanceDone(uint16_t& value)
{
    if (!i2cSensorWriteCommand.finished || !i2cSensorReadCommand.finished)
        return false;
    value = uint16_t(i2cSensorReadBuffer[0]) << 8 | uint16_t(i2cSensorReadBuffer[1]);
    return true;
}
