
/*********
  @Create By Evan Aif Widhyatma
*********/

#define BLYNK_TEMPLATE_ID "TMPL6o7H2yQGt"
#define BLYNK_TEMPLATE_NAME "Dashboard Agroklimat Litbang"
//#define BLYNK_AUTH_TOKEN "Xy7ghZtiTBHS5hO3YEwwyNJmzGHA4xF5"

#define BLYNK_FIRMWARE_VERSION "0.3.15"
#define BLYNK_PRINT Serial
#define APP_DEBUG
#define USE_ESP32_DEV_MODULE

//#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "BlynkEdgent.h"
#include <TinyGPS++.h>
#include <SPI.h>
#include <LoRa.h>
//#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include "time.h"
//#include "BluetoothSerial.h"

//char ssid[] = "JerukagungMeteorologi";
//char pass[] = "meteorologi";
#define RXD2 25
#define TXD2 26
HardwareSerial neogps(1);
TinyGPSPlus gps;

//Thingspeak API Key List
String ThingsKey1 = "E4LDA72WDCG5T7QF";  //A
String ThingsKey2 = "FCFMA0BDB3NACBXE";  //B
String ThingsKey3 = "QEOPEK1R1WIPET9W";  //C

String ThingspeaKey;

//Weathercloud API Key List
String wid1 = "e2013519a7853ac0";
String apicloud1 = "eff661ec5f6725c5d95fec8fcf9d92f8";

String wid2 = "d1f1e8f5be529287";
String apicloud2 = "d0ea8f15693f706f3f46ddb294e39b87";

String wid3 = "dfc8a593e3ffd9c8";
String apicloud3 = "bae962d498cf73467dea73289f0415d7";

String deviceID;
String deviceApi;

//Firebase Var
#define API_KEY       "AIzaSyDalcCwwOthPMjC3umkpQECqlQQj699FTY"
#define USER_EMAIL    "seis@gmail.com"
#define USER_PASSWORD "seisca"
#define DATABASE_URL  "https://staklimjerukagung-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char* ntpServer = "time.google.com";

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
//String uid;
// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String TempPath = "/temperature";
String HumiPath = "/humidity";
String PresPath = "/pressure";
String DewPath = "/dew";
String timePath        = "/timestamp";
String espheapramPath  = "/espheapram";

// Parent Node (to be updated in every loop)
String parentPath;
int timestamp;
int espheapram;
FirebaseJson json;

//define the pins used by the transceiver module
#define ss 5
#define rst 16
#define dio0 4

//#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
//#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
//#endif

//BluetoothSerial SerialBT;

uint id;
float temp;
float humi;
float pres;
float dew;
float volt;
float windir;
float windspeed;
float rain;

#include <ArduinoJson.h>
String buf_message;
String message;

WidgetTerminal terminal(V7);

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void Data() {
  if (buf_message != message) {
    digitalWrite(2, HIGH);
    Serial.print("JSON: ");
    Serial.println(buf_message);
    DynamicJsonDocument doc(128);
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
    double calc = log(humi / 100.0F) + ((17.625F * temp) / (243.04F + temp));
    double dew = (243.04F * calc / (17.625F - calc));
    Serial.print("Suhu:"); Serial.println(temp);
    Serial.print("Kelembapan:"); Serial.println(humi);
    Serial.print("Titik Embun:"); Serial.println(dew);
    Serial.print("Tekanan Udara:"); Serial.println(pres);
    Serial.print("Arah Angin:"); Serial.println(windir);
    Serial.print("Volt"); Serial.println(volt);
    buf_message = message;

    int rssi = WiFi.RSSI();
    int ram = ESP.getFreeHeap();
    int rssiLora = LoRa.packetRssi();

    if (id==3) {
      Blynk.virtualWrite(V0, temp);
      Blynk.virtualWrite(V1, humi);
      Blynk.virtualWrite(V2, pres);
      Blynk.virtualWrite(V3, dew);
    }
    Blynk.virtualWrite(V8, volt);
    Blynk.virtualWrite(V9, rssi);
  terminal.clear();
  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println("ID:" + String(id));
  terminal.println("Volt:" + String(volt));
  terminal.println("RSSI WiFi:" + String(rssi));
  terminal.println("RAM:" + String(ram));
  terminal.println("RSSI LORA:" + String(LoRa.packetRssi()));
  terminal.println("SNR:" + String(LoRa.packetSnr()));
  terminal.flush();

  /*while(neogps.available()){
    if(gps.encode(neogps.read())){
      Serial.print("SATS: ");
      Serial.println(gps.satellites.value());
      Serial.print("LATITUDE: ");
      Serial.println(gps.location.lat(), 8);
      Serial.print("LONGITUDE: ");
      Serial.println(gps.location.lng(), 8);
      Serial.println("---------------------------");
    }
  }*/
    WiFiClient client;
    HTTPClient http;
      http.setTimeout(2000);
      if (id==1) {
        ThingspeaKey=ThingsKey1;
      }
      if (id==2) {
        ThingspeaKey=ThingsKey2;
      }
      if (id==3) {
        ThingspeaKey=ThingsKey3;
      }

      String url1 = "http://api.thingspeak.com/update?api_key=" + ThingspeaKey;
      url1 += "&field1=" + String(temp);
      url1 += "&field2=" + String(humi);
      url1 += "&field3=" + String(pres);
      url1 += "&field4=" + String(dew);
      url1 += "&field8=" + String(volt);
      // Send HTTP POST request
      http.begin(client, url1);
      int httpResponseCode1 = http.GET();
      if (httpResponseCode1 > 0) {
        Serial.print("Thingspeak Response code: ");
        Serial.println(httpResponseCode1);
        String payload = http.getString();
        Serial.println(payload);
      } 
      else {
        Serial.print("Error code Thingspeak: ");
        Serial.println(httpResponseCode1);
      }

      // Free resources
      Serial.println("RSSI:" + String(LoRa.packetRssi()));
      Serial.print("SNR:" + String(LoRa.packetSnr()));
      Serial.println();
      http.end();
    // print RSSI of packet
    //SerialBT.print("RSSI :");
    //SerialBT.println(LoRa.packetRssi());
    //SerialBT.print("SNR :");
    //SerialBT.println(LoRa.packetSnr());
    //SerialBT.print("Freq Error :");
    //SerialBT.println(LoRa.packetFrequencyError());
    //SerialBT.println();    
    Weathercloud();
    //FirebaseData();
    digitalWrite(2, LOW);
  }
}

void Weathercloud() {
      if (id==1) {
        deviceID = wid1;
        deviceApi = apicloud1;
      }
      if (id==2) {
        deviceID = wid2;
        deviceApi = apicloud2;
      }
      if (id==3) {
        deviceID = wid3;
        deviceApi = apicloud3;
      }
      WiFiClient client;
      HTTPClient http;
      http.setTimeout(2000);
      String url2 = "http://api.weathercloud.net/set/wid/" + deviceID + "/key/" + deviceApi;
      url2 += "/temp/" + String(temp * 10);
      url2 += "/hum/" + String(humi);
      url2 += "/bar/" + String(pres * 10);
      url2 += "/dew/" + String(dew * 10);
      url2 += "/heat/" + String(dew * 10);
      //url2 += "/chill/" + String(dew * 10);
      url2 += "/wspd/" + String(0);
      url2 += "/wspdhi/" + String(0);
      url2 += "/wdir/" + String(0);
      url2 += "/rain/" + String(0 * 10);
      url2 += "/rainrate/" + String(0 * 10);
      //url2 += "/solarrad/" + String(0 * 10);
      //url2 += "/et/" + String(0 * 10);
      //url2 += "/uvi/" + String(0 * 10);

      http.begin(client, url2);
      int httpResponseCode2 = http.GET();
      if (httpResponseCode2 > 0) {
        Serial.print("Weathercloud Response code: ");
        Serial.println(httpResponseCode2);
        String payload = http.getString();
        Serial.println(payload);
      } 
      else {
        Serial.print("Error code Weathercloud: ");
        Serial.println(httpResponseCode2);
      }
      terminal.println("HTTP Weathercloud"+String(httpResponseCode2));
      terminal.flush();
      http.end();
}

/*void FirebaseSetup() {
  configTime(0, 0, ntpServer); //NTP Server
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  //uid = auth.token.uid.c_str();
  //Serial.print("User UID: "); Serial.println(uid);

  // Update database path
  databasePath = "/auto_weather_stat/id-03/data";
}*/

void FirebaseData() {
    timestamp = getTime();
    espheapram = ESP.getFreeHeap();
    Serial.println (timestamp);
    Serial.println (espheapram);
    
    parentPath = databasePath + "/" + String(timestamp);
    json.set(TempPath.c_str(), String(temp));
    json.set(HumiPath.c_str(), String(humi));
    json.set(PresPath.c_str(), String(pres));
    json.set(DewPath.c_str(),  String(dew));
    

    json.set(espheapramPath.c_str(),  String(espheapram));
    json.set(timePath.c_str(),  String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
   
    Serial.println(ESP.getFreeHeap());
}

/*void connectionstatus() {
  if ((WiFi.status() != WL_CONNECTED) ) {
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
    }
    Serial.println();
    Serial.println(WiFi.localIP());
    //Alternatively, you can restart your board
    //ESP.restart();
  } else {
    Serial.println("WiFi OK!");
  }
}*/

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

void onCadDone(boolean signalDetected) {
  // detect preamble
  if (signalDetected) {
    Serial.println("Signal detected");
    // put the radio into continuous receive mode
    LoRa.receive();
  } else {
    // try next activity dectection
  }
}

/*void GpsData() {
  while (neogps.available()){
    if (gps.encode(neogps.read())){
      Serial.print("SATS: ");
      Serial.println(gps.satellites.value());
      Serial.print("LATITUDE: ");
      Serial.println(gps.location.lat(), 8);
      Serial.print("LONGITUDE: ");
      Serial.println(gps.location.lng(), 8);
      Serial.println("---------------------------");
    }
  }
}*/

void setup() {
  //initialize Serial Monitor
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //FirebaseSetup();
  configTime(0, 0, ntpServer); //NTP Server
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;
  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  //uid = auth.token.uid.c_str();
  //Serial.print("User UID: "); Serial.println(uid);

  // Update database path
  databasePath = "/auto_weather_stat/id-03/data";
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
  Serial.println("LoRa Sukses");
  LoRa.onTxDone(onTxDone);
  // register the receive callback
  LoRa.onReceive(onReceive);
  LoRa_rxMode();
  BlynkEdgent.begin();
}
unsigned int checkStatusPeriode = 120000;
unsigned int checkStatusNext;
unsigned int checkGpsPeriode = 5000;
unsigned int checkGpsNext;
unsigned int checkWeatherPeriode = 150000;
unsigned int checkWeatherNext;
unsigned long FirebasePeriode = 60000;
unsigned long FirebaseNext;

void loop() {
  BlynkEdgent.run();
  /*if (checkStatusNext<=millis() && WiFi.status() !=WL_CONNECTED) {
  connectionstatus();
  checkStatusNext = millis() + checkStatusPeriode;
  }
  if (checkGpsNext <= millis()) {
  GpsData();
  checkGpsNext = millis() + checkGpsPeriode;
  }
  if (checkWeatherNext <= millis()) {
  Weathercloud();
  checkWeatherNext = millis() + checkWeatherPeriode;
  }*/
  /*if (Firebase.ready() && (FirebaseNext <= millis() || FirebaseNext == 0)) {
  FirebaseData();
  FirebaseNext = millis() + FirebasePeriode;
  }*/
  Data();
}