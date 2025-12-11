#ifndef __PRINT_LCD_H__
#define __PRINT_LCD_H__

#include "global.h"
//#include "LiquidCrystal_I2C.h"
#include "temp_humi_monitor.h"

extern SemaphoreHandle_t PrintOnLCDSemaphore;
void reportTempAndHumidity(void* pvParameters);
void reportWaterAmount(void* pvParameters);
void switchLCD(void* pvParameters);

//extern LiquidCrystal_I2C lcd;
extern TaskHandle_t waterTaskHandle;
extern TaskHandle_t tempHumidTaskHandle;

#endif