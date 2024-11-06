/*********
  @author By Evan Aif Widhyatma
  @date 2024
  @version 3.1
*********/
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include "rahasia.h"


//ConfigTime Constanta
const char* gmtOffset_sec = "25200"; //GMT +7
const char* daylightOffset_sec = "0";
const char* ntpServer = "pool.ntp.org";  //NTP server

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
String DewPath  = "/dew";

String VoltPath        = "/volt";
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

void Windy() {
const char* windy_ca= \
  "-----BEGIN CERTIFICATE-----\n" \
"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\n"\
"-----END CERTIFICATE-----\n" ;

      WiFiClientSecure client;
      client.setCACert(windy_ca);
      HTTPClient https;

      http.setTimeout(2000);
      String serverPath = "https://stations.windy.com/pws/update/";
      serverPath += "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJjaSI6MTM2NjMyNiwiaWF0IjoxNjY4OTYxMTYwfQ._axcUYfBSkHB_L1_NB5Vrru2aDUfFwj-ua7ewqXrPpA?";
      serverPath += "temp=" + String(temp); //suhu (Celcius)
      serverPath += "&humidity=" + String(humi); //kelembapan (Persen)
      serverPath += "&mbar=" + String(pres); //tekanan (hPa)
      serverPath += "&dewpoint=" + String(dew); //titik embun (Celcius)
      serverPath += "&wind=" + String(5); //kecepatan angin (m/s)
      serverPath += "&winddir=" + String(0); //arah angin (derajat)
      serverPath += "&rain=" + String(0); //curah hujan (mm) tiap jam
      serverPath += "&station=0";
      serverPath += "&name=Jerukagung_Meteorologi";
      https.addHeader("Content-Type", "text/plain");
      https.begin(client, serverPath.c_str());

      // Send HTTP GET request
      int ResponseCode = https.GET();
      
      if (ResponseCode>0) {
        Serial.print("Windy Response code: ");
        Serial.println(ResponseCode);
        String payload = https.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Windy Error code: ");
        Serial.println(ResponseCode);
      }
      // Free resources
      //Serial.println(serverPath);
      https.end();
}

void Thingspeak() {
      WiFiClient client;
      HTTPClient http;

      http.setTimeout(2000);
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
        Serial.print(F("Thingspeak Response code: "));
        Serial.println(httpResponseCode1);
        String payload = http.getString();
        Serial.println(payload);
      } 
      else {
        Serial.print(F("Error code Thingspeak: "));
        Serial.println(httpResponseCode1);
      }
      http.end();
}
void FirebaseSetup() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //NTP Server
  // Assign the api key (required)
  config.api_key = FIREBASE_AUTH;
  // Assign the user sign in credentials
  auth.user.email = FIREBASE_USER;
  auth.user.password = FIREBASE_PASS;
  // Assign the RTDB URL (required)
  config.database_url = FIREBASE_HOST;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

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
  String uid = auth.token.uid.c_str();
  Serial.print("User UID: "); Serial.println(uid);

}

void FirebaseData() {
    // Update database path
    databasePath = "/auto_weather_stat/id-0"+String(id)+"/data";
    timestamp = getTime();
    espheapram = ESP.getFreeHeap();
    Serial.println (timestamp);
    Serial.println (espheapram);
    
    parentPath = databasePath + "/" + String(timestamp);
    json.set(TempPath.c_str(), String(temp));
    json.set(HumiPath.c_str(), String(humi));
    json.set(PresPath.c_str(), String(pres));
    json.set(DewPath.c_str(),  String(dew));
    json.set(VoltPath.c_str(), String(volt));
    
    
    //json.set(espheapramPath.c_str(),  String(espheapram));
    json.set(timePath.c_str(),  String(timestamp));

    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
   
    Serial.println(ESP.getFreeHeap());
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
  wifiMulti.addAP("server", "risetiklim");
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
//unsigned int checkGpsPeriode = 5000;
//unsigned int checkGpsNext;
unsigned int WeathercloudPeriode = 150000;
unsigned int WeathercloudNext;
unsigned int WindyPeriode = 600000;
unsigned int WindyNext;
unsigned long FirebasePeriode = 60000;
unsigned long FirebaseNext;

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
    
    Serial.print(F("Suhu:")); Serial.println(temp);
    Serial.print(F("Kelembapan:")); Serial.println(humi);
    Serial.print(F("Titik Embun:")); Serial.println(dew);
    Serial.print(F("Tekanan Udara:")); Serial.println(pres);
    Serial.print(F("Arah Angin:")); Serial.println(windir);
    Serial.print(F("Volt")); Serial.println(volt);
    buf_message = message;

    int rssi = WiFi.RSSI();
    int ram = ESP.getFreeHeap();
    int rssiLora = LoRa.packetRssi();

    // print RSSI of packet
    //SerialBT.print("RSSI :");
    //SerialBT.println(LoRa.packetRssi());
    //SerialBT.print("SNR :");
    //SerialBT.println(LoRa.packetSnr());
    //SerialBT.print("Freq Error :");
    //SerialBT.println(LoRa.packetFrequencyError());
    //SerialBT.println();  
    
    // Free resources
    Serial.println("RSSI LoRa:" + String(LoRa.packetRssi()));
    Serial.println("SNR:" + String(LoRa.packetSnr()));
    Serial.println(); 
    Thingspeak();

    if (Firebase.ready()) {
    FirebaseData();
    }
    Windy();
    digitalWrite(2, LOW);
  }
}

void loop() {
  /*if (checkStatusNext<=millis() && WiFi.status() !=WL_CONNECTED) {
  connectionstatus();
  checkStatusNext = millis() + checkStatusPeriode;
  }*/
  if (checkStatusNext<=millis() && WiFi.status() !=WL_CONNECTED) {
  connectionstatusMulti();
  checkStatusNext = millis() + checkStatusPeriode;
  }
  /**
  if (Firebase.ready() && (FirebaseNext <= millis() || FirebaseNext == 0)) {
  FirebaseData();
  FirebaseNext = millis() + FirebasePeriode;
  }*/
  Data();
}