#include "printLCD.h"

// Định nghĩa ký tự đặc biệt
byte thermometer[8] = { B00100, B01010, B01010, B01110, B01110, B11111, B11111, B01110 };
byte waterdrop[8]   = { B00100, B00100, B01010, B01010, B10001, B10001, B10001, B01110 };

bool button_flag = 0;
bool lastState = 0;
bool currentState;

TaskHandle_t waterTaskHandle = NULL;
TaskHandle_t tempHumidTaskHandle = NULL;
SemaphoreHandle_t PrintOnLCDSemaphore = xSemaphoreCreateMutex();

void reportTempAndHumidity(void* pvParameters){
    
    lcd.createChar(0, thermometer);
    lcd.createChar(1, waterdrop);
    
    static bool lcdScreenToggle = true; 
    TempHumid receiver; // Biến hứng dữ liệu từ Queue

    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        TickType_t startTime = xTaskGetTickCount();

        if(xSemaphoreTake(PrintOnLCDSemaphore, portMAX_DELAY)){
            
            while(xTaskGetTickCount() - startTime < 5000){
                
                float t = 0;
                float h = 0;

                // ƯU TIÊN 1: Đọc từ Queue (Chuẩn struct)
                if (TempHumidQueue != NULL && xQueuePeek(TempHumidQueue, &receiver, 0) == pdTRUE) {
                    t = receiver.temperature;
                    h = receiver.humidity;
                } 
                // ƯU TIÊN 2: Nếu Queue lỗi thì đọc biến toàn cục
                else {
                    t = temperature;
                    h = humidity;
                }

                lcd.clear();

                // Logic hiển thị Credit 3
                if (t > 32.0) { // Nguy hiểm
                    lcd.setCursor(0, 0); lcd.print("!! NGUY HIEM !!");
                    lcd.setCursor(0, 1); lcd.print("T: "); lcd.print(t); lcd.write(0);
                    lcd.blink(); 
                    vTaskDelay(500); // Delay nhỏ để thấy blink
                }
                else if (t >= 28.0 && t <= 32.0) { // Cảnh báo
                    lcd.setCursor(0, 0); lcd.print("! CANH BAO !");
                    lcd.setCursor(0, 1); lcd.print("T: "); lcd.print(t); lcd.write(0);
                    lcd.noBlink();
                    vTaskDelay(1000);
                }
                else { // Bình thường
                    lcd.noBlink();
                    if (lcdScreenToggle) {
                        lcd.setCursor(0, 0); lcd.print("Nhiet Do");
                        lcd.setCursor(0, 1); lcd.write(0); lcd.print(" "); lcd.print(t); lcd.print(" C");
                    } else {
                        lcd.setCursor(0, 0); lcd.print("Do Am");
                        lcd.setCursor(0, 1); lcd.write(1); lcd.print(" "); lcd.print(h); lcd.print(" %");
                    }
                    lcdScreenToggle = !lcdScreenToggle;
                    vTaskDelay(2500); // Đổi màn hình mỗi 2.5s
                }
            }
            xSemaphoreGive(PrintOnLCDSemaphore);
        }
    }
}

// --- TASK HIỂN THỊ NƯỚC (GIỮ NGUYÊN CẤU TRÚC NHÓM) ---
void reportWaterAmount(void* pvParameters){
    int value = 0; // Biến lưu giá trị nước

    while(1){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        TickType_t startTime = xTaskGetTickCount();
        
        if(xSemaphoreTake(PrintOnLCDSemaphore, portMAX_DELAY)){
            lcd.noBlink(); // Tắt blink từ task trên nếu có
            
            while(xTaskGetTickCount() - startTime < 5000){
                
                // Đọc dữ liệu nước (Giả sử bạn có Queue này từ nhóm)
                // Nếu chưa có file waterSensor.h thì đoạn này có thể báo lỗi
                // Bạn có thể comment tạm logic đọc Queue nếu chưa có
                /*
                if(waterValueQueue != NULL && xQueuePeek(waterValueQueue, &value, 0) == pdTRUE) {
                    // Lấy được dữ liệu
                }
                */

                // Hiển thị đơn giản
                lcd.clear();
                lcd.setCursor(0,0); lcd.print("Water Monitor");
                lcd.setCursor(0,1); lcd.print("Val: "); lcd.print(value);
                
                vTaskDelay(1000);
            }
            xSemaphoreGive(PrintOnLCDSemaphore);
        }
    }
}

// --- TASK ĐIỀU PHỐI (GIỮ NGUYÊN) ---
void switchLCD(void* pvParameters){
    while(1){
        // Bật Task Nhiệt/Ẩm
        if(tempHumidTaskHandle != NULL) xTaskNotifyGive(tempHumidTaskHandle);
        vTaskDelay(5000);

        // Bật Task Nước
        if(waterTaskHandle != NULL) xTaskNotifyGive(waterTaskHandle);
        vTaskDelay(5000);
    }   
}