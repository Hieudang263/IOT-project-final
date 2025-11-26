#include <Arduino.h>
#include <WiFi.h>
#include "coreiot.h"
#include "config_coreiot.h"
#include "global.h"

void task_mqtt(void *pv) {
    Serial.println("=== MQTT task start ===");
    // ✅ Đợi semaphore Internet trước khi chạy MQTT
    Serial.println("⏳ Đợi WiFi kết nối...");
    if (xBinarySemaphoreInternet != NULL) {
        xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY);
        Serial.println("✅ WiFi đã sẵn sàng, bắt đầu MQTT task");
    }

    unsigned long lastPublish = 0;
    bool testMode = true;           // Chế độ test khi chưa có sensor
    static bool errorLogged = false;  // ✅ FIXED: Khai báo NGOÀI các nhánh if

    for (;;) {
        // ✅ Điều kiện hợp lệ để chạy MQTT loop
        if (WiFi.isConnected() &&
            coreiot_server != "" &&
            coreiot_port > 0 &&
            coreiot_client_id != "" &&
            coreiot_username != "") 
        { 
            coreiot_loop(); 

            // ✅ Reset logged flag khi kết nối OK
            errorLogged = false;

            // ✅ GỬI TELEMETRY MỖI 10 GIÂY
            unsigned long now = millis();
            if (now - lastPublish >= 10000) {
                lastPublish = now;

                // ✅ KIỂM TRA CÓ SENSOR (logic chặt chẽ hơn)
                bool hasSensor = (!isnan(glob_temperature) && 
                                  !isnan(glob_humidity) && 
                                  glob_temperature != -1 && 
                                  glob_humidity != -1 &&
                                  glob_temperature != 0 &&  // ✅ Tránh giá trị khởi tạo
                                  glob_humidity != 0);

                String json;
                if (hasSensor) {
                    // ✅ CÓ SENSOR: Gửi dữ liệu thật
                    json = "{\"temperature\":" + String(glob_temperature, 1) + 
                           ",\"humidity\":" + String(glob_humidity, 1) + 
                           ",\"status\":\"sensor_active\"}";
                    
                    Serial.println("\n📤 Publishing REAL sensor data:");
                    Serial.println("   Temperature: " + String(glob_temperature, 1) + "°C");
                    Serial.println("   Humidity: " + String(glob_humidity, 1) + "%");
                    
                    testMode = false;
                } 
                else {
                    // ✅ KHÔNG CÓ SENSOR
                    if (testMode) {
                        json = "{\"message\":\"hello this is test data\",\"status\":\"test_mode\",\"timestamp\":" + String(millis()) + "}";
                        Serial.println("\n🧪 Publishing TEST data (no sensor detected)");
                    } else {
                        json = "{\"status\":\"sensor_lost\",\"temperature\":0,\"humidity\":0}";
                        Serial.println("\n⚠️ Publishing SENSOR LOST warning");
                    }
                }

                Serial.println("   JSON: " + json);
                publishData(json);
            }
        } 
        else 
        {
            // ✅ Log lỗi CHỈ MỘT LẦN (nhưng có thể log lại sau khi reconnect)
            if (!errorLogged) {
                if (coreiot_server == "" || 
                    coreiot_port == 0 || 
                    coreiot_client_id == "" || 
                    coreiot_username == "") 
                {
                    Serial.println("⚠️ CoreIOT config chưa đầy đủ, vui lòng vào Settings để cấu hình");
                } 
                else 
                {
                    Serial.println("⚠️ WiFi mất kết nối, đợi reconnect...");
                }
                errorLogged = true;
            }
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}