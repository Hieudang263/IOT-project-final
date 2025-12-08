#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "global.h"
#include "task_wifi.h"
#include "task_check_info.h"
#include "mainserver.h"

// ==================== LED CONFIG ====================
static const uint8_t LED1_CHANNEL = 0;
static const uint8_t LED2_CHANNEL = 1;
static const bool LED1_IS_NEOPIXEL = true;
static Adafruit_NeoPixel led1Strip(1, LED1_PIN, NEO_GRB + NEO_KHZ800);

struct LEDState {
  bool isOn;
  int brightness;
  int pwmValue;
};

LEDState led1 = {false, 50, 127};
LEDState led2 = {false, 50, 127};

// ==================== EXTERN VARIABLES ====================
extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern bool isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern String WIFI_SSID;
extern String WIFI_PASS;
extern String WIFI_USERNAME;  // ✅ THÊM DÒNG NÀY
extern void Save_info_File(String, String, String, String, String, bool);
// ==================== GLOBAL VARIABLES ====================
WebServer server(80);
bool isAPMode = false;
String ap_ssid = "ESP32-Setup-Wifi";
String ap_password = "123456789";

// ==================== PWM FUNCTIONS ====================
void setupPWM() {
  Serial.println("[PWM] Setting up PWM...");

  if (LED1_IS_NEOPIXEL) {
    led1Strip.begin();
    led1Strip.clear();
    led1Strip.show();
    Serial.println("[PWM] LED1 configured as NeoPixel on GPIO16");
  } else {
    ledcSetup(LED1_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED1_PIN, LED1_CHANNEL);
    ledcWrite(LED1_CHANNEL, 0);
  }

  ledcSetup(LED2_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LED2_PIN, LED2_CHANNEL);
  ledcWrite(LED2_CHANNEL, 0);

  Serial.println("[PWM] Initialized (LED1:GPIO16, LED2:GPIO17 PWM)");
}

void setLED(int num, bool state, int brightness) {
  LEDState* led = (num == 1) ? &led1 : &led2;
  uint8_t channel = (num == 1) ? LED1_CHANNEL : LED2_CHANNEL;

  led->isOn = state;
  led->brightness = constrain(brightness, 0, 100);

  if (state && brightness > 0) {
    led->pwmValue = map(led->brightness, 0, 100, 0, 255);
    if (num == 1 && LED1_IS_NEOPIXEL) {
      led1Strip.setBrightness(led->pwmValue);
      uint32_t color = led1Strip.Color(255, 255, 255);
      led1Strip.fill(color, 0, 1);
      led1Strip.show();
    } else {
      ledcWrite(channel, led->pwmValue);
    }
    Serial.printf("[LED] LED%d: ON @ %d%% (PWM:%d)\n", num, led->brightness, led->pwmValue);
  } else {
    led->pwmValue = 0;
    if (num == 1 && LED1_IS_NEOPIXEL) {
      led1Strip.clear();
      led1Strip.show();
    } else {
      ledcWrite(channel, 0);
    }
    Serial.printf("[LED] LED%d: OFF\n", num);
  }
}

// Shared LED control helper (used by both port 80 and port 8080)
String processLedControl(int device, const String &state, int brightness, int &httpCode) {
  if (device < 1 || device > 2) {
    httpCode = 400;
    return "{\"ok\":false,\"message\":\"Invalid device\"}";
  }

  bool isOn = (state == "ON" || state == "on");
  int clampedBrightness = constrain(brightness, 0, 100);
  setLED(device, isOn, clampedBrightness);

  httpCode = 200;
  return "{\"ok\":true,\"led\":" + String(device) +
         ",\"state\":\"" + (isOn ? "ON" : "OFF") +
         "\",\"brightness\":" + String(clampedBrightness) + "}";
}

// ==================== HTML PAGE ====================
String mainPage() {
  if (LittleFS.exists("/config.html")) {
    File f = LittleFS.open("/config.html", "r");
    if (f) {
      String html = f.readString();
      f.close();
      return html;
    }
  }
  return "<!DOCTYPE html><html><body><h1>Upload config.html to /data</h1></body></html>";
}

// ==================== HTTP HANDLERS ====================
void handleRoot() {
  Serial.println("📥 GET / (Port 80)");
  server.send(200, "text/html", mainPage());
}

void handleControl() {
  int device = server.arg("device").toInt();
  String state = server.arg("state");
  int brightness = server.arg("brightness").toInt();
  
  Serial.println("\n======== LED CONTROL (Port 80) ========");
  Serial.printf("Device:%d State:%s Bright:%d%%\n", device, state.c_str(), brightness);
  
  int httpCode = 200;
  String json = processLedControl(device, state, brightness, httpCode);
  server.send(httpCode, httpCode == 200 ? "application/json" : "text/plain", json);
  Serial.println("=======================================\n");
}

void handleScan() {
  Serial.println("📥 GET /scan");
  int n = WiFi.scanNetworks();
  
  String json = "{\"networks\":[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    
    wifi_auth_mode_t authMode = WiFi.encryptionType(i);
    String encType = "Unknown";
    bool isEnterprise = false;
    
    switch (authMode) {
      case WIFI_AUTH_OPEN:
        encType = "Open";
        break;
      case WIFI_AUTH_WEP:
        encType = "WEP";
        break;
      case WIFI_AUTH_WPA_PSK:
        encType = "WPA-PSK";
        break;
      case WIFI_AUTH_WPA2_PSK:
        encType = "WPA2-PSK";
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        encType = "WPA/WPA2-PSK";
        break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        encType = "WPA2-Enterprise";
        isEnterprise = true;
        break;
      case WIFI_AUTH_WPA3_PSK:
        encType = "WPA3-PSK";
        break;
      default:
        encType = "Unknown";
        break;
    }
    
    json += "\"encryption\":\"" + encType + "\",";
    json += "\"is_enterprise\":" + String(isEnterprise ? "true" : "false");
    json += "}";
  }
  
  json += "]}";
  server.send(200, "application/json", json);
}

void handleConnect() {
    Serial.println("\n======== WIFI CONNECT ========");
    
    wifi_ssid = server.arg("ssid");
    wifi_password = server.arg("pass");
    String username = server.arg("user");
    
    Serial.println("SSID: " + wifi_ssid);
    Serial.println("Pass: " + String(wifi_password.length()) + " chars");
    if (!username.isEmpty()) {
        Serial.println("Username: " + username + " (WPA2-Enterprise)");
    } else {
        Serial.println("Username: (empty - WPA2-PSK)");
    }
    
    if (wifi_ssid.isEmpty()) {
        server.send(400, "text/plain", "SSID required");
        return;
    }
    
    WIFI_SSID = wifi_ssid;
    WIFI_PASS = wifi_password;
    WIFI_USERNAME = username;
    
    server.send(200, "text/plain", "Connecting to: " + wifi_ssid);
    delay(100);
    
    // ✅ TẮT AP SAU KHI LƯU CẤU HÌNH
    //if (isAPMode) {
    //    Serial.println("🛑 Disabling AP mode after WiFi config...");
    //    WiFi.softAPdisconnect(true);
    //    isAPMode = false;
     //    }
     
    
    // ✅ LƯU VÀO JSON FILE (/info.dat) - GIỮ LẠI CÁC TRƯỜNG CŨ
    DynamicJsonDocument doc(4096);
    
    // Load file cũ để giữ lại MQTT config
    if (LittleFS.exists("/info.dat")) {
        File oldFile = LittleFS.open("/info.dat", "r");
        if (oldFile) {
            deserializeJson(doc, oldFile);
            oldFile.close();
            Serial.println("📄 Loaded existing /info.dat (keeping MQTT config)");
        }
    }
    
    // Cập nhật WiFi config
    doc["WIFI_SSID"] = WIFI_SSID;
    doc["WIFI_PASS"] = WIFI_PASS;
    doc["WIFI_USERNAME"] = WIFI_USERNAME;
    
    // Lưu vào file
    File file = LittleFS.open("/info.dat", "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("✅ WiFi config saved to /info.dat");
    } else {
        Serial.println("❌ Failed to save /info.dat!");
    }
    
    // Debug - xem nội dung JSON
    Serial.println("--- JSON content ---");
    serializeJsonPretty(doc, Serial);
    Serial.println("\n--------------------");
    
    Serial.println("==============================\n");
    
    // ✅ KẾT NỐI WIFI
    startSTA(false);
}

void handleAPConfig() {
  String newSSID = server.arg("ssid");
  String newPass = server.arg("pass");
  
  if (newSSID.isEmpty()) {
    server.send(400, "text/plain", "SSID required");
    return;
  }
  if (!newPass.isEmpty() && newPass.length() < 8) {
    server.send(400, "text/plain", "Password min 8 chars");
    return;
  }
  
  File f = LittleFS.open("/ap_config.txt", "w");
  if (f) { f.println(newSSID); f.println(newPass); f.close(); }
  
  server.send(200, "text/plain", "Saved! Restarting...");
  delay(1000);
  ESP.restart();
}

void handleSensor() {
  String json;
  TempHumid th;
  int water;
  bool hasData = false;
  
  // Peek at queue data without removing it
  if (TempHumidQueue != NULL && waterValueQueue != NULL) {
    hasData = (xQueuePeek(TempHumidQueue, &th, 0) == pdTRUE) && (xQueuePeek(waterValueQueue, &water, 0) == pdTRUE);

  #ifdef DEBUG
    Serial.println("HAS DATA NOW");
  #endif
  }
  
  if (hasData) {
    json = "{\"error\":false,\"temperature\":" + String(th.temperature, 1) + 
           ",\"humidity\":" + String(th.humidity, 1) + 
           ",\"rain\":" + String(water) + "}"; // đổi thành ",\"rain\":" + String((water*100)/4095) + "}";

#ifdef DEBUG
    Serial.println(json);
#endif 
  } else {
    // No data available in queue yet
    json = "{\"error\":true,\"temperature\":0,\"humidity\":0,\"rain\":0}";

#ifdef DEBUG
    Serial.println("NO DATA HAS BEEN RECEIVED YET");
#endif
  }
  
  server.send(200, "application/json", json);
}
void handleStatic(String path, String type) {
  if (LittleFS.exists(path)) {
    File f = LittleFS.open(path, "r");
    if (f) { server.streamFile(f, type); f.close(); return; }
  }
  server.send(404, "text/plain", "Not found");
}

// ==================== AP FUNCTIONS ====================
void startAP() {
  Serial.println("\n======== START AP (Port 80) ========");
  
  if (LittleFS.exists("/ap_config.txt")) {
    File f = LittleFS.open("/ap_config.txt", "r");
    if (f) {
      ap_ssid = f.readStringUntil('\n'); ap_ssid.trim();
      ap_password = f.readStringUntil('\n'); ap_password.trim();
      f.close();
    }
  }
  
  Serial.println("AP SSID: " + ap_ssid);
  
  if (ap_password.length() >= 8) {
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  } else {
    WiFi.softAP(ap_ssid.c_str());
  }
  
  delay(100);
  
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("====================================\n");
  
  isAPMode = true;
}

// ==================== MAIN SERVER TASK ====================
void main_server_task(void *pvParameters) {
    Serial.println("\n📡 Main Server Task Starting...");
    vTaskDelay(500 / portTICK_PERIOD_MS);
    
    setupPWM();
    
    // ✅ CHỈ BẬT AP KHI CHƯA CÓ CẤU HÌNH WIFI HOẶC KẾT NỐI THẤT BẠI
    if (WIFI_SSID.isEmpty() || WiFi.status() != WL_CONNECTED) {
        Serial.println("⚠️ No WiFi config or STA failed → Starting AP mode");
        startAP();
    } else {
        Serial.println("✅ WiFi STA connected → AP mode is still ENABLED (dual mode)");
    }
    
    // Register routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/control", HTTP_GET, handleControl);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/connect", HTTP_GET, handleConnect);
    server.on("/apconfig", HTTP_GET, handleAPConfig);
    server.on("/sensor", HTTP_GET, handleSensor);
    
    server.on("/script.js", HTTP_GET, []() { handleStatic("/script.js", "application/javascript"); });
    server.on("/styles.css", HTTP_GET, []() { handleStatic("/styles.css", "text/css"); });
    
    server.onNotFound([]() {
        Serial.println("404: " + server.uri());
        server.send(404, "text/plain", "Not Found");
    });

    server.begin();
    Serial.println("✅ HTTP Server started on port 80");
    
    if (isAPMode) {
        Serial.println("🌐 Access: http://" + WiFi.softAPIP().toString());
    } else {
        Serial.println("🌐 Access: http://" + WiFi.localIP().toString());
    }
    
    for (;;) {
        server.handleClient();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
