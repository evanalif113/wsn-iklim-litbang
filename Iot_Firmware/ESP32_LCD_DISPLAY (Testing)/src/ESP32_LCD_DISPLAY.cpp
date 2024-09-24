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

// LED indicator
int ledPin = 2; // GPIO 2
unsigned long lastTime = 0;
unsigned long timerDelay = 30000; // 30 seconds

// WiFi credentials
const char* ssid = RAHASIA_SSID1;
const char* password = RAHASIA_PASS1;

// SensorData class: handles temperature, humidity, pressure, dew point, and voltage
class SensorData {
  private:
    Adafruit_SHT4x sht4;
    Adafruit_BMP280 bmp;
    Adafruit_MAX17048 maxWin;
    float temperature, humidity, pressure, dewPoint, voltage;

  public:
    SensorData() :  sht4(), bmp(), maxWin(), 
                    temperature(0), humidity(0), pressure(0), dewPoint(0), voltage(0) {}

    void init() {
      // Initialize SHT40
      if (!sht4.begin()) {
        Serial.println("Couldn't find SHT40 sensor! Check wiring.");
        while (1);
      }
      Serial.println("Found SHT40 sensor!");

      // Initialize BMP280
      if (!bmp.begin(0x76)) {
        Serial.println("Couldn't find BMP280 sensor! Check wiring.");
        while (1);
      }
      Serial.println("Found BMP280 sensor!");
      bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);

      // Initialize MAX17048
      if (!maxWin.begin()) {
        Serial.println("Couldn't find MAX17048 sensor! Check wiring.");
        while (1);
      }
      Serial.println("Found MAX17048 sensor!");
    }

    void updateData() {
      sensors_event_t humidityEvent, tempEvent;
      sht4.getEvent(&humidityEvent, &tempEvent);
      temperature = tempEvent.temperature;
      humidity = humidityEvent.relative_humidity;
      pressure = bmp.readPressure() / 100.0F;
      voltage = maxWin.cellVoltage();
      dewPoint = calculateDewPoint(temperature, humidity);
    }

    float getTemperature() { return temperature; }
    float getHumidity() { return humidity; }
    float getPressure() { return pressure; }
    float getDewPoint() { return dewPoint; }
    float getVoltage() { return voltage; }

  private:
    float calculateDewPoint(float temperature, float humidity) {
      const float a = 17.27;
      const float b = 237.7;
      float alpha = ((a * temperature) / (b + temperature)) + log(humidity / 100.0);
      return (b * alpha) / (a - alpha);
    }
};

// LCDDisplay class: handles LCD initialization and data display
class LCDDisplay {
  private:
    LiquidCrystal_I2C lcd;

  public:
    LCDDisplay() : lcd(0x27, 20, 4) {}

    void init() {
      lcd.init();
      lcd.backlight();
    }

    void displayData(SensorData& data) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp: " + String(data.getTemperature(), 1) + (char)223 + "C");
      lcd.setCursor(0, 1);
      lcd.print("Humi: " + String(data.getHumidity(), 1) + "%");
      lcd.setCursor(0, 2);
      lcd.print("Pres: " + String(data.getPressure(), 1) + "hPa");
      lcd.setCursor(0, 3);
      lcd.print("DewP: " + String(data.getDewPoint(), 1) + (char)223 + "C");
    }
};

// ThingspeakClient class: handles data sending to Thingspeak
class ThingspeakClient {
  public:
    void sendData(SensorData& data) {
      WiFiClient client;
      HTTPClient http;
      http.setTimeout(2000);

      String url = "http://api.thingspeak.com/update?api_key=" + String(WRITE_APIKEY);
      url += "&field1=" + String(data.getTemperature());
      url += "&field2=" + String(data.getHumidity());
      url += "&field3=" + String(data.getPressure());
      url += "&field4=" + String(data.getDewPoint());
      url += "&field8=" + String(data.getVoltage());

      http.begin(client, url);
      int httpResponseCode = http.GET();
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
};

// WiFiManager class: handles WiFi connection
class WiFiManager {
  public:
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
};

// Instantiate objects
SensorData sensorData;
LCDDisplay lcdDisplay;
ThingspeakClient thingspeakClient;
WiFiManager wifiManager;

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  sensorData.init();
  lcdDisplay.init();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    digitalWrite(ledPin, HIGH);

    // Connect to WiFi
    wifiManager.connectWiFi();

    // Update sensor data
    sensorData.updateData();

    // Display data on LCD
    lcdDisplay.displayData(sensorData);

    // Send data to Thingspeak
    thingspeakClient.sendData(sensorData);

    // Print data to serial
    Serial.print("Temperature: ");
    Serial.println(sensorData.getTemperature());
    Serial.print("Humidity: ");
    Serial.println(sensorData.getHumidity());
    Serial.print("Dew Point: ");
    Serial.println(sensorData.getDewPoint());
    Serial.print("Pressure: ");
    Serial.println(sensorData.getPressure());

    lastTime = millis();
    digitalWrite(ledPin, LOW);
  }
}
