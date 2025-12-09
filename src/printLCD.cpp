#include "printLCD.h"

LiquidCrystal_I2C lcd(0x21,16,2);

byte thermometer[8] = { B00100, B01010, B01010, B01110, B01110, B11111, B11111, B01110 };
byte waterdrop[8]   = { B00100, B00100, B01010, B01010, B10001, B10001, B10001, B01110 };

bool button_flag = 0;
bool lastState = 0;
bool currentState;
TaskHandle_t waterTaskHandle = NULL;
TaskHandle_t tempHumidTaskHandle = NULL;
SemaphoreHandle_t PrintOnLCDSemaphore = xSemaphoreCreateMutex();

void reportTempAndHumidity(void* pvParameters){
    
    // Tạo ký tự đặc biệt một lần khi Task bắt đầu
    lcd.createChar(0, thermometer);
    lcd.createChar(1, waterdrop);
    
    // Biến trạng thái hiển thị (dùng static để giữ giá trị qua các lần gọi)
    static bool lcdScreenToggle = true; 

    while(1){
        // Chờ tín hiệu từ switchLCD
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t startTime = xTaskGetTickCount();

        if(xSemaphoreTake(PrintOnLCDSemaphore, portMAX_DELAY)){
            
            // Chạy trong khoảng 5 giây
            while(xTaskGetTickCount() - startTime < 5000){
                
                float t = 0;
                float h = 0;
                
                // Dùng Mutex để đọc an toàn
                if(xHumidityMutex != NULL) {
                    if(xSemaphoreTake(xHumidityMutex, 100) == pdTRUE) {
                        t = glob_temperature;
                        h = glob_humidity;
                        xSemaphoreGive(xHumidityMutex);
                    }
                } else {
                    t = glob_temperature;
                    h = glob_humidity;
                }

                lcd.clear();

                
                // Trạng thái 1: Nguy hiểm (> 32 độ)
                if (t > 32.0) {
                    lcd.setCursor(0, 0); 
                    lcd.print("!! NGUY HIEM !!");
                    lcd.setCursor(0, 1);
                    lcd.print("T: "); lcd.print(t); lcd.write(0);
                    lcd.blink(); // Nhấp nháy cảnh báo
                }
                // Trạng thái 2: Cảnh báo (28-32 độ)
                else if (t >= 28.0 && t <= 32.0) {
                    lcd.setCursor(0, 0);
                    lcd.print("! CANH BAO !");
                    lcd.setCursor(0, 1);
                    lcd.print("T: "); lcd.print(t); lcd.write(0);
                    lcd.noBlink();
                }
                // Trạng thái 3: Bình thường (Luân phiên)
                else {
                    lcd.noBlink();
                    if (lcdScreenToggle) {
                        // Màn hình Nhiệt độ
                        lcd.setCursor(0, 0); lcd.print("Nhiet Do");
                        lcd.setCursor(0, 1); lcd.write(0); lcd.print(" "); lcd.print(t); lcd.print(" C");
                    } else {
                        // Màn hình Độ ẩm
                        lcd.setCursor(0, 0); lcd.print("Do Am");
                        lcd.setCursor(0, 1); lcd.write(1); lcd.print(" "); lcd.print(h); lcd.print(" %");
                    }
                    
                    // Đảo trạng thái cho lần hiển thị tiếp theo
                    lcdScreenToggle = !lcdScreenToggle;
                }

                // Delay một chút để màn hình không bị nháy liên tục
                vTaskDelay(2500); 
            }
            
            xSemaphoreGive(PrintOnLCDSemaphore);
        }
    }
}

void reportWaterAmount(void* pvParameters){
    int value;

    while(1){

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        TickType_t startTime = xTaskGetTickCount();
        
        if(xSemaphoreTake(PrintOnLCDSemaphore, portMAX_DELAY)){
            lcd.noBlink();
            while(xTaskGetTickCount() - startTime < 5000){
                if(xSemaphoreTake(xWaterSemaphore, portMAX_DELAY)){
                    xQueuePeek(waterValueQueue, &value, 100);
                    currentState = digitalRead(SENSOR_PIN);
                    if(currentState != lastState){
                        button_flag = !button_flag;
                        lastState = currentState;
                        vTaskDelay(10);
                    }
                        if(button_flag){
                        lcd.clear();
                        lcd.home();
                        lcd.print("ADC value: ");
                        lcd.print(value);
                    }
                    else{
                        lcd.clear();
                        lcd.home();
                        if(value > 2000){
                            lcd.setCursor(0,0);
                            lcd.print("Sensor Wet");
                        }
                        else if(value > 1000){
                            lcd.setCursor(0,0);
                            lcd.print("Sensor half wet");
                        }
                        else{
                            lcd.setCursor(0,0);
                            lcd.print("Sensor dry");
                        }
                    }
                    lcd.clear();
                     lcd.setCursor(0,0);
                     lcd.print("Checking Water...");
                }
                vTaskDelay(500);
            }
        }
        xSemaphoreGive(PrintOnLCDSemaphore);
    }
}

void switchLCD(void* pvParameters){
    while(1){
        xTaskNotifyGive(tempHumidTaskHandle);
        vTaskDelay(5000);

        xTaskNotifyGive(waterTaskHandle);
        vTaskDelay(5000);
    }
}