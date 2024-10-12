#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// Define relay pin and other settings
const int relayPin = 13; // Pin relay

uint id;
float temp;
float humi;
float pres;
float dew;
float volt;
float windir;
float windspeed;
float rain;

//define the pins used by the transceiver module
#define ss 5
#define rst 16
#define dio0 4

String buf_message;
String message;


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
  //Serial.println(message);
  //Serial.println(packetSize);
  message = "";
}
 
void onTxDone() {
  Serial.println("TxDone");
  LoRa_rxMode();
}

boolean runEvery(unsigned long interval) {
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

void setup() {
 //initialize Serial Monitor
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  //SerialBT.begin("LoRa Receiver"); //Bluetooth device name
  while (!Serial);
  Serial.println("LoRa Gateway");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(922E6)) {
    digitalWrite(2, HIGH);
    Serial.println(".");
    digitalWrite(2, LOW);
    delay(500);
  }
  LoRa.setGain(6);
  LoRa.setSpreadingFactor(11);           // ranges from 6-12,default 7 see API docs
  LoRa.setSignalBandwidth(250E3);
  LoRa.setCodingRate4(5);
  LoRa.enableCrc();
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xE5);
  LoRa.setPreambleLength(8);
  LoRa.onTxDone(onTxDone);
  // register the receive callback
  LoRa.onReceive(onReceive);
  LoRa_rxMode();
  Serial.println("LoRa Sukses");
}

void loop() {
  // Main loop doing nothing unless LoRa message received
  if (buf_message != message) {
    digitalWrite(2, HIGH);
    Serial.print("JSON: ");
    Serial.println(buf_message);
    JsonDocument doc;
    deserializeJson(doc, buf_message);
    id = doc["i"];
    float t= doc["t"];
    float h= doc["h"];
    float p= doc["p"];
    float wd= doc["wd"];
    float ws= doc["ws"];
    float r= doc["r"];
    float v= doc["v"];
    temp = t/100;
    humi = h/100;
    pres = p/100;
    volt = v/100;

    //filter NULL
    if (temp == 0) {
      temp = NAN; // Set temp menjadi NaN (Not a Number)
      }
    if (humi == 0) {
      humi = NAN; // Set temp menjadi NaN (Not a Number)
      }
    if (pres == 0) {
      pres = NAN; // Set temp menjadi NaN (Not a Number)
      }

    double calc = log(humi / 100.0F) + ((17.625F * temp) / (243.04F + temp));
    dew = (243.04F * calc / (17.625F - calc));

    //filter calc
    if (dew == 0){
      dew = NAN;
      }
    
    Serial.print(F("Suhu: ")); Serial.println(temp);
    Serial.print(F("Kelembapan: ")); Serial.println(humi);
    Serial.print(F("Titik Embun: ")); Serial.println(dew);
    Serial.print(F("Tekanan Udara: ")); Serial.println(pres);
    Serial.print(F("Volt: ")); Serial.println(volt);
    buf_message = message;
    // print RSSI of packet
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