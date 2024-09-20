#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP280.h>
#include <math.h>
#include "rahasia.h"

// Wi-Fi credentials
const char* ssid = RAHASIA_SSID;   
const char* password = RAHASIA_PASS;   

unsigned long myChannelNumber = RAHASIA_CH_ID;  
const char * myWriteAPIKey = RAHASIA_WRITE_APIKEY;  

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;  // 30 seconds interval

// LED indicator
int ledPin = 2; // GPIO 2

// Create sensor objects
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_BMP280 bmp; // I2C (GPIO 21 = SDA, GPIO 22 = SCL)

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

  // Initialize the SHT40 sensor
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT40 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found SHT40 sensor!");

  // Initialize the BMP280 sensor
  if (!bmp.begin(0x76)) {
    Serial.println("Couldn't find BMP280 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found BMP280 sensor!");

  // Set BMP280 parameters
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,   // temperature
                  Adafruit_BMP280::SAMPLING_X16,  // pressure
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  
  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for Wi-Fi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the LED pin
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Only run once every 30 seconds
  if ((millis() - lastTime) > timerDelay) {
    digitalWrite(ledPin, HIGH);

    // Check Wi-Fi connection
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi not connected. Reconnecting...");
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, password);
        delay(1000);
      }
      Serial.println("WiFi reconnected.");
    }

    // Get data from SHT40 sensor (temperature and humidity)
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    float temperatureSHT40 = temp.temperature;  // Temperature in ºC
    float humiditySHT40 = humidity.relative_humidity;  // Humidity in %

    // Print the data to Serial monitor
    Serial.print("Temperature (SHT40, ºC): ");
    Serial.println(temperatureSHT40);
    Serial.print("Humidity (SHT40, %): ");
    Serial.println(humiditySHT40);

    // Calculate dew point
    float dewPoint = calculateDewPoint(temperatureSHT40, humiditySHT40);
    Serial.print("Dew Point (ºC): ");
    Serial.println(dewPoint);

    // Get data from BMP280 sensor (air pressure)
    float pressureBMP280 = bmp.readPressure() / 100.0F;  // Pressure in hPa

    // Print the pressure to Serial monitor
    Serial.print("Pressure (BMP280, hPa): ");
    Serial.println(pressureBMP280);

    // Send data to ThingSpeak using HTTPClient
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String url = "http://api.thingspeak.com/update?api_key=" + String(myWriteAPIKey) +
                   "&field1=" + String(temperatureSHT40) +
                   "&field2=" + String(humiditySHT40) +
                   "&field3=" + String(pressureBMP280) +
                   "&field4=" + String(dewPoint);

      http.begin(url); // Specify the URL
      int httpResponseCode = http.GET(); // Make the request

      // Check the returning code
      if (httpResponseCode > 0) {
        String response = http.getString();  // Get the response to the request
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        Serial.println("ThingSpeak Response: " + response);
      } else {
        Serial.println("Error on HTTP request: " + String(httpResponseCode));
      }

      // Free resources
      http.end();
    } else {
      Serial.println("WiFi Disconnected");
    }

    // Update the last time the data was sent
    lastTime = millis();
    digitalWrite(ledPin, LOW);
  }
}
