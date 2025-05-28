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
#define USE_MANUAL_WEATHER

//#define DEBUG

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <HTTPClient.h>
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

#ifdef USE_MANUAL_WEATHER
#include "SparkFun_Weather_Meter_Kit_Arduino_Library.h"
uint8_t windDirectionPin = 25;
uint8_t windSpeedPin = 26;
uint8_t rainfallPin = 27;
SFEWeatherMeterKit weatherMeterKit(windDirectionPin, windSpeedPin, rainfallPin);
#endif

//PENTING ini ID DEVICE
uint8_t id = 4;

// Delay with millis
unsigned long lastTime = 0;
uint16_t timerDelay = 60000; // Atur delay

// Pin dan LED indicator
uint8_t ledPin = 2; // GPIO 2

void processData(AsyncResult &aResult);

WiFiClientSecure ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;
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
#ifdef USE_MANUAL_WEATHER
  float rainFall = 0, //Total in One Day
        rainRate = 0, //Total in One Hour
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
#ifdef USE_MANUAL_WEATHER
    // Here we create a struct to hold all the calibration parameters
    SFEWeatherMeterKitCalibrationParams calibrationParams = weatherMeterKit.getCalibrationParams();
    
    // The wind vane has 8 switches, but 2 could close at the same time, which
    // results in 16 possible positions. Each position has a resistor connected
    // to GND, so this library assumes a voltage divider is created by adding
    // another resistor to VCC. Some of the wind vane resistor values are
    // fairly close to each other, meaning an accurate ADC is required. However
    // some ADCs have a non-linear behavior that causes this measurement to be
    // inaccurate. To account for this, the vane resistor values can be manually
    // changed here to compensate for the non-linear behavior of the ADC
    calibrationParams.vaneADCValues[WMK_ANGLE_0_0] = 3143;
    calibrationParams.vaneADCValues[WMK_ANGLE_22_5] = 1624;
    calibrationParams.vaneADCValues[WMK_ANGLE_45_0] = 1845;
    calibrationParams.vaneADCValues[WMK_ANGLE_67_5] = 335;
    calibrationParams.vaneADCValues[WMK_ANGLE_90_0] = 372;
    calibrationParams.vaneADCValues[WMK_ANGLE_112_5] = 264;
    calibrationParams.vaneADCValues[WMK_ANGLE_135_0] = 738;
    calibrationParams.vaneADCValues[WMK_ANGLE_157_5] = 506;
    calibrationParams.vaneADCValues[WMK_ANGLE_180_0] = 1149;
    calibrationParams.vaneADCValues[WMK_ANGLE_202_5] = 979;
    calibrationParams.vaneADCValues[WMK_ANGLE_225_0] = 2520;
    calibrationParams.vaneADCValues[WMK_ANGLE_247_5] = 2397;
    calibrationParams.vaneADCValues[WMK_ANGLE_270_0] = 3780;
    calibrationParams.vaneADCValues[WMK_ANGLE_292_5] = 3309;
    calibrationParams.vaneADCValues[WMK_ANGLE_315_0] = 3548;
    calibrationParams.vaneADCValues[WMK_ANGLE_337_5] = 2810;

    // The rainfall detector contains a small cup that collects rain water. When
    // the cup fills, the water is dumped and the total rainfall is incremented
    // by some value. This value defaults to 0.2794mm of rain per count, as
    // specified by the datasheet
    calibrationParams.mmPerRainfallCount = 0.2794;

    // The rainfall detector switch can sometimes bounce, causing multiple extra
    // triggers. This input is debounced by ignoring extra triggers within a
    // time window, which defaults to 100ms
    calibrationParams.minMillisPerRainfall = 100;

    // The anemometer contains a switch that opens and closes as it spins. The
    // rate at which the switch closes depends on the wind speed. The datasheet
    // states that a wind of 2.4kph causes the switch to close once per second
    calibrationParams.kphPerCountPerSec = 2.4;

    // Because the anemometer generates discrete pulses as it rotates, it's not
    // possible to measure the wind speed exactly at any point in time. A filter
    // is implemented in the library that averages the wind speed over a certain
    // time period, which defaults to 1 second. Longer intervals result in more
    // accurate measurements, but cause delay in the measurement
    calibrationParams.windSpeedMeasurementPeriodMillis = 1000;

    // Now we can set all the calibration parameters at once
    weatherMeterKit.setCalibrationParams(calibrationParams);

    // Begin weather meter kit
    weatherMeterKit.begin();
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
  #ifdef DEBUG
    //maxlipo.setActivityThreshold(0.15);
    Serial.print(F("Activity threshold = ")); 
    Serial.print(maxWin.getActivityThreshold()); 
    Serial.println(" V change");
    //maxlipo.setHibernationThreshold(5);
    Serial.print(F("Hibernation threshold = "));
    Serial.print(maxWin.getHibernationThreshold()); 
    Serial.println(" %/hour");
  #endif
  dewPoint = calculateDewPoint(temperature, humidity);
#ifdef USE_RAINFALL_SENSOR
  // Update data sensor curah hujan
  //sensorWorkingTime = Sensor.getSensorWorkingTime() * 60; // Total waktu sensor bekerja (detik)     // Total curah hujan (mm)
  rainRate = Sensor.getRainfall(1);          
  delay(1000);
  rainFall = Sensor.getRainfall(24);          
  //rawData = Sensor.getRawData();   
#endif
#ifdef USE_MANUAL_WEATHER
  windDirection = weatherMeterKit.getWindDirection();
  windSpeed = weatherMeterKit.getWindSpeed();
  rainRate =weatherMeterKit.getTotalRainfall();
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

    initializeApp(aClient, app, getAuth(user_auth), processData, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");   
}

void FirebaseData() {
  // Update NTP time
  unsigned long timestamp;
  timestamp = getTime();// Get current epoch time

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
  Database.set<object_t>(aClient, dbPath.c_str(), object_t(dataCuaca), processData, "setTask");
}

void processData(AsyncResult &aResult)
{
    // Exits when no result available when calling from the loop.
    if (!aResult.isResult())
        return;

    if (aResult.isEvent())
    {
        Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.eventLog().message().c_str(), aResult.eventLog().code());
    }

    if (aResult.isDebug())
    {
        Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
    }

    if (aResult.isError())
    {
        Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
    }

    if (aResult.available())
    {
        Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(), aResult.c_str());
    }
}

void Windy() {
  const char* windy_ca= \
    "-----BEGIN CERTIFICATE-----\n" \
  "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\n"\
  "-----END CERTIFICATE-----\n" ;
  
        WiFiClientSecure client;
        client.setCACert(windy_ca);
        HTTPClient https;
  
        String serverPath = "https://stations.windy.com/pws/update/eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJjaSI6MTM4MDQwODUsImlhdCI6MTcyNjc2MjI0M30.RQE7e9Qzy6rhAm6EOy577poYuNdKCN05k03bKaEKejA?";
        serverPath += "temp=" + String(temperature); //suhu (Celcius)
        serverPath += "&humidity=" + String(humidity); //kelembapan (Persen)
        serverPath += "&mbar=" + String(pressure); //tekanan (hPa)
        serverPath += "&dewpoint=" + String(dewPoint); //titik embun (Celcius)
        serverPath += "&wind=" + String(5); //kecepatan angin (m/s)
        serverPath += "&winddir=" + String(0); //arah angin (derajat)
        #ifdef USE_RAINFALL_SENSOR
        serverPath += "&precip=" + String(rainRate); //curah hujan (mm) tiap jam
        #endif
        serverPath += "&station=" + String(id); //ID Station
        
        https.addHeader("Content-Type", "text/plain");
        https.begin(client, serverPath.c_str());
  
        // Send HTTP GET request
        int ResponseCode = https.GET();
        
        if (ResponseCode>0) {
          Serial.print("Windy Response code: ");
          Serial.println(ResponseCode);
          //String payload = https.getString();
          //Serial.println(payload);
        }
        else {
          Serial.print("Windy Error code: ");
          Serial.println(ResponseCode);
        }
        // Free resources
        //Serial.println(serverPath);
        https.end();
  }


/*
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
        body { font-family: 'Roboto', Arial, sans-serif; background: linear-gradient(to bottom, #ffe0b2, #ffcc80); color: #333; padding: 20px; }
        header { text-align: center; margin-bottom: 30px; }
        header h1 { font-size: 2.5rem; font-weight: bold; color: #e65100; margin-bottom: 10px; }
        header h2 { font-size: 1.3rem; color: #ef6c00; margin-bottom: 10px; }
        .datetime { text-align: right; font-size: 1.2rem; color: #fb8c00; margin-bottom: 20px; font-weight: bold; }
        .grid-container { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin: 0 auto; max-width: 1200px; }
        .card { background: #ffffff; border-radius: 12px; padding: 20px; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.15); text-align: center; transition: transform 0.3s ease, box-shadow 0.3s ease; }
        .card:hover { transform: translateY(-5px); box-shadow: 0 6px 15px rgba(0, 0, 0, 0.2); }
        .card h3 { font-size: 1.3rem; color: #ff8f00; margin-bottom: 15px; }
        .card .value { font-size: 2rem; font-weight: bold; color: #e65100; margin-bottom: 5px; }
        .card small { font-size: 0.9rem; color: #ffb74d; }
        footer { text-align: center; margin-top: 30px; font-size: 1rem; color: #ff8f00; }
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
      </div>
      <div class="grid-container">
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
        <div class="card">
          <h3>Voltage</h3>
          <div class="value">)rawliteral";

  html += String(voltage, 2); // Tambahkan data dinamis
  html += R"rawliteral(</div>
          <small>V</small>
        </div>
      </div>
      <footer>Last Update: --/--/---- --:--:-- (UTC)</footer>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", html);
}*/

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

  server.on("/", []() {
    server.send(200, "text/plain", "Halo ini Sensor dengan ID = " + String(id) + "");
  });
 
  ElegantOTA.begin(&server);    // Start ElegantOTA
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

    Windy();

    #ifdef DEBUG
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
    #endif


    #ifdef USE_RAINFALL_SENSOR
    rainFall = 0.00;
    rainRate = 0.00;
    #endif

    lastTime = millisSekarang;
    digitalWrite(ledPin, LOW);
    
  }
}
