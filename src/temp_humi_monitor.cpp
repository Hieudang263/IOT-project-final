#include "temp_humi_monitor.h"
DHT20 dht20;

void temp_humi_monitor(void *pvParameters){

    Wire.begin(21, 22);
    dht20.begin();

    while (1){
        // ĐỌC DỮ LIỆU TỪ CẢM BIẾN
        dht20.read();
        float temp_val = dht20.getTemperature();
        float humi_val = dht20.getHumidity();

        // GHI DỮ LIỆU VÀO BIẾN TOÀN CỤC (AN TOÀN)
        // Kiểm tra xem Mutex đã được tạo chưa để tránh lỗi Crash
        if (xHumidityMutex != NULL) {
            // Xin quyền truy cập (Chờ tối đa thời gian cho phép)
            if (xSemaphoreTake(xHumidityMutex, portMAX_DELAY) == pdTRUE) {
                
                // Khi đã có khóa, cập nhật dữ liệu
                glob_temperature = temp_val;
                glob_humidity = humi_val;

                // Trả khóa ngay lập tức
                xSemaphoreGive(xHumidityMutex);
            }
        } 
        else {
            // Trường hợp dự phòng nếu quên tạo Mutex (để không bị treo)
            glob_temperature = temp_val;
            glob_humidity = humi_val;
        }

        // BÁO HIỆU CHO TASK LED ĐƠN
        // Đánh thức Task LED Blink để nó cập nhật trạng thái đèn ngay
        if (xTempHumiSemaphore != NULL) {
            xSemaphoreGive(xTempHumiSemaphore);
        }

        // Đợi 3 giây trước khi đọc lần tiếp theo
        vTaskDelay(3000 / portTICK_PERIOD_MS); 
    }
}