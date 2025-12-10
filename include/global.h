#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/queue.h"

#define LED_GPIO 48
#define SENSOR_PIN 4
// #define DEBUG 1

struct TempHumid{
    float temperature;
    float humidity;
};
extern float temperature;
extern float humidity;

extern bool ap_started;

extern QueueHandle_t TempHumidQueue;
extern QueueHandle_t PredictQueue;

extern LiquidCrystal_I2C lcd;

extern String WIFI_SSID;
extern String WIFI_PASS;
extern String WIFI_USERNAME;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;
extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern SemaphoreHandle_t xTempHumiSemaphore;
extern SemaphoreHandle_t xHumidityMutex;
extern SemaphoreHandle_t PrintOnLCDSemaphore;

// Thêm struct cho dữ liệu dự đoán
struct PredictData {
    float predicted_temp;
    float predicted_humi;
    int accuracy;
    bool has_data;
};

// Thêm queue
extern QueueHandle_t PredictQueue;

#endif