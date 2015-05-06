#include "led_rgbw_pca9632.h"
#include "i2c_driver.h"

#define I2C_LED_ADDRESS 0b1100001

static i2cCommand led_command;
static uint8_t led_command_buffer[5];

void ledRGBWInit()
{
    i2cCommand led_init_command;
    uint8_t led_init_buffer[10];

    i2cDriverCommandSetup(led_command, I2C_LED_ADDRESS << 1 | i2cWriteBit, 1, led_command_buffer, sizeof(led_command_buffer));
    i2cDriverCommandSetup(led_init_command, I2C_LED_ADDRESS << 1 | i2cWriteBit, 1, led_init_buffer, sizeof(led_init_buffer));

    led_init_buffer[0] = 0x80;//Write from address 0 with auto-increase.
    led_init_buffer[1] = 0x80;//MODE1
    led_init_buffer[2] = 0x1C;//MODE2
    led_init_buffer[3] = 0;//PWM0=Red
    led_init_buffer[4] = 0;//PWM1=Green
    led_init_buffer[5] = 0;//PWM2=Blue
    led_init_buffer[6] = 0x00;//PWM3
    led_init_buffer[7] = 0xFF;//GRPPWM
    led_init_buffer[8] = 0x00;//GRPFREQ
    led_init_buffer[9] = 0xAA;//LEDOUT
    i2cDriverExecuteAndWait(&led_init_command);
}

void ledRGBWUpdate(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    if (!led_command.finished)
        return;

    led_command_buffer[0] = 0x82;//Start at address 2 with auto-increase enabled.
    led_command_buffer[1] = r;//PWM0
    led_command_buffer[2] = g;//PWM1
    led_command_buffer[3] = b;//PWM2
    led_command_buffer[4] = w;//PWM2
    i2cDriverPlan(&led_command);
}
