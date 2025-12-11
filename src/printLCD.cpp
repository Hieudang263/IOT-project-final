// #include "printLCD.h"

// LiquidCrystal_I2C lcd(0x21,16,2);

// bool button_flag = 0;
// bool lastState = 0;
// bool currentState;
// TaskHandle_t waterTaskHandle = NULL;
// TaskHandle_t tempHumidTaskHandle = NULL;
// SemaphoreHandle_t PrintOnLCDSemaphore = xSemaphoreCreateMutex();

// void reportTempAndHumidity(void* pvParameters){
//     while(1){
//         if(xSemaphoreTake(xTempHumidSemaphore, portMAX_DELAY)) 
//             break;
//     }
//     TempHumid receiver;

//     while(1){
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         TickType_t startTime = xTaskGetTickCount();

//         if(xSemaphoreTake(PrintOnLCDSemaphore, portMAX_DELAY)){
//             while(xTaskGetTickCount() - startTime < 5000){
//                 xQueuePeek(TempHumidQueue, &receiver, 100);
//                 currentState = digitalRead(SENSOR_PIN);
//                 if(currentState != lastState){
//                     button_flag = !button_flag;
//                     lastState = currentState;
//                     vTaskDelay(10);
//                 }
            
//                 if(button_flag){
//                     lcd.clear();
//                     lcd.home();
//                     lcd.print("Temp: ");
//                     lcd.print(receiver.temperature, 1);
//                     lcd.print(char(223));
//                     lcd.print("C");
//                     lcd.setCursor(0,1);
//                     lcd.print("Humidity: ");
//                     lcd.print(receiver.humidity, 1);
//                     lcd.print("%");
//                 }
//                 else{
//                     lcd.clear();
//                     lcd.home();
//                     if(receiver.temperature > 30){
//                         lcd.setCursor(0,0);
//                         lcd.print("Hot Temperature");
//                     }
//                     else if(receiver.temperature > 25){
//                         lcd.setCursor(0,0);
//                         lcd.print("Mid Temperature");
//                     }
//                     else{
//                         lcd.setCursor(0,0);
//                         lcd.print("Low Temperature");
//                     }
//                     if(receiver.humidity > 90){
//                         lcd.setCursor(0,1);
//                         lcd.print("                ");
//                         lcd.setCursor(0,1);
//                         lcd.print("High Humidity");
//                     }
//                     else if(receiver.humidity > 70){
//                         lcd.setCursor(0,1);
//                         lcd.print("               ");
//                         lcd.setCursor(0,1);
//                         lcd.print("Mid Humidity");
//                     }
//                     else if(receiver.humidity > 50){
//                         lcd.setCursor(0,1);
//                         lcd.print("               ");
//                         lcd.setCursor(0,1);
//                         lcd.print("Average Humidity");
//                     }
//                     else{
//                         lcd.setCursor(0,1);
//                         lcd.print("               ");
//                         lcd.setCursor(0,1);
//                         lcd.print("Low Humidity");
//                     }
//                 }
//                 vTaskDelay(500);
//             }
//         }
//         xSemaphoreGive(PrintOnLCDSemaphore);
//     }
// }

// void reportWaterAmount(void* pvParameters){
//     int value;

//     while(1){

//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

//         TickType_t startTime = xTaskGetTickCount();
        
//         if(xSemaphoreTake(PrintOnLCDSemaphore, portMAX_DELAY)){
//             while(xTaskGetTickCount() - startTime < 5000){
//                 if(xSemaphoreTake(xWaterSemaphore, portMAX_DELAY)){
//                     xQueuePeek(waterValueQueue, &value, 100);
//                     currentState = digitalRead(SENSOR_PIN);
//                     if(currentState != lastState){
//                         button_flag = !button_flag;
//                         lastState = currentState;
//                         vTaskDelay(10);
//                     }
//                         if(button_flag){
//                         lcd.clear();
//                         lcd.home();
//                         lcd.print("ADC value: ");
//                         lcd.print(value);
//                     }
//                     else{
//                         lcd.clear();
//                         lcd.home();
//                         if(value > 2000){
//                             lcd.setCursor(0,0);
//                             lcd.print("Sensor Wet");
//                         }
//                         else if(value > 1000){
//                             lcd.setCursor(0,0);
//                             lcd.print("Sensor half wet");
//                         }
//                         else{
//                             lcd.setCursor(0,0);
//                             lcd.print("Sensor dry");
//                         }
//                     }
//                 }
//                 vTaskDelay(500);
//             }
//         }
//         xSemaphoreGive(PrintOnLCDSemaphore);
//     }
// }

// void switchLCD(void* pvParameters){
//     while(1){
//         xTaskNotifyGive(tempHumidTaskHandle);
//         vTaskDelay(5000);

//         xTaskNotifyGive(waterTaskHandle);
//         vTaskDelay(5000);
//     }
// }