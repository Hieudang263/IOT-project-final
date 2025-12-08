#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "LiquidCrystal_I2C.h"

#define LED_GPIO 48
#define SENSOR_PIN 4
// #define DEBUG 1

struct TempHumid{
    float temperature;
    float humidity;
};
extern bool ap_started;

extern float glob_temperature;
extern float glob_humidity;

extern QueueHandle_t TempHumidQueue;
extern QueueHandle_t waterValueQueue;
extern QueueHandle_t fanSpeedQueue;
extern LiquidCrystal_I2C lcd;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String WIFI_USERNAME;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;
extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern SemaphoreHandle_t xTempHumiSemaphore; // Semaphore đồng bộ nhiệt độ và LED
extern SemaphoreHandle_t xHumidityMutex; // MUTEX cho độ ẩm

#endif