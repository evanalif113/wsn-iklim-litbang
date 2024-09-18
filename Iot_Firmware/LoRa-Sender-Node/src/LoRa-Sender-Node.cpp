/*********
  @author By Evan Alif Widhyatma
  @date 2024
  @version 2.0
*********/

#include "esp_sleep.h"
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_SHT4x.h"
#include <Adafruit_BMP280.h>
#include "Adafruit_MAX1704X.h"
#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>
//#include <LiquidCrystal_I2C.h>

#define ID 3

uint64_t uS_TO_S_FACTOR = 1000000;  // Conversion factor for micro seconds to seconds
uint64_t TIME_TO_SLEEP = 60; //sleep time in seconds

//define the pins used by the transceiver module
#define ss 5
#define rst 16
#define dio0 4


const int ledPin = 2 ;
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_BMP280 bmp;
Adafruit_MAX17048 maxlipo;

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  setCpuFrequencyMhz(80);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
    // initialize LCD
  //lcd.init();
  // turn on LCD backlight                      
  //lcd.backlight();
  if (!sht4.begin()) {   // Set to 0x45 for alternate i2c addr
    Serial.println("SHT4x Tidak Ditemukan, cek sambungan");
    while (1) delay(10);
  }
  if (!bmp.begin(0x76)) {
    Serial.println("BMP280 Tidak ditemukan, cek sambungan");
    while (1) delay(10);
  }
  if (!maxlipo.begin()) {
    Serial.println("MAXLIPO Tidak ditemukan, cek sambungan");
    while (1) delay(10);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  //Setting batas bawah dan atas voltase
  maxlipo.setAlertVoltages(2.0, 4.2);
  //sensors.begin();
  Serial.println("Modul Pengirim LoRa");
  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  //Baca Sensor
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);

  float tempi, humi, press, volt;
  tempi = temp.temperature;
  humi = humidity.relative_humidity;
  press = bmp.readPressure()/100.00F;
  volt = maxlipo.cellVoltage();

  //float suhu,lembap, tekanan;
  //suhu   = constrain(tempi, 10.00F, 45.00F);
  //lembap = constrain(humi,0.0F, 100.0F);
  //tekanan= constrain(press, 1000.0F, 1025.0F);

  int t,h,p,v;
   t = tempi*100;
   h = humi*100;
   p = press*100;
   v = volt*100;


  //Force Shutdown
  if (v<=320) {
    uint64_t uS_TO_S_FACTOR = 1000000;  // Conversion factor for micro seconds to seconds
    uint64_t TIME_TO_SLEEP = 600; //sleep time in seconds
    Serial.println("Mode Hibernate Daya Baterai" + String(maxlipo.cellVoltage()));
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); 
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    Serial.println("Mode Hibernate");
    esp_deep_sleep_start(); 
  }
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(922E6)) {
    digitalWrite(ledPin, HIGH);
    Serial.println(".");
    delay(500);
    digitalWrite(ledPin,LOW);
  }
  LoRa.setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSpreadingFactor(11);           // ranges from 6-12,default 7 see API docs
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xE5);
  LoRa.setPreambleLength(8);
  Serial.println("LoRa Sukses");
//void loop() {
  //digitalWrite(2, HIGH);
  //i00t0000h0000p000000v000
  JsonDocument lora_data;
  String sendLoRa;
  lora_data["i"] = ID;
  lora_data["t"] = t;
  lora_data["h"] = h;
  lora_data["p"] = p;
  lora_data["v"] = v;
  serializeJson(lora_data, sendLoRa);
  Serial.println(sendLoRa);
  LoRa.idle();
  LoRa.beginPacket();                   // start packet
  LoRa.print(sendLoRa);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
  sendLoRa="";
  /*lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IKLIM PRIGI");
  lcd.setCursor(0, 1);
  lcd.print("T:"+ String(temp.temperature));lcd.write(0xdf);lcd.write(0x43);
  lcd.setCursor(0, 2);
  lcd.print("H:"+ String(humidity.relative_humidity) +"%");
  lcd.setCursor(10, 1);
  lcd.print("P:"+ String(bmp.readPressure()/100) );
  lcd.setCursor(10, 2);
  lcd.print("V:"+ String(maxlipo.cellVoltage()));*/
  //lcd.setCursor(0, 3);
  //lcd.print("RSI:" + String(LoRa.packetRssi()));
  //lcd.setCursor(8, 3);
  //lcd.print("SNR:" + String(LoRa.packetSnr()));
 
  //LoRa.sleep();
  //LoRa.end();
  //SPI.end();
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); 
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  Serial.println("Mode Tidur");
  digitalWrite(ledPin, LOW);
  esp_deep_sleep_start();  
  Serial.println("Harusnya Kagak Muncul!");
}

void loop() {
  //Kosong mlompong
}