#include "led_blinky.h"

// Cài đặt LEDC (PWM điều khiển độ sáng của đèn LED)
#define LEDC_CHANNEL    0 // Sử dụng Kênh số 0
#define LEDC_TIMER      13 // Chọn một bộ đếm có độ phân giải 13-bit
#define LEDC_RESOLUTION 8191 // Độ phân giải tối đa là 2^13 - 1
#define LEDC_FREQ_HZ    5000 // Đặt tần số tín hiệu PWM là 5000 Hz

// Hàm hiệu ứng "Thở"
void led_breathing_effect() {
  // Sáng dần
  for (int dutyCycle = 0; dutyCycle <= 8191; dutyCycle += 200) {
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    vTaskDelay(10);
  }
  // Mờ dần
  for (int dutyCycle = 8191; dutyCycle >= 0; dutyCycle -= 200) {
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    vTaskDelay(10);
  }
}

// Hàm hiệu ứng nháy (tắt PWM)
void led_digital_blink(int delay_ms, int times) {
  for (int i = 0; i < times; i++) {
    ledcWrite(LEDC_CHANNEL, 8191); // Bật sáng tối đa
    vTaskDelay(delay_ms);
    ledcWrite(LEDC_CHANNEL, 0); // Tắt hẳn
    vTaskDelay(delay_ms);
  }
}

void led_blinky(void *pvParameters){
  // pinMode(LED_GPIO, OUTPUT); 
  
  // Cài đặt LEDC
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, 13);
  ledcAttachPin(LED_GPIO, LEDC_CHANNEL);

  while(1) {
    // Chờ Semaphore từ Task 3
    if (xSemaphoreTake(xTempHumiSemaphore, portMAX_DELAY) == pdTRUE) {
      
      // Đã nhận được tín hiệu!
      float currentTemp = glob_temperature;

      // Trạng thái 1: Nguy hiểm (> 32°C) - SOS
      if (currentTemp > 32.0) {
        // SOS (3 nhanh, 3 chậm, 3 nhanh)
        led_digital_blink(100, 3); // 3 nhanh
        vTaskDelay(100);
        led_digital_blink(300, 3); // 3 chậm
        vTaskDelay(100);
        led_digital_blink(100, 3); // 3 nhanh
        vTaskDelay(1000);
      }
      // Trạng thái 2: Cảnh báo (28-32°C) - Nhịp tim
      else if (currentTemp >= 28.0 && currentTemp <= 32.0) {
        // Nhịp tim (chớp - chớp - nghỉ dài)
        led_digital_blink(100, 2); // Nháy 2 lần nhanh
        vTaskDelay(500);
      }
      // Trạng thái 3: Bình thường (< 28°C) - Hiệu ứng "Thở"
      else {
        led_breathing_effect();
      }
    }
  }
}