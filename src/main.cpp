#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "temp_humi_sht30_oled.h"
#include "mainserver.h"
#include "tinyml.h"

#include "coreiot.h"
#include "config_coreiot.h"
#include "task_webserver.h"
#include "task_mqtt.h"
#include "task_check_info.h"
#include "task_wifi.h"

#include "printTerminal.h"
#include "waterSensor.h"
#include "fanControl.h"

// include task

#define LED_PIN 48
#define SENSOR_PIN 4

void setup()
{
  delay(3000);
  Serial.begin(115200);
  lcd.begin();
  Serial.println("Booted successfully");

   // ✅ 1. Mount LittleFS FIRST
    if (!LittleFS.begin(true)) {
      Serial.println("❌ LittleFS Mount Failed");
      return;
  }
  Serial.println("✅ LittleFS Mounted");

  // ✅ 2. Create Semaphore BEFORE any task
  xBinarySemaphoreInternet = xSemaphoreCreateBinary();
  xTempHumiSemaphore = xSemaphoreCreateBinary();
  xHumidityMutex = xSemaphoreCreateMutex();
  QueueHandle_t TempHumidQueue = xQueueCreate(1, sizeof(TempHumid));
  QueueHandle_t PredictQueue = xQueueCreate(1, sizeof(PredictData));
  if (TempHumidQueue == NULL) Serial.println("❌ Failed to create TempHumidQueue");
  if (xBinarySemaphoreInternet == NULL || xTempHumiSemaphore == NULL || xHumidityMutex == NULL) {
      Serial.println("❌ Failed to create one of the semaphores!");
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
      coreiot_client_id = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
      coreiot_username = "";
      coreiot_password = "";
      saveCoreIOTConfig();
  }
  loadCoreIOTConfig();

    // ✅ 5. Check WiFi info file
  if (check_info_File(0)) {
    // Try STA immediately if credentials exist
    startSTA(false);
  }

  //startSTA();
  // pinMode(LED_GPIO, OUTPUT);
  //check_info_File(0);

  //--IOAS Sensor--//
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT); // BLUE
  pinMode(LED2_PIN, OUTPUT); // GREEN

  xTaskCreate(led_blinky, "Task LED Blink", 2048, NULL, 2, NULL);
  xTaskCreate(neo_blinky, "Task NEO Blink", 4096, NULL, 2, NULL);
  xTaskCreate(temp_humi_monitor, "Task TEMP HUMI Monitor", 4096, NULL, 2, NULL);
  //xTaskCreatePinnedToCore(reportTempAndHumidity, "Report T and H", 8192, NULL, 3, &tempHumidTaskHandle, 0);
  //xTaskCreatePinnedToCore(waterSensing, "Water sensing", 2048, NULL, 4, NULL, 0);
  //xTaskCreatePinnedToCore(reportWaterAmount, "Report Water Amount", 8192, NULL, 3, &waterTaskHandle, 1);
  //xTaskCreatePinnedToCore(switchLCD, "Switching messages", 8192, NULL, 2, NULL, 1);
  // xTaskCreatePinnedToCore(printTH, "Print Temp and Humidity", 2048, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(printCondition, "Print Condition", 2048, NULL, 1, NULL, 1);
   xTaskCreatePinnedToCore(tiny_ml_task, "Tiny ML Task" ,8192  ,NULL  ,3 , NULL, 0);
  // xTaskCreatePinnedToCore(coreiot_task, "CoreIOT Task" ,8192  ,NULL  ,2 , NULL, 0);
  // xTaskCreatePinnedToCore(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 1, NULL, 0);
  // xTaskCreatePinnedToCore(fanDrive, "Controling fan", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(main_server_task, "MainServer80", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(task_mqtt, "MQTT", 4096, NULL, 1, NULL, 1);
  //xTaskCreatePinnedToCore(temp_humi_sht30_oled, "Temp Humidity SHT30", 8192, NULL, 4, NULL, 0);
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