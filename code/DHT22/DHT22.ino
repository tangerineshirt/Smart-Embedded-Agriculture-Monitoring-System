#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 25      // Pin yang terhubung ke DHT22
#define DHTTYPE DHT22  // Jenis sensor DHT yang digunakan
#define LDR_PIN 39     // Pin ADC untuk LDR
#define SOIL_PIN 36    // Pin yang terhubung dengan Soil Moisture Sensor


const char *ssid = "OPPO A77s";
const char *password = "12345678";

#define BOTtoken "6712436152:AAGhBCGRhHTJ914gi-Z9HkAxiq7dhEUv_y8"
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
const String chat_id = "-4239672717";  //group chat id

SemaphoreHandle_t xSemaphore;
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Ganti alamat I2C sesuai dengan LCD Anda

void TaskDHT22(void *pvParameters);
void TaskLDR(void *pvParameters);
void TaskSoil(void *pvParameters);
void TaskLCD(void *pvParameters);
void TaskTelegram(void *pvParameters);

float humidity, temperature;
int ldrValue, soilValue;
int currentTask = 0;
String soilState, ldrState, humidState;

unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  lcd.init();
  lcd.backlight();
  dht.begin();
  if (xSemaphore == NULL) {
    xSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive((xSemaphore));
    xTaskCreate(TaskDHT22, "DHT22", 4096, NULL, 1, NULL);
    xTaskCreate(TaskLDR, "LDR", 2048, NULL, 2, NULL);
    xTaskCreate(TaskSoil, "Soil", 2048, NULL, 3, NULL);
    xTaskCreate(TaskLCD, "LCD", 2048, NULL, 4, NULL);
    xTaskCreate(TaskTelegram, "Telegram", 4096, NULL, 5, NULL);
  }
  client.setInsecure();
}

void loop() {
  // Tidak perlu ada kode di sini
}

void TaskDHT22(void *pvParameters) {
  for (;;) {
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    if (xSemaphoreTake(xSemaphore, (TickType_t)5) == pdTRUE) {
      if (humidity >= 0 && humidity <= 25.0) {
        humidState = "Dry";
      } else if (humidity <= 80.0) {
        humidState = "Moderate";
      } else if (humidity <= 100.0) {
        humidState = "Moist";
      } else {
        humidState = "Failed to read from sensor";
      }
      xSemaphoreGive(xSemaphore);
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}


void TaskLDR(void *pvParameters) {
  for (;;) {
    ldrValue = analogRead(LDR_PIN);
    if (((ldrValue * -1) + 4095) <= 1300) {
      ldrState = "Dark";
    } else if (((ldrValue * -1) + 4095) <= 2700) {
      ldrState = "Moderate";
    } else if (((ldrValue * -1) + 4095) <= 4095) {
      ldrState = "Bright";
    } else {
      ldrState = "Failed to read from sensor";
    }
    vTaskDelay(1);
  }
}

void TaskSoil(void *pvParameters) {
  for (;;) {
    soilValue = analogRead(SOIL_PIN);
    if (((soilValue * -1) + 3000) <= 700) {
      soilState = "Dry";
    } else if (((soilValue * -1) + 3000) <= 1400) {
      soilState = "Moderate";
    } else if (((soilValue * -1) + 3000) <= 2100) {
      soilState = "Wet";
    } else {
      soilState = "Failed to read from sensor";
    }

    vTaskDelay(1);
  }
}

void TaskLCD(void *pvParameters) {
  for (;;) {
    switch (currentTask) {
      case 0:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Humidity: ");
        lcd.print(humidity);
        lcd.print("%");
        lcd.setCursor(0, 1);
        lcd.print("Temp: ");
        lcd.print(temperature);
        lcd.print("C");
        break;
      case 1:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("LDR Value:");
        lcd.setCursor(0, 1);
        lcd.print((ldrValue * -1) + 4095);
        break;
      case 2:
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Soil Moisture:");
        lcd.setCursor(0, 1);
        lcd.print((soilValue * -1) + 3000);
        break;
    }
    currentTask = (currentTask + 1) % 3;
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Ganti tampilan setiap 2 detik
  }
}

void TaskTelegram(void *pvParameters) {
  for (;;) {
    if (millis() - lastTime > timerDelay) {
      String message = "Temperature: " + String(temperature) + " °C\n";
      message += "Humidity: " + humidState + " \n";
      message += "Soil Moisture: " + soilState + "\n";
      message += "Light Intensity: " + ldrState;
      bot.sendMessage(chat_id, message, "");
    }
    vTaskDelay(1);
  }
}
