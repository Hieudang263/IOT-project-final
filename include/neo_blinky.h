#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define NEO_PIN 15
#define LED_COUNT 8

void neo_blinky(void *pvParameters);


#endif