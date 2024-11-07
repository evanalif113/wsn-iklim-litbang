/*********
  @author By Evan Aif Widhyatma
  @date 2024
  @version 4.1
*********/

#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FirebaseClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include "UserConfig.h"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "AIzaSyCLnLUN0jSUj7X37VTVJciUHsIyl4sT0-0"

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "esp32@mail.com"
#define USER_PASSWORD "kirim1234"
#define DATABASE_URL "https://database-sensor-iklim-litbang-default-rtdb.asia-southeast1.firebasedatabase.app/"

void asyncCB(AsyncResult &aResult);
void printResult(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;

using AsyncClient = AsyncClientClass;

WiFiClientSecure ssl_client;
AsyncClient aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);  // UTC+7

//define the pins used by the transceiver module
#define ss 5
#define rst 16
#define dio0 4

uint id;
float temp;
float humi;
float pres;
float dew;
float volt;
float windir;
float windspeed;
float rain;

String buf_message;
String message;

void FirebaseSetup() {
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl_client.setInsecure();

    initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
    timeClient.begin();  // Initialize NTP Client
}

void FirebaseData() {
  timeClient.update();  // Update NTP time
  unsigned long timestamp = timeClient.getEpochTime(); // Get current epoch time

  //JSON Constructor by FirebaseClient
  /*JsonWriter writer;
  object_t json, t, h, p, d, v, times;

  writer.create(t, "temp", temp);
  writer.create(h, "humi", humi);
  writer.create(p, "pres", pres);
  writer.create(d, "dew", dew);
  writer.create(v, "volt", volt);
  writer.create(times, "timestamp",timestamp);

  writer.join(json, 6, t, h, p, d, v, times);*/

  //JSON Constructor by ArduinoJSON
  JsonDocument doc1;

  doc1["dew"] = dew;
  doc1["humidity"] = humi;
  doc1["pressure"] = pres;
  doc1["temperature"] = temp;
  doc1["timestamp"] = timestamp;
  doc1["volt"] = volt;

  String datCu;

  doc1.shrinkToFit();  // optional
  serializeJson(doc1, datCu);

  // Dynamically use timestamp in the path
  String dbPath = "/auto_weather_stat/id-0"+String(id)+"/data/" + timestamp;
  Database.set<object_t>(aClient, dbPath.c_str(), object_t(datCu), asyncCB, "setTask");
}

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

WiFiMulti wifiMulti;

void initMultiWiFi() {
  // Add list of wifi networks
  wifiMulti.addAP("Jerukagung Seismologi", "riset1234");
  wifiMulti.addAP("server", "jeris6467");
  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi.....");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

// Maintain WiFi connection
void connectionstatusMulti() {
  if ((wifiMulti.run(10000) != WL_CONNECTED) ) {
    Serial.println("Reconnecting to WiFi...");
    Serial.print("WiFi connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
    while (wifiMulti.run() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
    }
    Serial.println();
    Serial.print(WiFi.SSID());
    Serial.print(" ");
    Serial.println(WiFi.RSSI());
    Serial.println(WiFi.localIP());
    //Alternatively, you can restart your board
    //ESP.restart();
    } else {
    Serial.println("WiFi Failed!");
  }
}

void asyncCB(AsyncResult &aResult) {
    printResult(aResult);
}

void printResult(AsyncResult &aResult){
    if (aResult.isEvent()) {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
    }

    if (aResult.isDebug()) {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError()) {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available()) {
        RealtimeDatabaseResult &RTDB = aResult.to<RealtimeDatabaseResult>();
        if (RTDB.isStream()) {
            Serial.println("----------------------------");
            Firebase.printf("task: %s\n", aResult.uid().c_str());
            Firebase.printf("event: %s\n", RTDB.event().c_str());
            Firebase.printf("path: %s\n", RTDB.dataPath().c_str());
            Firebase.printf("data: %s\n", RTDB.to<const char *>());
            Firebase.printf("type: %d\n", RTDB.type());
        }
        else {
            Serial.println("----------------------------");
            Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
        }
        Firebase.printf("Free Heap: %d\n", ESP.getFreeHeap());
    }
}

void setup() {
  //initialize Serial Monitor
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  //neogps.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //initWiFi();
  initMultiWiFi();
  FirebaseSetup();
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

unsigned int checkStatusPeriode = 120000;
unsigned int checkStatusNext;
unsigned int WindyPeriode = 600000;
unsigned int WindyNext;

void Data() {
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

  double calc = log(humi / 100.0F) + ((17.625F * temp) / (243.04F + temp));
  dew = (243.04F * calc / (17.625F - calc));

  if (!isnan(temp) && temp != 0 &&
      !isnan(humi) && humi != 0 &&
      !isnan(pres) && pres != 0 &&
      !isnan(dew)  && dew != 0 &&
      !isnan(volt) && volt != 0) {
    Serial.print(F("Suhu:")); Serial.println(temp);
    Serial.print(F("Kelembapan:")); Serial.println(humi);
    Serial.print(F("Titik Embun:")); Serial.println(dew);
    Serial.print(F("Tekanan Udara:")); Serial.println(pres);
    Serial.print(F("Arah Angin:")); Serial.println(windir);
    Serial.print(F("Volt")); Serial.println(volt);
    buf_message = message;

    // Free resources
    Serial.println("RSSI LoRa:" + String(LoRa.packetRssi()));
    Serial.println("SNR:" + String(LoRa.packetSnr()));
    Serial.println(); 
    FirebaseData();
    digitalWrite(2, LOW);
    }
  }
}

void loop() {
  app.loop();
  Database.loop();
  /*if (checkStatusNext<=millis() && WiFi.status() !=WL_CONNECTED) {
  connectionstatus();
  checkStatusNext = millis() + checkStatusPeriode;
  }*/
  if (checkStatusNext<=millis() && WiFi.status() !=WL_CONNECTED) {
  connectionstatusMulti();
  checkStatusNext = millis() + checkStatusPeriode;
  }
  Data();
}