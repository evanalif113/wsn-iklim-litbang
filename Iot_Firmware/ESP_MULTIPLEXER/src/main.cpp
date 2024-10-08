#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>  // Library for BMP280

#define SEALEVELPRESSURE_HPA (1022)

Adafruit_BMP280 bmp; // I2C

void setup() {
  Serial.begin(115200);
  
  // Start I2C communication and initialize BMP280 sensor
  if (!bmp.begin(0x76)) {  // Pastikan alamat I2C sesuai dengan sensor Anda (0x76 atau 0x77)
    Serial.println("Could not find a valid BMP280 sensor, check wiring or I2C address!");
    while (1);
  }
  Serial.println("BMP280 initialized successfully");
}

void loop() { 
  // Print sensor readings
  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");
  
  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.println();
  
  delay(5000); // Delay 5 seconds between readings
}
