#ifndef CONFIG_COREIOT_H
#define CONFIG_COREIOT_H

#include <Arduino.h>
#include "global.h"

// ✅ MQTT Basic Authentication
extern String coreiot_server;
extern int    coreiot_port;
extern String coreiot_client_id;
extern String coreiot_username;
extern String coreiot_password;

// ✅ Device location
extern float  coreiot_longitude;
extern float  coreiot_latitude;

bool loadCoreIOTConfig();
bool saveCoreIOTConfig();

#endif