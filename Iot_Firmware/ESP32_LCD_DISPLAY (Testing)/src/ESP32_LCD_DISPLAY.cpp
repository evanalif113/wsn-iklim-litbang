#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MAX1704X.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <ArduinoJson.h>
#include "time.h"
#include "rahasia.h"

// LED indicator
int ledPin = 2; // GPIO 2

float tempe;
float humid;
float press;
float dewPoint;


// Create sensor objects
Adafruit_SHT4x sht4 = Adafruit_SHT4x(); // I2C (GPIO 21 = SDA, GPIO 22 = SCL)
Adafruit_BMP280 bmp; // I2C (GPIO 21 = SDA, GPIO 22 = SCL)
Adafruit_MAX17048 maxWin; // I2C (GPIO 21 = SDA, GPIO 22 = SCL)
// Initialize the LCD with 20 columns and 4 rows
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C (GPIO 21 = SDA, GPIO 22 = SCL)


//SHT40 Setup
void InitSHT40() {
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT40 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found SHT40 sensor!");
}

void SHT40getData() {
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);
  float tempe = temp.temperature;  // Temperature in ºC
  float humid = humidity.relative_humidity;  // Humidity in %
}

// BMP280 Setup
void InitBMP280() {
  if (!bmp.begin(0x76)) {
    Serial.println("Couldn't find BMP280 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found BMP280 sensor!");
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // temperature
                  Adafruit_BMP280::SAMPLING_X16,  // pressure
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
}

void BMP280getData() {
  float press = bmp.readPressure()/100.0F;  // Pressure in hPa
}

// MAX17048 Setup
void InitMAX17048() {
  if (!maxWin.begin()) {
    Serial.println("Couldn't find MAX17048 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found MAX17048 sensor!");
}

void MAX17048getData() {
  float volt = maxWin.cellVoltage();
}

// LCD Setup
void InitLCD() {
  lcd.init();
  lcd.backlight();
}
// Function to calculate dew point
float calculateDewPoint(float temperature, float humidity) {
  const float a = 17.27;
  const float b = 237.7;
  float alpha = ((a * temperature) / (b + temperature)) + log(humidity / 100.0);
  float dewPoint = (b * alpha) / (a - alpha);
  return dewPoint;
}

void setup() {
  Serial.begin(115200);
  InitSHT40(); // Initialize the SHT40 sensor
  InitBMP280(); // Initialize the BMP280 sensor
  InitMAX17048(); // Initialize the MAX17048 sensor
  InitLCD(); // Initialize the LCD
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Only run once every 30 seconds
  if ((millis() - lastTime) > timerDelay) {
    digitalWrite(ledPin, HIGH);
    
    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(RAHASIA_SSID1, RAHASIA_PASS1);
        delay(5000);
        Serial.print(".");
      } 
      Serial.println("\nConnected.");
    }

    // Calculate dew point
    float dewPoint = calculateDewPoint(tempe, humid);

    // Get data from BMP280 sensor (air pressure)
    float pressure = bmp.readPressure() / 100.0F;  // Pressure in hPa
    float volt = maxWin.cellVoltage();

    // Print the data to Serial monitor
    Serial.print("Temperature (SHT40, ºC): ");
    Serial.println(tempe);
    Serial.print("Humidity (SHT40, %): ");
    Serial.println(humid);
    Serial.print("Dew Point (ºC): ");
    Serial.println(dewPoint);
    Serial.print("Pressure (BMP280, hPa): ");
    Serial.println(pressure);

    // Display sensor data on 20x4 LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: " + String(tempe, 1) + (char)223 + "C");
    lcd.setCursor(0, 1);
    lcd.print("Humi: " + String(humid, 1) + "%");
    lcd.setCursor(0, 2);
    lcd.print("Pres: " + String(pressure, 1) + "hPa");
    lcd.setCursor(0, 3);
    lcd.print("DewP: " + String(dewPoint, 1) + (char)223 + "C");

    // Send data to ThingSpeak using HTTP
    if (WiFi.status() == WL_CONNECTED) {
      String serverPath = server;
      serverPath += "?api_key=" + String(myWriteAPIKey);
      serverPath += "&field1=" + String(tempe);
      serverPath += "&field2=" + String(humid);
      serverPath += "&field3=" + String(pressure);
      serverPath += "&field4=" + String(dewPoint);
      serverPath += "&field8=" + String(volt);

      http.begin(client, serverPath.c_str());
      int httpResponseCode = http.GET();
      
      if (httpResponseCode > 0) {
        Serial.println("Data sent to ThingSpeak successfully.");
      } else {
        Serial.println("Error sending data to ThingSpeak.");
      }
      
      http.end();
    }

    // Update the last time the data was sent
    lastTime = millis();
    digitalWrite(ledPin, LOW);
  }
}
