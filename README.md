# 🚀 ESP32 Merged System - Complete Guide

## 📌 Tổng quan

Đây là hệ thống **MERGED** hoàn chỉnh từ 2 repository GitHub của bạn:
- **Port 80**: AP Mode + LED PWM Control (Task 4)
- **Port 8080**: Full Dashboard với WebSocket + MQTT

---

## 🏗️ Kiến trúc hệ thống

```
ESP32 Dual-Port System
│
├── 📡 Port 80 (WebServer) ─────────────── AP Mode Configuration
│   ├── GET /                → config.html (LED control)
│   ├── GET /settings        → WiFi setup form
│   ├── GET /connect         → Save WiFi credentials
│   └── GET /control         → LED PWM control (0-100%)
│
├── 🌐 Port 8080 (AsyncWebServer) ──────── Full Dashboard
│   ├── WebSocket /ws        → Real-time data
│   ├── GET /                → index.html (dashboard)
│   ├── GET /script.js       → Frontend logic
│   ├── GET /styles.css      → Styles
│   └── API /api/coreiot/*   → MQTT configuration
│
└── 🔧 FreeRTOS Tasks
    ├── main_server_task     → Port 80 (config + LED)
    ├── task_mqtt            → MQTT CoreIOT
    ├── temp_humi_monitor    → DHT20 sensor (optional)
    ├── led_blinky           → LED test (optional)
    ├── neo_blinky           → NeoPixel test (optional)
    ├── tiny_ml_task         → TensorFlow Lite (optional)
    └── Task_Toogle_BOOT     → Factory reset (optional)
```

---

## 📦 Cấu trúc thư mục

```
project/
├── data/
│   ├── config.html      ← ✅ NEW: AP Mode LED control
│   ├── index.html       ← Port 8080 dashboard
│   ├── script.js        ← Dashboard JS
│   └── styles.css       ← Dashboard CSS
│
├── include/
│   ├── mainserver.h     ← Port 80 server
│   ├── task_webserver.h ← Port 8080 server
│   ├── config_coreiot.h ← MQTT config
│   ├── coreiot.h        ← MQTT client
│   └── ...              ← Other headers
│
└── src/
    ├── main.cpp         ← ✅ MERGED: Complete setup
    ├── mainserver.cpp   ← ✅ MERGED: Port 80 + LED PWM
    ├── task_webserver.cpp ← Port 8080 server
    ├── config_coreiot.cpp ← MQTT config loader
    ├── coreiot.cpp      ← MQTT client
    └── ...              ← Other source files
```

---

## ⚙️ Cài đặt

### 1️⃣ Upload LittleFS

```bash
# PlatformIO
pio run --target uploadfs

# Arduino IDE
Tools → ESP32 Sketch Data Upload
```

**Files cần có trong `data/`:**
- ✅ `config.html` (Port 80 - LED control)
- ✅ `index.html` (Port 8080 - Dashboard)
- ✅ `script.js` (Dashboard logic)
- ✅ `styles.css` (Dashboard styles)

### 2️⃣ Upload Code

```bash
# PlatformIO
pio run --target upload

# Arduino IDE
Sketch → Upload (Ctrl+U)
```

### 3️⃣ Monitor Serial

```bash
# PlatformIO
pio device monitor

# Arduino IDE
Tools → Serial Monitor (115200 baud)
```

---

## 🎯 Cách sử dụng

### **Chế độ 1: AP Mode (Lần đầu khởi động)**

1. ESP32 khởi động ở AP Mode:
   - **SSID:** `ESP32-Setup-Wifi`
   - **Password:** `123456789`
   - **IP:** `192.168.4.1`

2. Kết nối điện thoại/laptop vào WiFi ESP32

3. Truy cập: `http://192.168.4.1`

4. **Giao diện config.html:**
   - Điều khiển LED 1 (GPIO 48)
   - Điều khiển LED 2 (GPIO 41)
   - Slider độ sáng 0-100%
   - Nút "Cấu hình Wi-Fi"

5. Click "⚙️ Cấu hình Wi-Fi" → Nhập SSID + Password → Kết nối

---

### **Chế độ 2: STA Mode (Sau khi kết nối WiFi)**

1. ESP32 kết nối vào WiFi nhà

2. Kiểm tra IP trên Serial Monitor:
   ```
   ✅ WiFi STA connected!
   IP: 192.168.1.xxx
   ```

3. Truy cập:
   - **Port 80:** `http://192.168.1.xxx` (config.html)
   - **Port 8080:** `http://192.168.1.xxx:8080` (dashboard)

4. **Dashboard Port 8080:**
   - 🏠 Trang chủ: Gauge nhiệt độ & độ ẩm
   - ⚡ Thiết bị: Điều khiển Relay
   - ℹ️ Thông tin: System info
   - ⚙️ Cài đặt: MQTT CoreIOT config

---

## 🔧 Cấu hình Tasks

Trong `main.cpp`, uncomment các task cần dùng:

```cpp
void setup() {
  // ...
  
  // ✅ REQUIRED: Port 80 + LED Control
  xTaskCreate(main_server_task, "Task Main Server (Port 80)", 8192, NULL, 2, NULL);
  
  // ✅ REQUIRED: MQTT CoreIOT
  xTaskCreate(task_mqtt, "MQTT Task", 4096, NULL, 1, NULL);
  
  // 🔧 OPTIONAL: Sensor monitoring (uncomment if DHT20 connected)
  // xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);
  
  // 🔧 OPTIONAL: LED blink test
  // xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  
  // 🔧 OPTIONAL: NeoPixel test
  // xTaskCreate(neo_blinky, "Task NEO Blink", 2048, NULL, 2, NULL);
  
  // 🔧 OPTIONAL: TensorFlow Lite
  // xTaskCreate(tiny_ml_task, "Tiny ML Task", 2048, NULL, 2, NULL);
  
  // 🔧 OPTIONAL: Factory reset (hold BOOT 2s)
  // xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}
```

---

## 🌐 API Endpoints

### **Port 80 (WebServer)**

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | config.html (LED control) |
| GET | `/settings` | WiFi setup form |
| GET | `/connect?ssid=X&pass=Y` | Save WiFi credentials |
| GET | `/control?device=X&state=ON/OFF&brightness=Y` | LED PWM control |

**Example:**
```bash
# LED 1 ON at 75% brightness
curl "http://192.168.4.1/control?device=1&state=ON&brightness=75"

# LED 2 OFF
curl "http://192.168.4.1/control?device=2&state=OFF&brightness=0"
```

---

### **Port 8080 (AsyncWebServer)**

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Dashboard (index.html) |
| GET | `/api/coreiot/config` | Get MQTT config |
| POST | `/api/coreiot/config` | Save MQTT config |
| GET | `/api/coreiot/status` | MQTT connection status |
| WebSocket | `/ws` | Real-time data stream |

**Example:**
```bash
# Get MQTT config
curl "http://192.168.1.100:8080/api/coreiot/config"

# Save MQTT config
curl -X POST "http://192.168.1.100:8080/api/coreiot/config" \
  -H "Content-Type: application/json" \
  -d '{"server":"app.coreiot.io","port":1883,"client_id":"ESP32_ABC","username":"test","password":"123456"}'
```

---

## 🎨 Customization

### **Đổi AP SSID & Password**

File: `src/global.cpp`

```cpp
String ssid = "ESP32-Setup-Wifi";     // Change this
String password = "123456789";        // Change this
```

### **Đổi LED GPIO Pins**

File: `src/mainserver.cpp`

```cpp
#define LED1_PIN 48   // Change to your pin
#define LED2_PIN 41   // Change to your pin
```

### **Đổi PWM Settings**

File: `src/mainserver.cpp`

```cpp
#define PWM_FREQ 5000           // Frequency (Hz)
#define PWM_RESOLUTION 8        // 0-255 (8-bit)
```

---

## 🐛 Troubleshooting

### ❌ **Không kết nối được AP Mode**

**Nguyên nhân:** ESP32 chưa khởi động xong

**Giải pháp:**
1. Reset ESP32 (nút RST)
2. Đợi 5 giây
3. Tìm WiFi `ESP32-Setup-Wifi`

---

### ❌ **LED không sáng khi điều khiển**

**Nguyên nhân:** Chưa kết nối LED đúng chân

**Giải pháp:**
```
LED1 Anode → GPIO 48 → Điện trở 220Ω → GND
LED2 Anode → GPIO 41 → Điện trở 220Ω → GND
```

---

### ❌ **Port 8080 không mở được**

**Nguyên nhân:** ESP32 chưa kết nối WiFi STA

**Giải pháp:**
1. Kiểm tra Serial Monitor:
   ```
   ✅ WiFi STA connected!
   IP: 192.168.1.xxx
   ```
2. Nếu chưa kết nối → vào `http://192.168.4.1/settings` để config WiFi

---

### ❌ **MQTT không kết nối**

**Nguyên nhân:** Chưa cấu hình CoreIOT

**Giải pháp:**
1. Truy cập: `http://192.168.1.xxx:8080`
2. Click tab "⚙️ Cài đặt"
3. Điền thông tin MQTT:
   - Server: `app.coreiot.io`
   - Port: `1883`
   - Client ID: `ESP32_XXX`
   - Username: `your_username`
   - Password: `your_password`
4. Click "💾 Lưu cấu hình"

---

## 📊 Task Priorities

```cpp
Priority 2 (High):
├── main_server_task      → Port 80 config
├── temp_humi_monitor     → Sensor đọc

Priority 1 (Medium):
└── task_mqtt             → MQTT client

Priority 0 (Low):
├── led_blinky            → LED test
├── neo_blinky            → NeoPixel test
└── tiny_ml_task          → TensorFlow
```

---

## 🎯 Task 4 Requirements ✅

### **✅ Redesign web interface**
- Giao diện `config.html` hiện đại, responsive

### **✅ Control 2 devices**
- LED1 (GPIO 48)
- LED2 (GPIO 41)

### **✅ At least 2 buttons**
- Toggle ON/OFF cho mỗi LED (2 buttons)
- Slider brightness cho mỗi LED (2 sliders)
- Button "Cấu hình Wi-Fi" (1 button)

### **✅ Labeled controls**
- "LED 1 (GPIO 48)"
- "LED 2 (GPIO 41)"
- "Độ sáng: 50%"

---

## 📝 Notes

- Port 80 hoạt động ở cả **AP Mode** và **STA Mode**
- Port 8080 chỉ hoạt động ở **STA Mode** (cần kết nối WiFi)
- LED PWM real-time, không cần WebSocket
- MQTT chỉ hoạt động sau khi cấu hình trong Settings

---

## ✅ Checklist Deployment

- [ ] Upload LittleFS (`data/` folder)
- [ ] Upload firmware code
- [ ] Kiểm tra Serial Monitor output
- [ ] Kết nối WiFi ESP32 (AP Mode)
- [ ] Test LED control trên `http://192.168.4.1`
- [ ] Cấu hình WiFi nhà
- [ ] Kiểm tra kết nối STA Mode
- [ ] Test dashboard trên `http://192.168.1.xxx:8080`
- [ ] Cấu hình MQTT CoreIOT
- [ ] Test sensor monitoring (nếu có)

---

## 🚀 Done!

Bạn đã có hệ thống ESP32 hoàn chỉnh với:
- ✅ AP Mode config + LED control (Port 80)
- ✅ Full dashboard với WebSocket (Port 8080)
- ✅ MQTT CoreIOT integration
- ✅ Sensor monitoring
- ✅ Task management

**Happy coding! 🎉**