#ifndef LED_RGBW_PCA9632_H
#define LED_RGBW_PCA9632_H

#include <stdint.h>

void ledRGBWInit();
void ledRGBWUpdate(uint8_t r, uint8_t g, uint8_t b, uint8_t w);

#endif//LED_RGBW_PCA9632_H
