#include "global.h"
#include "config_coreiot.h"
#include "coreiot.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "mainserver.h"
#include "tinyml.h"

#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_mqtt.h"

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\n========================================");
  Serial.println("🚀 ESP32 BOOT OK - COMBINED VERSION");
  Serial.println("========================================\n");
  
  // ✅ 1. Mount LittleFS FIRST
  if (!LittleFS.begin(true)) {
      Serial.println("❌ LittleFS Mount Failed");
      return;
  }
  Serial.println("✅ LittleFS Mounted");
  
  // ✅ 2. Create Semaphore BEFORE any task
  xBinarySemaphoreInternet = xSemaphoreCreateBinary();
  if (xBinarySemaphoreInternet == NULL) {
      Serial.println("❌ Failed to create semaphore!");
      return;
  }
  Serial.println("✅ Semaphore created");
  
  // ✅ 3. Initialize WiFi FIRST (CRITICAL!)
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP_STA);  // Enable both AP and STA
  delay(500);  // Wait for WiFi stack to initialize
  Serial.println("✅ WiFi stack initialized (AP+STA mode)");
  
  // ✅ 4. Load configs
  if (!LittleFS.exists("/coreiot.json")) {
      Serial.println("⚠️ Creating default coreiot.json...");
      coreiot_server = "app.coreiot.io";
      coreiot_port = 1883;
      coreiot_client_id = "";  
      coreiot_username = "";
      coreiot_password = "";
      saveCoreIOTConfig();
  }
  loadCoreIOTConfig();
  
  // ✅ 5. Check WiFi info file
  if (check_info_File(0)) {
    // Try STA immediately if credentials exist
    startSTA(true);
  }
  
  // ✅ 6. Create tasks with proper stack sizes
  Serial.println("\n📋 Creating tasks...");
  
  // Port 80: AP Mode Configuration Server (ALWAYS ON)
  xTaskCreatePinnedToCore(main_server_task,"MainServer80",8192,NULL,2,NULL,1);
  Serial.println("   ✅ MainServer (Port 80) task created (Core 1)");
  
  // MQTT Task
  xTaskCreatePinnedToCore(task_mqtt,"MQTT",4096,NULL,1,NULL,1);
  Serial.println("   ✅ MQTT task created (Core 1)");
  
  // Optional tasks (uncomment if needed):
  // xTaskCreate(temp_humi_monitor, "TempHumi", 4096, NULL, 1, NULL);
  // xTaskCreate(Task_Toogle_BOOT, "BootBtn", 4096, NULL, 1, NULL);
  // xTaskCreate(tiny_ml_task, "TinyML", 2048, NULL, 1, NULL);
  
  Serial.println("\n========================================");
  Serial.println("✅ All tasks created successfully!");
  Serial.println("📡 Port 80: AP Configuration (config.html)");
  Serial.println("📡 Port 8080: Dashboard (index.html) - starts after WiFi STA connected");
  Serial.println("========================================\n");
}

void loop()
{
  static unsigned long lastCheck = 0;
  unsigned long now = millis();
  
  // Check every 10 seconds
  if (now - lastCheck > 10000) {
    lastCheck = now;
    
    if (check_info_File(1)) {
      if (!Wifi_reconnect()) {
        Webserver_stop();  // Stop port 8080 if WiFi lost
      } else {
        CORE_IOT_reconnect();
      }
    }
  }
  
  // Port 8080 will auto-start when WiFi STA connects
  Webserver_reconnect();
  
  // ✅ CRITICAL: Add delay to prevent watchdog reset
  vTaskDelay(100 / portTICK_PERIOD_MS);
}
