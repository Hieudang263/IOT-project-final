#include "temp_humi_monitor.h"

DHT20 dht20;

void temp_humi_monitor(void *pvParameters){

    Wire.begin(21, 22);
    dht20.begin();

    while (1){
        dht20.read();
        float temp_val = dht20.getTemperature();
        float humi_val = dht20.getHumidity();

        // CẬP NHẬT BIẾN TOÀN CỤC (Cho LED/NeoPixel dùng ngay)
        if (xHumidityMutex != NULL) {
            if (xSemaphoreTake(xHumidityMutex, portMAX_DELAY) == pdTRUE) {
                temperature = temp_val; // Đã đổi thành temperature
                humidity = humi_val;    // Đã đổi thành humidity
                xSemaphoreGive(xHumidityMutex);
            }
        } else {
            temperature = temp_val;
            humidity = humi_val;
        }

        // GỬI VÀO QUEUE (Cho LCD và WebServer dùng)
        if (TempHumidQueue != NULL) {
            TempHumid data;
            data.temperature = temp_val;
            data.humidity = humi_val;
            // Ghi đè giá trị mới nhất vào Queue
            xQueueOverwrite(TempHumidQueue, &data);
        }

        // ĐÁNH THỨC TASK LED
        if (xTempHumiSemaphore != NULL) {
            xSemaphoreGive(xTempHumiSemaphore);
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS); 
    }
}