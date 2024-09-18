#include <WiFi.h>
#include "ThingSpeak.h"
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP280.h>
#include <math.h>
#include "rahasia.h"

// Wi-Fi credentials
const char* ssid = RAHASIA_SSID;   
const char* password = RAHASIA_PASS;   

WiFiClient client;


unsigned long myChannelNumber = RAHASIA_CH_ID;  
const char * myWriteAPIKey = RAHASIA_WRITE_APIKEY;  

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;  // 30 seconds interval

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
  
  // Initialize Wi-Fi and ThingSpeak
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
}

void loop() {
  // Only run once every 30 seconds
  if ((millis() - lastTime) > timerDelay) {

    // Connect or reconnect to WiFi
    if(WiFi.status() != WL_CONNECTED){
      Serial.print("Attempting to connect");
      while(WiFi.status() != WL_CONNECTED){
        WiFi.begin(ssid, password);
        delay(5000);
        Serial.print(".");
      } 
      Serial.println("\nConnected.");
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

    // Send data to ThingSpeak
    int x = ThingSpeak.writeField(myChannelNumber, 1, temperatureSHT40, myWriteAPIKey);  // Field 1: Temperature from SHT40
    int y = ThingSpeak.writeField(myChannelNumber, 2, humiditySHT40, myWriteAPIKey);     // Field 2: Humidity from SHT40
    int z = ThingSpeak.writeField(myChannelNumber, 3, pressureBMP280, myWriteAPIKey);    // Field 3: Air Pressure from BMP280
    int w = ThingSpeak.writeField(myChannelNumber, 4, dewPoint, myWriteAPIKey);          // Field 4: Dew Point

    if (x == 200 && y == 200 && z == 200 && w == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code: " + String(x) + " " + String(y) + " " + String(z) + " " + String(w));
    }

    // Update the last time the data was sent
    lastTime = millis();
  }
}
