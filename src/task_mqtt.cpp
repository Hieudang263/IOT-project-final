#include <Arduino.h>
#include <WiFi.h>
#include "coreiot.h"
#include "config_coreiot.h"
#include "global.h"

void task_mqtt(void *pv) {
    Serial.println("=== MQTT task start ===");
    Serial.println("⏳ Waiting for WiFi connection...");
    
    if (xBinarySemaphoreInternet != NULL) {
        xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY);
        Serial.println("✅ WiFi ready, starting MQTT task");
    }
    
    TempHumid th;
    PredictData pred;
    unsigned long lastPublish = 0;
    static bool errorLogged = false;
    static bool lastHadSensor = false;
    
    for (;;) {
        // ✅ Lấy dữ liệu cảm biến và dự đoán
        bool hasTempHumid = (TempHumidQueue != NULL && xQueuePeek(TempHumidQueue, &th, 0) == pdTRUE);
        bool hasPredictData = (PredictQueue != NULL && xQueuePeek(PredictQueue, &pred, 0) == pdTRUE);
        
        // ✅ Check if we have valid CoreIOT config and WiFi
        if (WiFi.isConnected() &&
            coreiot_server != "" &&
            coreiot_port > 0 &&
            coreiot_client_id != "" &&
            coreiot_username != "") 
        { 
            coreiot_loop(); 
            errorLogged = false;
            
            unsigned long now = millis();
            if (now - lastPublish >= 5000) {
                lastPublish = now;
                
                String json;
                
                // ✅ Có dữ liệu cảm biến: gửi đầy đủ
                if (hasTempHumid) {
                    json = "{\"temperature\":" + String(th.temperature, 1) + 
                           ",\"humidity\":" + String(th.humidity, 1);
                    
                    // Thêm dữ liệu dự đoán nếu có
                    if (hasPredictData && pred.has_data) {
                        json += ",\"predicted_temp\":" + String(pred.predicted_temp, 1);
                        json += ",\"predicted_humi\":" + String(pred.predicted_humi, 1);
                        json += ",\"accuracy\":" + String(pred.accuracy);
                    } else {
                        json += ",\"predicted_temp\":0";
                        json += ",\"predicted_humi\":0";
                        json += ",\"accuracy\":0";
                    }
                    
                    // Thêm tọa độ (longitude và latitude)
                    json += ",\"longitude\":" + String(coreiot_longitude, 6);
                    json += ",\"latitude\":" + String(coreiot_latitude, 6);
                    
                    json += ",\"status\":\"sensor_active\"}";
                    
                    Serial.println("\n✅ Publishing sensor data + TinyML prediction:");
                    Serial.println("   Temperature    : " + String(th.temperature, 1) + "°C");
                    Serial.println("   Humidity       : " + String(th.humidity, 1) + "%");
                    
                    if (hasPredictData && pred.has_data) {
                        Serial.println("   Predicted Temp : " + String(pred.predicted_temp, 1) + "°C");
                        Serial.println("   Predicted Humi : " + String(pred.predicted_humi, 1) + "%");
                        Serial.println("   Accuracy       : " + String(pred.accuracy) + "%");
                    } else {
                        Serial.println("   Prediction     : Not available yet");
                    }
                    
                    lastHadSensor = true;
                } 
                // ✅ Không có dữ liệu cảm biến: gửi 0
                else {
                    json = "{\"temperature\":0"
                           ",\"humidity\":0"
                           ",\"predicted_temp\":0"
                           ",\"predicted_humi\":0"
                           ",\"accuracy\":0"
                           ",\"longitude\":" + String(coreiot_longitude, 6) +
                           ",\"latitude\":" + String(coreiot_latitude, 6) +
                           ",\"status\":\"no_sensor\"}";
                    
                    Serial.println("\n⚠️ Publishing NO SENSOR status:");
                    Serial.println("   All values set to 0");
                    
                    if (lastHadSensor) {
                        Serial.println("   ⚠️ Sensor disconnected!");
                        lastHadSensor = false;
                    }
                }
                
                Serial.println("   JSON: " + json);
                publishData(json);
            }
        } 
        else 
        {
            if (!errorLogged) {
                if (coreiot_server == "" || 
                    coreiot_port == 0 || 
                    coreiot_client_id == "" || 
                    coreiot_username == "") 
                {
                    Serial.println("⚠️ CoreIOT config missing, please fill in Settings");
                } 
                else 
                {
                    Serial.println("⚠️ WiFi disconnected, waiting to reconnect...");
                }
                errorLogged = true;
            }
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}