#include "global.h"

QueueHandle_t TempHumidQueue = xQueueCreate(1, sizeof(TempHumid));
QueueHandle_t PredictQueue = xQueueCreate(1, sizeof(PredictData));

String WIFI_SSID = "";
String WIFI_PASS = "";
String WIFI_USERNAME = "";
String CORE_IOT_TOKEN;
String CORE_IOT_SERVER;
String CORE_IOT_PORT;

bool ap_started = false;

// WiFi credentials (mặc định)
String ssid = "ESP32-Setup-Wifi";
String password = "123456789";

// Các giá trị Wi-Fi khác (tuỳ phần code gốc)
String wifi_ssid = "abcde";
String wifi_password = "123456789";

boolean isWifiConnected = false;
SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();