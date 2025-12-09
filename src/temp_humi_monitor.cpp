// #include "temp_humi_monitor.h"
// #include "printLCD.h"
// // #define DHTPIN 2
// // #define DHTTYPE DHT11

// DHT20 dht20;
// // DHT dht(DHTPIN, DHTTYPE);

// SemaphoreHandle_t xTempHumidSemaphore = xSemaphoreCreateBinary();


// #define SCL_Pin  9
// #define SDA_Pin  8

// // NOTE: CODE WHEN USING DHT20 LIBRARIES AND RELATING PERIPHERALS
// void temp_humi_monitor(void *pvParameters){

//     Wire.begin(SDA_Pin, SCL_Pin);
//     dht20.begin();

//     while (1){
//         /* code */
        
//         dht20.read();
//         TempHumid th = {dht20.getTemperature(), dht20.getHumidity()};
//         xQueueOverwrite(TempHumidQueue, &th);

//         // Check if any reads failed and exit early
//         if (isnan(th.temperature) || isnan(th.humidity)) {
//             Serial.println("Failed to read from DHT sensor! Retrying");
//             delay(1000);
//             continue;
//         }

//         //Update global variables for temperature and humidity

//         // Print the results
//         xSemaphoreGive(xTempHumidSemaphore);

//         vTaskDelay(2000);
//     }
    
// }

// // void temp_humi_monitor(void *pvParameters) {
// //     dht.begin();
// //     while (1) {
// //         vTaskDelay(pdMS_TO_TICKS(100));
// //         Serial.println("Reading...");
// //         float temperature = dht.readTemperature();
// //         float humidity = dht.readHumidity();

// //         if (isnan(temperature) || isnan(humidity)) {
// //             Serial.println("Failed to read from DHT11 sensor!");
// //         } else {
// //             Serial.print("Humidity: ");
// //             Serial.print(humidity);
// //             Serial.print("%  Temperature: ");
// //             Serial.print(temperature);
// //             Serial.println("°C");
// //         }

// //         vTaskDelay(pdMS_TO_TICKS(3000)); 
// //     }
// // }
