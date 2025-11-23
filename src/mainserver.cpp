#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

#include "global.h"
#include "task_check_info.h"
#include "mainserver.h"

// ✅ LED PWM Config
#define LED1_PIN 48
#define LED2_PIN 41
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define PWM_CHANNEL_1 0
#define PWM_CHANNEL_2 1

// LED State
struct LEDState {
  bool isOn;
  int brightness;
  int pwmValue;
};

LEDState led1 = {false, 50, 127};
LEDState led2 = {false, 50, 127};

// Extern variables
extern String ssid;
extern String password;
extern String wifi_ssid;
extern String wifi_password;
extern bool isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern String WIFI_SSID;
extern String WIFI_PASS;
extern void Save_info_File(String wifi_ssid, String wifi_pass, String token, String server, String port);

WebServer server(80);

bool isAPMode = false;
bool connecting = false;
unsigned long connect_start_ms = 0;

// ==================== PWM FUNCTIONS ====================
void setupPWM() {
  ledcSetup(PWM_CHANNEL_1, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_2, PWM_FREQ, PWM_RESOLUTION);
  
  ledcAttachPin(LED1_PIN, PWM_CHANNEL_1);
  ledcAttachPin(LED2_PIN, PWM_CHANNEL_2);
  
  ledcWrite(PWM_CHANNEL_1, 0);
  ledcWrite(PWM_CHANNEL_2, 0);
  
  Serial.println("✅ PWM initialized (LED1: GPIO48, LED2: GPIO41)");
}

void setLED(int ledNum, bool state, int brightness) {
  LEDState* led = (ledNum == 1) ? &led1 : &led2;
  int channel = (ledNum == 1) ? PWM_CHANNEL_1 : PWM_CHANNEL_2;
  
  led->isOn = state;
  led->brightness = constrain(brightness, 0, 100);
  
  if (state) {
    led->pwmValue = map(led->brightness, 0, 100, 0, 255);
    ledcWrite(channel, led->pwmValue);
    Serial.printf("💡 LED%d ON - Brightness: %d%% (PWM: %d)\n", 
                  ledNum, led->brightness, led->pwmValue);
  } else {
    led->pwmValue = 0;
    ledcWrite(channel, 0);
    Serial.printf("💡 LED%d OFF\n", ledNum);
  }
}

// ==================== HTML: config.html ====================
String configPage() {
  if (LittleFS.exists("/config.html")) {
    File file = LittleFS.open("/config.html", "r");
    if (file) {
      String html = file.readString();
      file.close();
      Serial.println("📄 Serving /config.html from LittleFS");
      return html;
    }
  }
  
  // Fallback
  return R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Config - AP Mode</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', sans-serif;
      background: linear-gradient(135deg, #667eea, #764ba2);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 24px;
      padding: 40px;
      max-width: 500px;
      width: 100%;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
    }
    .header {
      text-align: center;
      margin-bottom: 40px;
    }
    .header h1 {
      color: #667eea;
      font-size: 28px;
      margin-bottom: 8px;
    }
    .header p {
      color: #666;
      font-size: 14px;
    }
    .led-card {
      background: #f8f9ff;
      border-radius: 16px;
      padding: 24px;
      margin-bottom: 20px;
    }
    .led-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 16px;
    }
    .led-name {
      font-size: 18px;
      font-weight: 600;
      color: #333;
    }
    .toggle-switch {
      position: relative;
      width: 56px;
      height: 30px;
    }
    .toggle-switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: 0.3s;
      border-radius: 30px;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 22px;
      width: 22px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: 0.3s;
      border-radius: 50%;
    }
    input:checked + .slider {
      background: linear-gradient(135deg, #667eea, #764ba2);
    }
    input:checked + .slider:before {
      transform: translateX(26px);
    }
    .brightness-label {
      display: flex;
      justify-content: space-between;
      margin-bottom: 10px;
      color: #666;
      font-size: 14px;
    }
    .brightness-value {
      font-weight: 700;
      color: #667eea;
    }
    input[type="range"] {
      -webkit-appearance: none;
      width: 100%;
      height: 6px;
      border-radius: 5px;
      background: #ddd;
      outline: none;
    }
    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: linear-gradient(135deg, #667eea, #764ba2);
      cursor: pointer;
      box-shadow: 0 2px 6px rgba(102, 126, 234, 0.4);
    }
    input[type="range"]:disabled {
      opacity: 0.3;
    }
    .slider-container.slider-enabled input[type="range"] {
      background: linear-gradient(to right, #667eea, #764ba2);
    }
    .settings-btn {
      width: 100%;
      padding: 16px;
      background: linear-gradient(135deg, #667eea, #764ba2);
      color: white;
      border: none;
      border-radius: 12px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      margin-top: 20px;
      transition: all 0.3s;
    }
    .settings-btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 20px rgba(102, 126, 234, 0.4);
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>💡 ESP32 LED Control</h1>
      <p>Access Point Mode - Task 4</p>
    </div>

    <!-- LED 1 -->
    <div class="led-card">
      <div class="led-header">
        <span class="led-name">LED 1 (GPIO 48)</span>
        <label class="toggle-switch">
          <input type="checkbox" id="led1Toggle">
          <span class="slider"></span>
        </label>
      </div>
      <div class="brightness-label">
        <span>Độ sáng</span>
        <span class="brightness-value" id="led1Value">50%</span>
      </div>
      <div class="slider-container" id="led1Container">
        <input type="range" id="led1Brightness" min="0" max="100" value="50" disabled>
      </div>
    </div>

    <!-- LED 2 -->
    <div class="led-card">
      <div class="led-header">
        <span class="led-name">LED 2 (GPIO 41)</span>
        <label class="toggle-switch">
          <input type="checkbox" id="led2Toggle">
          <span class="slider"></span>
        </label>
      </div>
      <div class="brightness-label">
        <span>Độ sáng</span>
        <span class="brightness-value" id="led2Value">50%</span>
      </div>
      <div class="slider-container" id="led2Container">
        <input type="range" id="led2Brightness" min="0" max="100" value="50" disabled>
      </div>
    </div>

    <button class="settings-btn" onclick="window.location.href='/settings'">
      ⚙️ Cấu hình Wi-Fi
    </button>
  </div>

  <script>
    // LED 1
    const led1Toggle = document.getElementById('led1Toggle');
    const led1Slider = document.getElementById('led1Brightness');
    const led1Value = document.getElementById('led1Value');
    const led1Container = document.getElementById('led1Container');

    led1Toggle.addEventListener('change', function() {
      led1Slider.disabled = !this.checked;
      if (this.checked) {
        led1Container.classList.add('slider-enabled');
        controlLED(1, 'ON', led1Slider.value);
      } else {
        led1Container.classList.remove('slider-enabled');
        controlLED(1, 'OFF', 0);
      }
    });

    led1Slider.addEventListener('input', function() {
      led1Value.textContent = this.value + '%';
    });

    led1Slider.addEventListener('change', function() {
      if (led1Toggle.checked) {
        controlLED(1, 'ON', this.value);
      }
    });

    // LED 2
    const led2Toggle = document.getElementById('led2Toggle');
    const led2Slider = document.getElementById('led2Brightness');
    const led2Value = document.getElementById('led2Value');
    const led2Container = document.getElementById('led2Container');

    led2Toggle.addEventListener('change', function() {
      led2Slider.disabled = !this.checked;
      if (this.checked) {
        led2Container.classList.add('slider-enabled');
        controlLED(2, 'ON', led2Slider.value);
      } else {
        led2Container.classList.remove('slider-enabled');
        controlLED(2, 'OFF', 0);
      }
    });

    led2Slider.addEventListener('input', function() {
      led2Value.textContent = this.value + '%';
    });

    led2Slider.addEventListener('change', function() {
      if (led2Toggle.checked) {
        controlLED(2, 'ON', this.value);
      }
    });

    function controlLED(device, state, brightness) {
      fetch(`/control?device=${device}&state=${state}&brightness=${brightness}`)
        .then(response => response.text())
        .then(data => {
          console.log(`✅ LED${device}: ${state} @ ${brightness}%`);
        })
        .catch(err => {
          console.error('❌ Error:', err);
        });
    }
  </script>
</body>
</html>
)rawliteral";
}

// ==================== HTML: settings.html ====================
String settingsPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Cấu hình Wi-Fi</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #1e90ff, #00e6b8);
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      margin: 0;
    }
    .card {
      background: white;
      padding: 40px;
      border-radius: 20px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.15);
      width: 100%;
      max-width: 360px;
      text-align: center;
    }
    h2 { color: #1e90ff; margin-bottom: 10px; }
    p { color: #666; margin-bottom: 25px; font-size: 14px; }
    input {
      width: 100%;
      padding: 12px 16px;
      margin: 10px 0;
      border-radius: 12px;
      border: 2px solid #e0e0e0;
      font-size: 15px;
      box-sizing: border-box;
      transition: all 0.3s;
    }
    input:focus {
      outline: none;
      border-color: #1e90ff;
      box-shadow: 0 0 0 3px rgba(30, 144, 255, 0.1);
    }
    .btn-row {
      margin-top: 20px;
      display: flex;
      gap: 12px;
    }
    button {
      flex: 1;
      border: none;
      border-radius: 12px;
      padding: 12px 20px;
      font-size: 15px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
    }
    .btn-primary {
      background: linear-gradient(90deg, #1e90ff, #00e6b8);
      color: white;
    }
    .btn-primary:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(30, 144, 255, 0.3);
    }
    .btn-secondary {
      background: #f1f5ff;
      color: #345;
    }
    .btn-secondary:hover {
      background: #e0e8ff;
    }
    #msg {
      margin-top: 15px;
      padding: 12px;
      border-radius: 10px;
      font-size: 14px;
      display: none;
    }
    #msg.show { display: block; }
    #msg.success { background: #d4edda; color: #155724; }
    #msg.error { background: #f8d7da; color: #721c24; }
  </style>
</head>
<body>
  <div class="card">
    <h2>⚙️ Cấu hình Wi-Fi</h2>
    <p>Nhập thông tin Wi-Fi để kết nối ESP32 vào mạng của bạn</p>
    
    <input id="ssid" placeholder="Tên Wi-Fi (SSID)" autocomplete="off" />
    <input id="pass" type="password" placeholder="Mật khẩu" autocomplete="off" />
    
    <div class="btn-row">
      <button class="btn-primary" onclick="sendConfig()">Kết nối</button>
      <button class="btn-secondary" onclick="window.location.href='/'">Quay lại</button>
    </div>
    
    <div id="msg"></div>
  </div>

  <script>
    function sendConfig() {
      const ssid = document.getElementById('ssid').value.trim();
      const pass = document.getElementById('pass').value.trim();
      const msg = document.getElementById('msg');

      if (!ssid) {
        showMessage('Vui lòng nhập tên Wi-Fi!', 'error');
        return;
      }

      showMessage('Đang gửi cấu hình...', 'success');

      fetch('/connect?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass))
        .then(res => res.text())
        .then(txt => {
          showMessage('✅ ' + txt, 'success');
        })
        .catch(err => {
          showMessage('❌ Lỗi: ' + err, 'error');
        });
    }

    function showMessage(text, type) {
      const msg = document.getElementById('msg');
      msg.textContent = text;
      msg.className = 'show ' + type;
    }
  </script>
</body>
</html>
)rawliteral";
}

// ==================== HTTP HANDLERS ====================
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", configPage());
}

void handleSettings() {
  server.send(200, "text/html; charset=utf-8", settingsPage());
}

void handleConnect() {
  Serial.println("\n===== /connect called =====");
  
  wifi_ssid = server.arg("ssid");
  wifi_password = server.arg("pass");
  
  Serial.println("SSID from web: " + wifi_ssid);
  Serial.println("PASS length: " + String(wifi_password.length()));
  
  WIFI_SSID = wifi_ssid;
  WIFI_PASS = wifi_password;
  
  Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);
  Serial.println("💾 Saved WiFi to /info.dat");
  
  server.send(200, "text/plain; charset=utf-8", "Đang kết nối... Xem Serial Monitor");
  
  connecting = true;
  connect_start_ms = millis();
  connectToWiFi();
}

void handleControl() {
  int device = server.arg("device").toInt();
  String state = server.arg("state");
  int brightness = server.arg("brightness").toInt();
  
  Serial.printf("📥 Control: LED%d = %s, Brightness = %d%%\n", 
                device, state.c_str(), brightness);
  
  bool isOn = (state == "ON");
  setLED(device, isOn, brightness);
  
  server.send(200, "text/plain", "OK");
}

void handleStaticFile(String path, String contentType) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    if (file) {
      server.streamFile(file, contentType);
      file.close();
      Serial.println("📄 Served: " + path);
      return;
    }
  }
  
  server.send(404, "text/plain", "File not found: " + path);
  Serial.println("❌ 404: " + path);
}

// ==================== WIFI FUNCTIONS ====================
void startAP() {
  Serial.println("\n=== Starting AP Mode ===");
  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + password);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(ip);
  
  isAPMode = true;
}

void connectToWiFi() {
  if (wifi_ssid.isEmpty()) {
    Serial.println("❌ SSID empty, cannot connect!");
    return;
  }
  
  Serial.println("\n=== Connecting to WiFi ===");
  Serial.println("SSID: " + wifi_ssid);
  Serial.println("PASS length: " + String(wifi_password.length()));
  
  WiFi.mode(WIFI_STA);
  
  if (wifi_password.isEmpty()) {
    WiFi.begin(wifi_ssid.c_str());
  } else {
    WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  }
}

// ==================== MAIN TASK ====================
void main_server_task(void *pvParameters) {
  Serial.println("\n=== Main Server Task Started ===");
  
  setupPWM();
  
  if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
    startAP();
  }
  
  server.on("/", HTTP_GET, handleRoot);
  server.on("/settings", HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.on("/control", HTTP_GET, handleControl);
  
  server.on("/config.html", HTTP_GET, []() {
    handleStaticFile("/config.html", "text/html");
  });
  
  server.onNotFound([]() {
    server.send(404, "text/plain", "404 Not Found");
    Serial.println("❌ 404: " + server.uri());
  });
  
  server.begin();
  Serial.println("✅ HTTP server started on port 80");
  Serial.println("📡 Access: http://192.168.4.1");
  
  for (;;) {
    server.handleClient();
    
    if (connecting) {
      wl_status_t st = WiFi.status();
      
      if (st == WL_CONNECTED) {
        Serial.println("\n✅ WiFi STA connected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        isWifiConnected = true;
        connecting = false;
        isAPMode = false;
        
        if (xBinarySemaphoreInternet != NULL) {
          xSemaphoreGive(xBinarySemaphoreInternet);
          Serial.println("✅ Semaphore given");
        }
      }
      else if (millis() - connect_start_ms > 15000) {
        Serial.println("\n❌ WiFi timeout! Back to AP mode");
        
        connecting = false;
        isWifiConnected = false;
        WiFi.disconnect(true);
        startAP();
      }
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}