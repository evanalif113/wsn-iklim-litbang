#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <ArduinoJson.h>

// ThingSpeak settings
WiFiClient client;                       // WiFi client untuk ThingSpeak
unsigned long myChannelNumber = 2289583;  // Channel ID
const char * myWriteAPIKey = "FCFMA0BDB3NACBXE"; // API Key

uint id;
float temp;
float humi;
float pres;
float dew;
float volt;
float windir;
float windspeed;
float rain;

// Define the pins used by the transceiver module
#define ss 5
#define rst 16
#define dio0 4

String buf_message;
String message;

// Wi-Fi credentials
const char* ssid = "Jerukagung Seismologi";    // SSID WiFi
const char* password = "riset1234";      // Password WiFi

void LoRa_rxMode() {
  LoRa.disableInvertIQ();               // normal mode
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode() {
  LoRa.idle();                          // set standby mode
  LoRa.enableInvertIQ();                // active invert I and Q signals
}

void LoRa_sendMessage(String message) {
  LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.print(message);                  // add payload
  LoRa.endPacket(true);                 // finish packet and send it
}

void onReceive(int packetSize) {
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  buf_message = message;
  message = "";
}

void onTxDone() {
  Serial.println("TxDone");
  LoRa_rxMode();
}

void sendDataToThingSpeak(float temperature, float humidity, float pressure, float voltage, float dew_point) {
  ThingSpeak.setField(1, temperature);      // Field 1 untuk Suhu
  ThingSpeak.setField(2, humidity);         // Field 2 untuk Kelembapan
  ThingSpeak.setField(3, pressure);         // Field 3 untuk Tekanan Udara
  ThingSpeak.setField(4, dew_point);        // Field 4 untuk Titik Embun
  ThingSpeak.setField(5, LoRa.packetRssi()); // Field 5 untuk RSSI
  ThingSpeak.setField(6, LoRa.packetSnr());  // Field 6 untuk SNR
  ThingSpeak.setField(7, LoRa.packetFrequencyError()); // Field 7 untuk Frequency Error
  ThingSpeak.setField(8, voltage);          // Field 8 untuk Voltase

  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  
  if (x == 200) {
    Serial.println("Data berhasil dikirim ke ThingSpeak.");
  } else {
    Serial.println("Gagal mengirim data ke ThingSpeak. Kode error: " + String(x));
  }
}

void setup() {
  // initialize Serial Monitor
  pinMode(2, OUTPUT);
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");

  // Initialize ThingSpeak
  ThingSpeak.begin(client);

  // setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  while (!LoRa.begin(922E6)) { // frequency 922 MHz
    digitalWrite(2, HIGH);
    Serial.print(".");
    digitalWrite(2, LOW);
    delay(500);
  }

  LoRa.setGain(6);
  LoRa.setSpreadingFactor(11); // ranges from 6-12
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  LoRa.setSyncWord(0xE5);
  LoRa.setPreambleLength(8);
  LoRa.onTxDone(onTxDone);
  LoRa.onReceive(onReceive);
  LoRa_rxMode();
  Serial.println("LoRa Inisialisasi Sukses");
}

void loop() {
  // Main loop processing LoRa message
  if (buf_message != message) {
    digitalWrite(2, HIGH);
    Serial.print("JSON: ");
    Serial.println(buf_message);

    // Deserialize the JSON message
    JsonDocument doc;
    deserializeJson(doc, buf_message);

    id = doc["i"];
    float t = doc["t"];
    float h = doc["h"];
    float p = doc["p"];
    float v = doc["v"];

    temp = t / 100;
    humi = h / 100;
    pres = p / 100;
    volt = v / 100;

    // Dew point calculation
    double calc = log(humi / 100.0F) + ((17.625F * temp) / (243.04F + temp));
    dew = (243.04F * calc / (17.625F - calc));
    
    if (!isnan(temp) && temp != 0 &&
        !isnan(humi) && humi != 0 &&
        !isnan(pres) && pres != 0 &&
        !isnan(dew) && dew != 0 &&
        !isnan(volt) && volt != 0) {

    Serial.print(F("Suhu: ")); Serial.println(temp);
    Serial.print(F("Kelembapan: ")); Serial.println(humi);
    Serial.print(F("Titik Embun: ")); Serial.println(dew);
    Serial.print(F("Tekanan Udara: ")); Serial.println(pres);
    Serial.print(F("Volt: ")); Serial.println(volt);
    
    // Send data to ThingSpeak
    sendDataToThingSpeak(temp, humi, pres, volt, dew);
    
    buf_message = message;

    // Print RSSI, SNR, and frequency error
    Serial.print("RSSI :");
    Serial.println(LoRa.packetRssi());
    Serial.print("SNR :");
    Serial.println(LoRa.packetSnr());
    Serial.print("Freq Error :");
    Serial.println(LoRa.packetFrequencyError());
    Serial.println(); 

    digitalWrite(2, LOW);
    }
  }
}
