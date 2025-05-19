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
#define USE_BMP280
//Komen jika tidak menggunakan MS5611
//#define USE_MS5611
//Komen jika tidak menggunakan LCD
//#define USE_LCD
//Komen Jika tidak menggunakan Rainfal Sensor
//#define USE_RAINFALL_SENSOR
//#define USE_MANUAL_WEATHER

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
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

//PENTING ini ID DEVICE
uint id = 4;

// Delay with millis
unsigned long lastTime = 0;
unsigned long timerDelay = 60000; // Atur delay

// Pin dan LED indicator
int ledPin = 2; // GPIO 2

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
  // Inisialisasi SHT31
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
  Serial.println("Founf MS5611 sensor!");
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
    //maxlipo.setActivityThreshold(0.15);
    Serial.print(F("Activity threshold = ")); 
    Serial.print(maxWin.getActivityThreshold()); 
    Serial.println(" V change");
    //maxlipo.setHibernationThreshold(5);
    Serial.print(F("Hibernation threshold = "));
    Serial.print(maxWin.getHibernationThreshold()); 
    Serial.println(" %/hour");
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
    configTime(0, 0, "time.google.com", "pool.ntp.org"); // Initialize NTP Client
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
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <title>Automatic Weather Station V4.5</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Roboto', Arial, sans-serif; background: linear-gradient(to bottom, #e3f2fd, #bbdefb); color: #333; padding: 20px; }
        header { text-align: center; margin-bottom: 30px; }
        header h1 { font-size: 2.5rem; font-weight: bold; color: #0d47a1; margin-bottom: 10px; }
        header h2 { font-size: 1.3rem; color: #1565c0; margin-bottom: 10px; }
        .datetime { text-align: right; font-size: 1.2rem; color: #1e88e5; margin-bottom: 20px; font-weight: bold; }
        .grid-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 0 auto; max-width: 1200px; }
        .card { background: #ffffff; border-radius: 12px; padding: 20px; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.15); text-align: center; transition: transform 0.3s ease, box-shadow 0.3s ease; }
        .card:hover { transform: translateY(-5px); box-shadow: 0 6px 15px rgba(0, 0, 0, 0.2); }
        .card h3 { font-size: 1.3rem; color: #546e7a; margin-bottom: 15px; }
        .card .value { font-size: 2rem; font-weight: bold; color: #0d47a1; margin-bottom: 5px; }
        .card small { font-size: 0.9rem; color: #78909c; }
        footer { text-align: center; margin-top: 30px; font-size: 1rem; color: #546e7a; }
      </style>
      <script>
        function updateTime() {
          const now = new Date();
          const utcTime = now.toUTCString().split(' ')[4]; // Extract HH:MM:SS
          const utcDate = now.toUTCString().split(' ').slice(0, 4).join(' '); // Extract Day, Date, Month, Year
          document.querySelector('.datetime').textContent = `${utcTime} (UTC)`;
          document.querySelector('footer').textContent = `Last Update: ${utcDate} ${utcTime} (UTC)`;
        }
        setInterval(updateTime, 1000); // Update every second
        window.onload = updateTime; // Initialize on page load
      </script>
    </head>
    <body>
      <header>
        <h1>Depertamen Sains Atmosfer Jerukagung Seismologi</h1>
        <h2>Automatic Weather Station - System Online</h2>
      </header>
      <div class="datetime">--:--:-- (UTC)</div>
      <div class="grid-container">
        <div class="card">
          <h3>WiFi SSID</h3>
          <div class="value">)rawliteral";

  html += WiFi.SSID(); // Tambahkan SSID WiFi
  html += R"rawliteral(</div>
          <small>Connected Network</small>
        </div>
        <div class="card">
          <h3>WiFi Signal Strength</h3>
          <div class="value">)rawliteral";

  html += String(WiFi.RSSI()); // Tambahkan RSSI WiFi
  html += R"rawliteral(</div>
          <small>dBm</small>
        </div>
        <div class="card">
          <h3>Working Time</h3>
          <div class="value">)rawliteral";

  #ifdef USE_RAINFALL_SENSOR
    html += String(sensorWorkingTime, 2); // Tambahkan data dinamis
  #else
    html += "0"; // Isi dengan 0 jika tidak menggunakan sensor curah hujan
  #endif
  html += R"rawliteral(</div>
          <small>Minute</small>
        </div>
        <div class="card">
          <h3>Total Rainfall</h3>
          <div class="value">)rawliteral";

  #ifdef USE_RAINFALL_SENSOR
    html += String(rainFall, 2); // Tambahkan data dinamis
  #else
    html += "0"; // Isi dengan 0 jika tidak menggunakan sensor curah hujan
  #endif
  html += R"rawliteral(</div>
          <small>mm</small>
        </div>
        <div class="card">
          <h3>1 Hour Rainfall</h3>
          <div class="value">)rawliteral";

  #ifdef USE_RAINFALL_SENSOR
    html += String(rainRate, 2); // Tambahkan data dinamis
  #else
    html += "0"; // Isi dengan 0 jika tidak menggunakan sensor curah hujan
  #endif
  html += R"rawliteral(</div>
          <small>mm</small>
        </div>
        <div class="card">
          <h3>Raw Data</h3>
          <div class="value">)rawliteral";

  #ifdef USE_RAINFALL_SENSOR
    html += String(rawData); // Tambahkan data dinamis
  #else
    html += "0"; // Isi dengan 0 jika tidak menggunakan sensor curah hujan
  #endif
  html += R"rawliteral(</div>
          <small>times</small>
        </div>
      </div>
      <div class="grid-container">
        <div class="card">
          <h3>Temperature</h3>
          <div class="value">)rawliteral";

  html += String(temperature, 2); // Tambahkan data dinamis
  html += R"rawliteral(</div>
          <small>°C</small>
        </div>
        <div class="card">
          <h3>Humidity</h3>
          <div class="value">)rawliteral";

  html += String(humidity, 2); // Tambahkan data dinamis
  html += R"rawliteral(</div>
          <small>Rh%</small>
        </div>
        <div class="card">
          <h3>Pressure</h3>
          <div class="value">)rawliteral";

  html += String(pressure, 2); // Tambahkan data dinamis
  html += R"rawliteral(</div>
          <small>hPa</small>
        </div>
        <div class="card">
          <h3>Dew Point</h3>
          <div class="value">)rawliteral";

  html += String(dewPoint, 2); // Tambahkan data dinamis
  html += R"rawliteral(</div>
          <small>°C</small>
        </div>
      </div>
      <footer>Last Update: --/--/---- --:--:-- (UTC)</footer>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  #ifdef USE_LCD
  // Inisialisasi LCD
  initDisplay();
  #endif

  // Koneksi WiFi
  initMultiWiFi();
  // Inisialisasi sensor
  Wire.begin();
  initSensors();

  // Konfigurasi endpoint web server
  server.on("/", handleRoot);
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
