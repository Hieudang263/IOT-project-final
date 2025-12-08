#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SHT31.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>

Adafruit_SHT31 sht30 = Adafruit_SHT31();

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ----------- WIFI -----------
#define WIFI_SSID     "100 Tran Van Du"
#define WIFI_PASSWORD "68686868"

// ---------- EMAIL ----------
SMTPSession smtp;
#define EMAIL_SENDER     "hau.phan0363758506@hcmut.edu.vn"
#define EMAIL_PASSWORD   "tebcgqgaqmphlami"
#define EMAIL_RECEIVER   "phanlehau404@gmail.com"

// Time variables
int secondNow = 0, minuteNow = 0, hourNow = 0;
int dayNow = 1, monthNow = 1, yearNow = 2025;
const int daysInMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

unsigned long previousMillis = 0;
const unsigned long interval = 1000;

bool alertSent = false;
float tempToSend = 0;

// Blink
unsigned long lastBlink = 0;
bool blinkState = true;


// ================= EMAIL TASK: RUNNING ON CORE 0 =================
void sendMailTask(void *param) {

  float temp = tempToSend; // take value from global variable

  Serial.println("TASK: preparing to send email...");

  smtp.debug(1);
  smtp.setTCPTimeout(5000);

  Session_Config config;
  config.server.host_name = "smtp.gmail.com";
  config.server.port = 587;
  config.secure.startTLS = true;
  config.login.email = EMAIL_SENDER;
  config.login.password = EMAIL_PASSWORD;

  SMTP_Message message;
  message.sender.name = "ESP32 Monitoring System";
  message.sender.email = EMAIL_SENDER;

  message.subject = "⚠ [ESP32] Abnormal Temperature Alert";

  // HTML
  String htmlMsg;
  htmlMsg += "<h2>Warning from ESP32</h2>";
  htmlMsg += "<ul>";
  htmlMsg += "<li><b>Time:</b> ";
  htmlMsg += String(hourNow) + ":" + String(minuteNow) + ":" + String(secondNow);
  htmlMsg += " - " + String(dayNow) + "/" + String(monthNow) + "/" + String(yearNow) + "</li>";
  htmlMsg += "<li><b>Current Temperature:</b> " + String(temp, 1) + " °C</li>";
  if (temp > 35)
    htmlMsg += "<li><b>Status:</b> 🔥 <b>Temperature Too High!</b></li>";
  else
    htmlMsg += "<li><b>Status:</b> ❄ <b>Temperature Too Low!</b></li>";
  htmlMsg += "</ul>";

  message.html.content = htmlMsg.c_str();
  message.html.charSet = "utf-8";
  message.addRecipient("Owner", EMAIL_RECEIVER);

  Serial.println("TASK: connecting SMTP...");
  if (!smtp.connect(&config)) {
    Serial.println("SMTP Connect failed!");
  } 
  else {
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Send mail failed!");
    else
      Serial.println("Mail sent successfully!");
  }

  vTaskDelete(NULL); // delete task after completion
}


// =========================== SETUP ===========================
void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(200);

  Wire.begin(12, 13);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setRotation(2);

  sht30.begin(0x44);

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  // xTaskCreatePinnedToCore(serialTask, "serialTask", 4096, NULL, 1, NULL, 0);
}


// =========================== LOOP (CORE 1) ===========================
void loop() {

  // TIME RECEIVE FROM SERIAL
  if (Serial.available() >= 6) {
    hourNow   = Serial.read();
    minuteNow = Serial.read();
    secondNow = Serial.read();
    dayNow    = Serial.read();
    monthNow  = Serial.read();
    yearNow   = Serial.read() + 2000;
  }

  // UPDATE TIME
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    secondNow++;
    if (secondNow >= 60) { secondNow = 0; minuteNow++; }
    if (minuteNow >= 60) { minuteNow = 0; hourNow++; }
    if (hourNow >= 24) { hourNow = 0; dayNow++; }

    if (dayNow > daysInMonth[monthNow - 1]) {
      dayNow = 1;
      monthNow++;
      if (monthNow > 12) { monthNow = 1; yearNow++; }
    }
  }

  // READ SENSOR
  float temp = sht30.readTemperature();
  float hum  = sht30.readHumidity();

  // ALERT CHECK
  if (!isnan(temp)) {
    if ((temp > 35 || temp < 25) && !alertSent) {

      tempToSend = temp;  // save for separate task

      // CREATE TASK ON CORE 0
      xTaskCreatePinnedToCore(
        sendMailTask,
        "sendMailTask",
        8192,
        NULL,
        1,
        NULL,
        0          // <--- CORE 0
      );

      alertSent = true;
    }

    if (temp <= 35 && temp >= 25)
      alertSent = false;
  }

  // BLINK
  if (millis() - lastBlink >= 500) {
    lastBlink = millis();
    blinkState = !blinkState;
  }

  // OLED DISPLAY
  display.clearDisplay();
  display.setCursor(0, 0);

  if (!isnan(temp)) {
    if (temp > 35 || temp < 25) {
      if (blinkState)
        display.printf("%.1f C\n", temp);
      else
        display.println("      ");
    } else {
      display.printf("%.1f C\n", temp);
    }

    display.printf("%.1f %%\n", hum);
  }
  else {
    display.println("Sensor Error!");
  }

  display.printf("%02d:%02d:%02d\n", hourNow, minuteNow, secondNow);
  display.printf("%02d/%02d/%04d\n", dayNow, monthNow, yearNow);

  display.display();
  delay(50);
}
