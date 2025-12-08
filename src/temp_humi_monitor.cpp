#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(0x21, 16, 2);

// Ký tự nhiệt độ và độ ẩm tùy chỉnh
byte thermometer[8] = { B00100, B01010, B01010, B01110, B01110, B11111, B11111, B01110 };
byte waterdrop[8]   = { B00100, B00100, B01010, B01010, B10001, B10001, B10001, B01110 };

// Biến quản lý trạng thái LCD
bool lcdScreenToggle = true; // true = show temp, false = show humi

void temp_humi_monitor(void *pvParameters){

    Wire.begin(21, 22);
    //Serial.begin(115200);
    dht20.begin();

    // Khởi tạo LCD
    lcd.begin();
    lcd.backlight();
    lcd.clear();

    // Khởi tạo ký tự tùy chỉnh
    lcd.createChar(0, thermometer);
    lcd.createChar(1, waterdrop);

    while (1){
        /* code */
        
        dht20.read();

        // Đọc dữ liệu từ cảm biến vào biến tạm thời
        float temp_val = dht20.getTemperature();
        float humi_val = dht20.getHumidity();
        
        // Xin quyền truy cập biến toàn cục
        if (xSemaphoreTake(xHumidityMutex, portMAX_DELAY) == pdTRUE) {
            
            // Đã có khóa, an toàn để GHI dữ liệu
            glob_temperature = temp_val;
            glob_humidity = humi_val;

            // Trả lại chìa khóa cho Task NeoPixel dùng
            xSemaphoreGive(xHumidityMutex);
        }

        // Xử lý 3 trạng thái LCD
        lcd.clear();

        // Trạng thái 1: Nguy hiểm
        if (temp_val > 32.0) {
            lcd.setCursor(0, 0); // Di chuyển con trỏ (vị trí viết chữ) màn hình LCD về cột 0, hàng 0(góc trên cùng bên trái)
            lcd.print("!! NGUY HIEM !!");
            lcd.setCursor(0, 1);
            lcd.print("T: "); lcd.print(temp_val);
            lcd.write(0); // Hiển thị biểu tượng nhiệt kế
            lcd.blink(); // Nhấp nháy con trỏ
        }
        // Trạng thái 2: Cảnh báo
        else if (temp_val >= 28.0 && temp_val <= 32.0) {
            lcd.setCursor(0, 0);
            lcd.print("! CANH BAO !");
            lcd.setCursor(0, 1);
            lcd.print("T: "); lcd.print(temp_val);
            lcd.write(0);
            lcd.noBlink();
        }
        // Trạng thái 3: Bình thường (Luân phiên hiển thị)
        else {
            lcd.noBlink();
            if (lcdScreenToggle) {
                // Màn hình Nhiệt độ
                lcd.setCursor(0, 0);
                lcd.print("Nhiet Do");
                lcd.setCursor(0, 1);
                lcd.write(0); // Biểu tượng nhiệt kế
                lcd.print(" ");
                lcd.print(temp_val);
                lcd.print(" C");
            } else {
                // Màn hình Độ ẩm
                lcd.setCursor(0, 0);
                lcd.print("Do Am");
                lcd.setCursor(0, 1);
                lcd.write(1); // Biểu tượng giọt nước
                lcd.print(" ");
                lcd.print(humi_val);
                lcd.print(" %");
            }
            lcdScreenToggle = !lcdScreenToggle; // Đảo trạng thái cho lần lặp sau
        }

        // Báo cho các task khác đã có dữ liệu mới
        xSemaphoreGive(xTempHumiSemaphore);

        vTaskDelay(3000); // Đọc cảm biến và cập nhật LCD mỗi 3 giây
    }
    
}