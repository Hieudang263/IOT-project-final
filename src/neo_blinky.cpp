#include "neo_blinky.h"

// Khởi tạo dải LED
Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);

// HÀM LÀM MỜ (TẠO HIỆU ỨNG ĐUÔI SAO CHỔI)
void fadeToBlack(int ledNo, byte fadeValue) {
    uint32_t oldColor;
    uint8_t r, g, b;
    
    oldColor = strip.getPixelColor(ledNo);
    
    r = (oldColor & 0x00FF0000UL) >> 16; // Lấy thành phần Đỏ
    g = (oldColor & 0x0000FF00UL) >> 8;  // Lấy thành phần Xanh lá
    b = (oldColor & 0x000000FFUL);       // Lấy thành phần Xanh dương

    r=(r<=10)? 0 : (int) r-(r*fadeValue/256);
    g=(g<=10)? 0 : (int) g-(g*fadeValue/256);
    b=(b<=10)? 0 : (int) b-(b*fadeValue/256);
    
    strip.setPixelColor(ledNo, r, g, b);
}

// HIỆU ỨNG 1: CẦU VỒNG XOAY
void effect_rainbow_cycle(int wait) {
    // Chạy 1 vòng màu (256 bước màu)
    for(long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
        for(int i=0; i<strip.numPixels(); i++) {
            int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
        }
        strip.show();
        vTaskDelay(wait); // Tốc độ trôi màu
    }
}

// HIỆU ỨNG 2: SAO CHỔI QUAY
void effect_comet_chase(uint32_t color, int wait) {
    for (int i = 0; i < strip.numPixels(); i++) {
        // Làm mờ tất cả các đèn cũ đi một chút (tạo đuôi)
        for(int j=0; j<strip.numPixels(); j++) {
            fadeToBlack(j, 64); // 64/256 = mờ đi 25% mỗi bước
        }

        // Bật đèn đầu tàu sáng rực
        strip.setPixelColor(i, color);
        strip.show();
        vTaskDelay(wait);
    }
}

// HIỆU ỨNG 3: CHỚP TẮT BÁO ĐỘNG
void effect_police_strobe(int count) {
    for(int j=0; j<count; j++) {
        // Chớp đỏ toàn bộ
        strip.fill(strip.Color(255, 0, 0));
        strip.show();
        vTaskDelay(50);
        
        // Tắt tối
        strip.fill(strip.Color(0, 0, 0));
        strip.show();
        vTaskDelay(50);
    }
    vTaskDelay(200);
}

void neo_blinky(void *pvParameters){
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.setBrightness(60); 
    strip.show(); 

    float currentHumi = 0.0;

    while(1) {                          
        if (xSemaphoreTake(xHumidityMutex, portMAX_DELAY) == pdTRUE) {
            // Dùng biến chuẩn "humidity"
            currentHumi = humidity;
            xSemaphoreGive(xHumidityMutex);
        }

        // CHỌN HIỆU ỨNG DỰA TRÊN ĐỘ ẨM
        
        // TRẠNG THÁI 1: KHÔ RÁO (< 60%) -> CẦU VỒNG
        if (currentHumi < 60.0) {
            // Chạy hiệu ứng cầu vồng 1 chu kỳ
            effect_rainbow_cycle(10); 
        } 
        
        // TRẠNG THÁI 2: TRUNG BÌNH (60-80%) -> SAO CHỔI VÀNG
        else if (currentHumi >= 60.0 && currentHumi <= 80.0) {
            // Chạy hiệu ứng sao chổi màu Vàng cam
            for(int k=0; k<4; k++) {
                effect_comet_chase(strip.Color(255, 160, 0), 60); 
            }
        } 
        
        // TRẠNG THÁI 3: ẨM ƯỚT/NGUY HIỂM (> 80%) -> BÁO ĐỘNG ĐỎ
        else {
            // Chớp tắt liên tục 10 lần
            effect_police_strobe(10);
        }
    }
}