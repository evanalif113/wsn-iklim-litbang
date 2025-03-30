/*********
  @author By Evan Aif Widhyatma
  @date 2024
  @version 4.5
*********/
//Komen Jika tidak menggunakan SHT31
#define USE_SHT31
//Komen jika tidak menggunakan SHT40
//#define USE_SHT40
//Komen jika tidak menggunakan BMP280
//#define USE_BMP280
//Komen jika tidak menggunakan MS5611
#define USE_MS5611
//Komen jika tidak menggunakan LCD
//#define USE_LCD
//Komen Jika tidak menggunakan Rainfal Sensor
//#define USE_RAINFALL_SENSOR

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <Adafruit_MAX1704X.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include "time.h"
#include "rahasia.h"

#ifdef USE_SHT31
#include "Adafruit_SHT31.h"
#endif

#ifdef USE_SHT40
#include <Adafruit_SHT4x.h>
#endif

#ifdef USE_BMP280
#include <Adafruit_BMP280.h>
#endif

#ifdef USE_MS5611
#include "MS5611.h"
#endif

#ifdef USE_LCD
#include <LiquidCrystal_I2C.h>
#endif

#ifdef USE_RAINFALL_SENSOR
#include "DFRobot_RainfallSensor.h"
#endif

#define ARDUINOJSON_SLOT_ID_SIZE 1
#define ARDUINOJSON_STRING_LENGTH_SIZE 1


//PENTING ini ID DEVICE
uint id = 4;

// Delay with millis
unsigned long lastTime = 0;
unsigned long timerDelay = 15000; // Atur delay

// Pin dan LED indicator
int ledPin = 2; // GPIO 2

const char* ntpServer = "time.google.com";
const char* ntpServer2 = "pool.ntp.org";

void asyncCB(AsyncResult &aResult);
void printResult(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;

using AsyncClient = AsyncClientClass;

WiFiClientSecure ssl_client;
AsyncClient aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;

// Sensor SHT40, BMP280, MAX17048
#ifdef USE_SHT31
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

#ifdef USE_SHT40
Adafruit_SHT4x sht4;
#endif

#ifdef USE_BMP280
Adafruit_BMP280 bmp;
#endif

#ifdef USE_MS5611
MS5611 ms5611(0x77);
#endif

Adafruit_MAX17048 maxWin;
#ifdef USE_RAINFALL_SENSOR
DFRobot_RainfallSensor_I2C Sensor(&Wire);
#endif

WebServer server(80);

#ifdef USE_LCD
// LCD Display
LiquidCrystal_I2C lcd(0x27, 20, 4);
#endif 

// Data sensor
float temperature = 0, 
      humidity = 0, 
      pressure = 0, 
      dewPoint = 0, 
      windSpeed = 0,
      windDirection = 0,
      voltage = 0;
#ifdef USE_RAINFALL_SENSOR
float rainFall = 0, //Total in One Day
      rainRate = 0, //Total in One Hour
      sensorWorkingTime = 0;
// Variabel tiping bucket
int rawData = 0;
#endif

// Fungsi untuk mengambil waktu epoch saat ini
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

// Fungsi untuk menghitung titik embun (dew point)
float calculateDewPoint(float temperature, float humidity) {
  const float a = 17.27;
  const float b = 237.7;
  float alpha = ((a * temperature) / (b + temperature)) + log(humidity / 100.0);
  return (b * alpha) / (a - alpha);
}

// Fungsi untuk inisialisasi sensor
void initSensors() {
  #ifdef USE_SHT31
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31 sensor! Check wiring.");
    while (1);
  }
  sht31.heater(false);
  Serial.println("Found SHT31 sensor!");
  #endif
  // Inisialisasi SHT40
  #ifdef USE_SHT40
  if (!sht4.begin()) {
    Serial.println("Couldn't find SHT40 sensor! Check wiring.");
    while (1);
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  Serial.println("Found SHT40 sensor!");
  #endif
  // Inisialisasi BMP280
  #ifdef USE_BMP280
  if (!bmp.begin(0x76)) {
    Serial.println("Couldn't find BMP280 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found BMP280 sensor!");
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, 
                  Adafruit_BMP280::SAMPLING_X2, 
                  Adafruit_BMP280::SAMPLING_X16, 
                  Adafruit_BMP280::FILTER_X16, 
                  Adafruit_BMP280::STANDBY_MS_500);
  #endif
  // Inisialisasi MS5611
  #ifdef USE_MS5611
  if (!ms5611.begin()) {
    Serial.println("Couldn't find MS5611 sensor! Check wiring.");
    while (1);
  }
  #endif
  // Inisialisasi MAX17048
  if (!maxWin.begin()) {
    Serial.println("Couldn't find MAX17048 sensor! Check wiring.");
    while (1);
  }
  Serial.println("Found MAX17048 sensor!");
#ifdef USE_RAINFALL_SENSOR
  // Inisialisasi sensor curah hujan
  while (!Sensor.begin()) {
    Serial.println("Sensor init err!!!");
    delay(1000);
    }
  // Set nilai awal curah hujan (unit: mm)
  Sensor.setRainAccumulatedValue(0.2794);
#endif
}


#ifdef USE_LCD
void initDisplay() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}
#endif

WiFiMulti wifiMulti;

void initMultiWiFi() {
  // Add list of wifi networks
  wifiMulti.addAP(SSID_1, PASS1);
  wifiMulti.addAP(SSID_2, PASS2);
  wifiMulti.addAP(SSID_3, PASS3);
  wifiMulti.addAP(SSID_4, PASS4);
  wifiMulti.addAP(SSID_5, PASS5);
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

// Fungsi untuk update data sensor
void updateSensorData() {
  #ifdef USE_SHT31
  temperature = sht31.readTemperature();
  humidity = sht31.readHumidity();
  #endif

  #ifdef USE_SHT40
  sensors_event_t humidityEvent, tempEvent;
  sht4.getEvent(&humidityEvent, &tempEvent);
  temperature = tempEvent.temperature;
  humidity = humidityEvent.relative_humidity;
  #endif

  #ifdef USE_BMP280
  pressure = bmp.readPressure() / 100.0F;
  #endif

  #ifdef USE_MS5611
  ms5611.setOversampling(OSR_ULTRA_HIGH);
  ms5611.read();
  pressure = ms5611.getPressure();
  #endif

  voltage = maxWin.cellVoltage();
  dewPoint = calculateDewPoint(temperature, humidity);
#ifdef USE_RAINFALL_SENSOR
  // Update data sensor curah hujan
  sensorWorkingTime = Sensor.getSensorWorkingTime() * 60;
  rainFall = Sensor.getRainfall(24);            // Total curah hujan 24 jam (mm)
  rainRate = Sensor.getRainfall(1);           // Curah hujan selama 1 jam (mm)
  rawData = Sensor.getRawData();  
#endif
}

#ifdef USE_LCD
// Fungsi untuk menampilkan data di LCD
void displayDataOnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: " + String(temperature, 1) + (char)223 + "C");
  lcd.setCursor(0, 1);
  lcd.print("Humi: " + String(humidity, 1) + "%");
  lcd.setCursor(0, 2);
  lcd.print("Pres: " + String(pressure, 1) + "hPa");
  lcd.setCursor(0, 3);
  lcd.print("DewP: " + String(dewPoint, 1) + (char)223 + "C");
}
#endif

void FirebaseSetup() {
    configTime(0, 0, ntpServer, ntpServer2); // Initialize NTP Client
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);

    ssl_client.setInsecure();

    initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");   
}

void FirebaseData() {
  // Update NTP time
  unsigned long timestamp;
  timestamp = getTime();// Get current epoch time

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
  JsonDocument docW;

  docW["dew"] = dewPoint;
  docW["humidity"] = humidity;
  docW["pressure"] = pressure;
  docW["temperature"] = temperature;
  docW["timestamp"] = timestamp;
  docW["volt"] = voltage;
#ifdef USE_RAINFALL_SENSOR
  docW["rainfall"] = rainFall;
  docW["rainrate"] = rainRate;
#endif

  String dataCuaca;

  docW.shrinkToFit();  // optional
  serializeJson(docW, dataCuaca);

  // Dynamically use timestamp in the path
  String dbPath = "/auto_weather_stat/id-0"+String(id)+"/data/" + timestamp;
  Database.set<object_t>(aClient, dbPath.c_str(), object_t(dataCuaca), asyncCB, "setTask");
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

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Index file not found");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleData() {
  JsonDocument doc;
#ifdef USE_RAINFALL_SENSOR
  doc["workingTime"] = sensorWorkingTime;
  doc["totalRainfall"] = rainFall;
  doc["HourRainfall"] = rainRate;
  doc["rawData"] = rawData;
#endif
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pressure"] = pressure;
  doc["dewPoint"] = dewPoint;
  doc["voltage"] = voltage;

  String jsonStr;
  serializeJson(doc, jsonStr);
  server.send(200, "application/json", jsonStr);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  #ifdef USE_LCD
  // Inisialisasi LCD
  initDisplay();
  #endif

  // Inisialisasi LittleFS
  if (!LittleFS.begin(true)) {  // Format otomatis jika mount gagal
    Serial.println("LittleFS mount failed");
    while (1);
  }
  Serial.println("LittleFS mounted successfully");

  // Koneksi WiFi
  initMultiWiFi();
  // Inisialisasi sensor
  Wire.begin();
  initSensors();

    // Konfigurasi endpoint web server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  ElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server started");

  // Inisialisasi Firebase
  FirebaseSetup();
  digitalWrite(ledPin, LOW);
}
unsigned int checkStatusPeriode = 120000;
unsigned int checkStatusNext;

void loop() {
  app.loop();
  Database.loop();
  server.handleClient();
  ElegantOTA.loop();
  if (checkStatusNext<=millis() && WiFi.status() !=WL_CONNECTED) {
  connectionstatusMulti();
  checkStatusNext = millis() + checkStatusPeriode;
  }

  unsigned long millisSekarang = millis();
  if (millisSekarang - lastTime >= timerDelay ) {
    digitalWrite(ledPin, HIGH);

    // Update data sensor
    updateSensorData();

    #ifdef USE_LCD
    // Tampilkan data di LCD
    displayDataOnLCD();
    #endif

    // Kirim data ke Firebase
    FirebaseData();

    // Cetak data ke serial
    Serial.print("Temperature: ");
    Serial.println(temperature);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    Serial.print("Dew Point: ");
    Serial.println(dewPoint);
    Serial.print("Pressure: ");
    Serial.println(pressure);
    Serial.print("Voltage: ");
    Serial.println(voltage);

    lastTime = millisSekarang;
    digitalWrite(ledPin, LOW);
    
  }
}
