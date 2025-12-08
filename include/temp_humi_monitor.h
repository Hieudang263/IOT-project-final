#ifndef __TEMP_HUMI_MONITOR__
#define __TEMP_HUMI_MONITOR__
#include <Arduino.h>
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include "DHT.h" //for DHT11 uses
#include "global.h"
#include "printLCD.h"


void temp_humi_monitor(void *pvParameters);


#endif