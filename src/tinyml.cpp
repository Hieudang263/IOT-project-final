#include <DHT.h>
#include <TensorFlowLite_ESP32.h>
#include "model.h"
#include "global.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// ----- TinyML -----
tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;
tflite::AllOpsResolver resolver;

const tflite::Model* tflite_model = nullptr;
tflite::MicroInterpreter* interpreter;

constexpr int kTensorArenaSize = 40 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

// ----- Buffer 3 mẫu -----
float ring_temp[3];
float ring_hum[3];
int ring_index = 0;
bool buffer_full = false;

// Thêm extern để truy cập queue
extern QueueHandle_t PredictQueue;
extern QueueHandle_t TempHumidQueue;  // ✅ THÊM DÒNG NÀY

void tinyml_setup() {
  Serial.begin(115200);
  dht.begin();
  tflite_model = tflite::GetModel(g_model);
  interpreter = new tflite::MicroInterpreter(tflite_model, resolver, tensor_arena,
                                             kTensorArenaSize, error_reporter);
  interpreter->AllocateTensors();
}

void tinyml_loop() {
  // ---- 1) Đọc DHT ----
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // ✅ Đẩy vào TempHumidQueue để các task khác sử dụng
  TempHumid th;
  if (!isnan(t) && !isnan(h)) {
      th.temperature = t;
      th.humidity = h;
      if (TempHumidQueue != NULL) {
          xQueueOverwrite(TempHumidQueue, &th);
      }
  } else {
      // Nếu lỗi sensor, gửi giá trị 0
      th.temperature = 0;
      th.humidity = 0;
      if (TempHumidQueue != NULL) {
          xQueueOverwrite(TempHumidQueue, &th);
      }
  }

  // Nếu lỗi cảm biến → gửi giá trị 0
  if (isnan(t) || isnan(h)) {
    Serial.println("{\"current_temp\": 0, \"current_humi\": 0, \"predicted_temp\": null, \"predicted_humi\": null, \"accuracy\": null}");
    
    PredictData predictData;
    predictData.has_data = false;
    predictData.predicted_temp = 0;
    predictData.predicted_humi = 0;
    predictData.accuracy = 0;
    
    if (PredictQueue != NULL) {
        xQueueOverwrite(PredictQueue, &predictData);
    }
    
    delay(1000);
    return;
  }

  // ---- 2) Lưu vào buffer vòng ----
  ring_temp[ring_index] = t;
  ring_hum[ring_index]  = h;

  ring_index = (ring_index + 1);
  if (ring_index >= 3) {
    ring_index = 0;
    buffer_full = true;
  }

  float pred_t = 0;
  float pred_h = 0;
  float acc = 0;

  // ---- 3) Nếu có đủ 3 mẫu → đưa vào mô hình ----
  if (buffer_full) {
    TfLiteTensor* input = interpreter->input(0);

    // 3 mẫu → 6 giá trị input
    int k = ring_index;   // vị trí mẫu mới nhất
    for (int i = 0; i < 3; i++) {
      int idx = (k + i) % 3;  
      input->data.f[i * 2 + 0] = ring_temp[idx];
      input->data.f[i * 2 + 1] = ring_hum[idx];
    }

    // ---- RUN ----
    interpreter->Invoke();

    TfLiteTensor* output = interpreter->output(0);
    pred_t = output->data.f[0];
    pred_h = output->data.f[1];

    // ---- 4) Tính accuracy ----
    float dt = fabs(pred_t - t);
    float dh = fabs(pred_h - h);
    acc = 100.0 - (dt * 2 + dh * 1.5);
    if (acc < 0) acc = 0;
    if (acc > 100) acc = 100;
  }

  // ---- 5) Xuất UART và đẩy vào queue ----
  Serial.print("{\"current_temp\": ");
  Serial.print(t, 2);
  Serial.print(", \"current_humi\": ");
  Serial.print(h, 2);

  PredictData predictData;
  predictData.has_data = false;

  if (!buffer_full) {
      Serial.println(", \"predicted_temp\": null, \"predicted_humi\": null, \"accuracy\": null}");
  } else {
      Serial.print(", \"predicted_temp\": ");
      Serial.print(pred_t, 2);
      Serial.print(", \"predicted_humi\": ");
      Serial.print(pred_h, 2);
      Serial.print(", \"accuracy\": ");
      Serial.print((int)acc);
      Serial.println("}");
      
      // Đẩy vào queue để webserver lấy
      predictData.predicted_temp = pred_t;
      predictData.predicted_humi = pred_h;
      predictData.accuracy = (int)acc;
      predictData.has_data = true;
  }

  // Gửi vào queue (overwrite nếu đầy)
  if (PredictQueue != NULL) {
      xQueueOverwrite(PredictQueue, &predictData);
  }

  delay(1000);
}

// ==================== FREERTOS TASK WRAPPER ====================
void tiny_ml_task(void *param) {
    tinyml_setup();
    
    for (;;) {
        tinyml_loop();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
