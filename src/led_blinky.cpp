#include "led_blinky.h"

#define LEDC_CHANNEL    0 
#define LEDC_TIMER      13 
#define LEDC_RESOLUTION 8191 
#define LEDC_FREQ_HZ    5000 

void led_breathing_effect() {
  for (int dutyCycle = 0; dutyCycle <= 8191; dutyCycle += 200) {
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    vTaskDelay(10);
  }
  for (int dutyCycle = 8191; dutyCycle >= 0; dutyCycle -= 200) {
    ledcWrite(LEDC_CHANNEL, dutyCycle);
    vTaskDelay(10);
  }
}

void led_digital_blink(int delay_ms, int times) {
  for (int i = 0; i < times; i++) {
    ledcWrite(LEDC_CHANNEL, 8191); 
    vTaskDelay(delay_ms);
    ledcWrite(LEDC_CHANNEL, 0); 
    vTaskDelay(delay_ms);
  }
}

void led_blinky(void *pvParameters){
  ledcSetup(LEDC_CHANNEL, LEDC_FREQ_HZ, 13);
  ledcAttachPin(LED_GPIO, LEDC_CHANNEL);

  while(1) {
    if (xSemaphoreTake(xTempHumiSemaphore, portMAX_DELAY) == pdTRUE) {
      // Dùng biến toàn cục chuẩn "temperature"
      float currentTemp = temperature;

      if (currentTemp > 32.0) {
        led_digital_blink(100, 3); vTaskDelay(100);
        led_digital_blink(300, 3); vTaskDelay(100);
        led_digital_blink(100, 3); vTaskDelay(1000);
      }
      else if (currentTemp >= 28.0 && currentTemp <= 32.0) {
        led_digital_blink(100, 2); vTaskDelay(500);
      }
      else {
        led_breathing_effect();
      }
    }
  }
}