/*********
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/tca9548a-i2c-multiplexer-esp32-esp8266-arduino/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>  // Library for BMP280

#define SEALEVELPRESSURE_HPA (1022)

Adafruit_BMP280 bmp1; // I2C
Adafruit_BMP280 bmp2; // I2C

// Select I2C BUS
void TCA9548A(uint8_t bus){
  Wire.beginTransmission(0x70);  // TCA9548A address (default 0x70)
  Wire.write(1 << bus);          // Send byte to select bus
  Wire.endTransmission();
  delay(100);                    // Short delay for stability
}

void printValues(Adafruit_BMP280 bmp, int bus) {
  TCA9548A(bus);                  // Switch to the selected bus
  Serial.print("Sensor number on bus ");
  Serial.println(bus);
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
}

void setup() {
  Serial.begin(115200);
  
  // Start I2C communication with the Multiplexer
  Wire.begin();

  // Init sensor on bus number 2 (first BMP280)
  TCA9548A(2);
  if (!bmp1.begin(0x76)) {            // Ensure this address matches your sensor's I2C address
    Serial.println("Could not find a valid BMP280 sensor on bus 2, check wiring or address!");
    while (1);
  }
  Serial.println();
  
  // Init sensor on bus number 3 (second BMP280)
  TCA9548A(3);
  if (!bmp2.begin(0x76)) {            // Ensure this address matches your sensor's I2C address
    Serial.println("Could not find a valid BMP280 sensor on bus 3, check wiring or address!");
    while (1);
  }
  Serial.println();
}

void loop() { 
  // Print values for both sensors
  printValues(bmp1, 2); // Sensor 1 on bus 2
  delay(1000);          // Small delay to stabilize sensor reading
  printValues(bmp2, 3); // Sensor 2 on bus 3
  delay(5000);          // Wait for a while before next reading
}
