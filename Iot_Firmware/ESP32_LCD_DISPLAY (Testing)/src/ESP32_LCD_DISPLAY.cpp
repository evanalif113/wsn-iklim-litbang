#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MAX1704X.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"
#include "rahasia.h"

// Pin dan LED indicator
int ledPin = 2; // GPIO 2
unsigned long lastTime = 0;
unsigned long timerDelay = 30000; // 30 seconds

// Sensor SHT40, BMP280, MAX17048
Adafruit_SHT4x sht4;
Adafruit_BMP280 bmp;
Adafruit_MAX17048 maxWin;

// LCD Display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// WiFi credentials
const char* ssid = RAHASIA_SSID1;
const char* password = RAHASIA_PASS1;

// Data sensor
float temperature = 0, humidity = 0, pressure = 0, dewPoint = 0, voltage = 0;

// Fungsi untuk menghitung titik embun (dew point)
float calculateDewPoint(float temperature, float humidity) {
  const float a = 17.27;
  const float b = 237.7;
  float alpha = ((a * temperature) / (b + temperature)) + log(humidity / 100.0);
  return (b * alpha) / (a - alpha);
}

// Fungsi untuk inisialisasi sensor
void initSensors() {
  // Inisialisasi SHT40
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT40 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found SHT40 sensor!");

  // Inisialisasi BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("Couldn't find BMP280 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found BMP280 sensor!");
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, 
                  Adafruit_BMP280::SAMPLING_X2, 
                  Adafruit_BMP280::SAMPLING_X16, 
                  Adafruit_BMP280::FILTER_X16, 
                  Adafruit_BMP280::STANDBY_MS_500);

  // Inisialisasi MAX17048
  if (!maxWin.begin()) {
    Serial.println("Couldn't find MAX17048 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found MAX17048 sensor!");
}

// Fungsi untuk update data sensor
void updateSensorData() {
  sensors_event_t humidityEvent, tempEvent;
  sht4.getEvent(&humidityEvent, &tempEvent);
  temperature = tempEvent.temperature;
  humidity = humidityEvent.relative_humidity;
  pressure = bmp.readPressure() / 100.0F;
  voltage = maxWin.cellVoltage();
  dewPoint = calculateDewPoint(temperature, humidity);
}

// Fungsi untuk menampilkan data di LCD
void displayDataOnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(temperature, 1) + (char)223 + "C");
  lcd.setCursor(0, 1);
  lcd.print("Humi: " + String(humidity, 1) + "%");
  lcd.setCursor(0, 2);
  lcd.print("Pres: " + String(pressure, 1) + "hPa");
  lcd.setCursor(0, 3);
  lcd.print("DewP: " + String(dewPoint, 1) + (char)223 + "C");
}

// Fungsi untuk mengirim data ke ThingSpeak
void sendDataToThingspeak() {
  WiFiClient client;
  HTTPClient http;
  http.setTimeout(2000);

  String url = "http://api.thingspeak.com/update?api_key=" + String(WRITE_APIKEY);
  url += "&field1=" + String(temperature);
  url += "&field2=" + String(humidity);
  url += "&field3=" + String(pressure);
  url += "&field4=" + String(dewPoint);
  url += "&field8=" + String(voltage);

  http.begin(client, url);
  uint16_t httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print(F("Thingspeak Response code: "));
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print(F("Error code Thingspeak: "));
    Serial.println(httpResponseCode);
  }
  http.end();
}

// Fungsi untuk koneksi WiFi
void connectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect");
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      delay(5000);
      Serial.print(".");
    }
    Serial.println("\nConnected.");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();

  // Koneksi WiFi
  connectWiFi();

  // Inisialisasi sensor
  initSensors();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    digitalWrite(ledPin, HIGH);

    // Update data sensor
    updateSensorData();

    // Tampilkan data di LCD
    displayDataOnLCD();

    // Kirim data ke ThingSpeak
    sendDataToThingspeak();

    // Cetak data ke serial
    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Dew Point: ");
    Serial.println(dewPoint);
    Serial.print("Pressure: ");
    Serial.println(pressure);
    Serial.print("Voltage: ");
    Serial.println(voltage);

    lastTime = millis();
    digitalWrite(ledPin, LOW);
  }
}
