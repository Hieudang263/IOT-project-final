#include "config_coreiot.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// ✅ Biến toàn cục
//{clientId:"ESP32_S3",userName:"KietNguyen",password:"87654321"}
// ✅ Biến toàn cục
String coreiot_server    = "";
int    coreiot_port      = 0;
String coreiot_client_id = "";
String coreiot_username  = "";
String coreiot_password  = "";
float  coreiot_longitude = 0.0;
float  coreiot_latitude  = 0.0;

bool loadCoreIOTConfig() {
    if (!LittleFS.exists("/coreiot.json")) {
        Serial.println("⚠️ Chưa có coreiot.json");
        return false;
    }

    File f = LittleFS.open("/coreiot.json", "r");
    if (!f) {
        Serial.println("❌ Không mở được coreiot.json");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    
    if (err) {
        Serial.print("❌ JSON parse error: ");
        Serial.println(err.c_str());
        return false;
    }

    coreiot_server    = doc["server"] | "";
    coreiot_port      = doc["port"] | 0;
    coreiot_client_id = doc["client_id"] | "";
    coreiot_username  = doc["username"] | "";
    coreiot_password  = doc["password"] | "";
    coreiot_longitude = doc["longitude"] | 0.0;
    coreiot_latitude  = doc["latitude"] | 0.0;

    Serial.println("📄 Loaded CoreIOT config:");
    Serial.println("   Server: " + coreiot_server);
    Serial.println("   Port: " + String(coreiot_port));
    Serial.println("   Client ID: " + coreiot_client_id);
    Serial.println("   Username: " + coreiot_username);
    Serial.println("   Password: " + String(coreiot_password.length() > 0 ? "***" : "(empty)"));
    Serial.println("   Longitude: " + String(coreiot_longitude, 6));
    Serial.println("   Latitude: " + String(coreiot_latitude, 6));

    ///xoá client_id mặc định cũ nếu có 
    if (!coreiot_client_id.isEmpty()) {
        String defaultClientId = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
        if (coreiot_client_id.equalsIgnoreCase(defaultClientId)) {
            Serial.println("⚠️ Clearing legacy default Client ID");
            coreiot_client_id = "";
            saveCoreIOTConfig();
        }
    }

    return true;
}

bool saveCoreIOTConfig() {
    StaticJsonDocument<512> doc;
    doc["server"]    = coreiot_server;
    doc["port"]      = coreiot_port;
    doc["client_id"] = coreiot_client_id;
    doc["username"]  = coreiot_username;
    doc["password"]  = coreiot_password;
    doc["longitude"] = coreiot_longitude;
    doc["latitude"]  = coreiot_latitude;

    File f = LittleFS.open("/coreiot.json", "w");
    if (!f) {
        Serial.println("❌ Cannot write coreiot.json");
        return false;
    }

    serializeJson(doc, f);
    f.close();

    Serial.println("💾 Saved coreiot.json");
    return true;
}