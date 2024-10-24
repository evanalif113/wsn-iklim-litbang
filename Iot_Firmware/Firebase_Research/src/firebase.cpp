#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>) || defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#elif __has_include(<WiFiC3.h>) || defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif __has_include(<WiFi.h>)
#include <WiFi.h>
#endif

#include <FirebaseClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define WIFI_SSID "server"
#define WIFI_PASSWORD "jeris6467"

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

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFiClientSecure.h>
WiFiClientSecure ssl_client1, ssl_client2;
#elif defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_GIGA) || defined(ARDUINO_OPTA) || defined(ARDUINO_PORTENTA_C33) || defined(ARDUINO_NANO_RP2040_CONNECT)
#include <WiFiSSLClient.h>
WiFiSSLClient ssl_client1, ssl_client2;
#endif

using AsyncClient = AsyncClientClass;

AsyncClient aClient(ssl_client1, getNetwork(network)), aClient2(ssl_client2, getNetwork(network));
RealtimeDatabase Database;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);  // UTC+7

unsigned long ms = 0;

void setup()
{
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
    Serial.println("Initializing app...");

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
    ssl_client1.setInsecure();
    ssl_client2.setInsecure();
#if defined(ESP8266)
    ssl_client1.setBufferSizes(4096, 1024);
    ssl_client2.setBufferSizes(4096, 1024);
#endif
#endif

    initializeApp(aClient2, app, getAuth(user_auth), asyncCB, "authTask");

    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);

    Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");

    timeClient.begin();  // Initialize NTP Client
}

void loop() {
    app.loop();
    Database.loop();
    timeClient.update();  // Update NTP time

    if (millis() - ms > 10000 && app.ready()) {
        ms = millis();
        String timestamp = String(timeClient.getEpochTime()); // Get current epoch time
        JsonWriter writer;
        object_t json, temp, humi, press, dew, volt, times;

        writer.create(temp, "suhu", random(20, 35));
        writer.create(humi, "kelembapan", random(40, 80));
        writer.create(press, "tekanan", random(1000, 1010));
        writer.create(dew, "embun", random(20,25));
        writer.create(volt, "volt",random(0,4));
        writer.create(times, "timestamp",timestamp);

        writer.join(json, 6, temp, humi, press, dew, volt, times);

        

        // Dynamically use timestamp in the path
        String dbPath = "/test/stream/" + timestamp;
        Database.set<object_t>(aClient2, dbPath.c_str(), json, asyncCB, "setTask");
    }
}

void asyncCB(AsyncResult &aResult){
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

#if defined(ESP32) || defined(ESP8266)
        Firebase.printf("Free Heap: %d\n", ESP.getFreeHeap());
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
        Firebase.printf("Free Heap: %d\n", rp2040.getFreeHeap());
#endif
    }
}
