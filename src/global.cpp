#include "global.h"
LiquidCrystal_I2C lcd(0x21, 16, 2);

float temperature = 0;
float humidity = 0;

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
SemaphoreHandle_t xTempHumiSemaphore = NULL;
SemaphoreHandle_t xHumidityMutex = NULL;
QueueHandle_t TempHumidQueue = NULL;
SemaphoreHandle_t PrintOnLCDSemaphore = xSemaphoreCreateMutex();