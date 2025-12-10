#ifndef MAINSERVER_H
#define MAINSERVER_H

#include <Arduino.h>
#include "global.h"

extern bool isAPMode;

String mainPage();
String settingsPage();

// ✅ Khai báo hàm startAP để các file khác có thể dùng
void startAP();

void setupServer();
void connectToWiFi();

// Shared LED control helper so both port 80 and port 8080 can reuse it
String processLedControl(int device, const String &state, int brightness, int &httpCode);
void setLED(int num, bool state, int brightness);

void main_server_task(void *pvParameters);

#define LED1_PIN 48
#define LED2_PIN 17
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern bool isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern String WIFI_SSID;
extern String WIFI_PASS;
extern void Save_info_File(String, String, String, String, String, bool);

#endif
