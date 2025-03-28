#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "server"
#define WIFI_PASSWORD "jeris6467"

// The API key can be obtained from Firebase console > Project Overview > Project settings.
#define API_KEY "AIzaSyCLnLUN0jSUj7X37VTVJciUHsIyl4sT0-0"

// User Email and password that already registerd or added in your project.
#define USER_EMAIL "esp32@mail.com"
#define USER_PASSWORD "kirim1234"
#define DATABASE_URL "https://database-sensor-iklim-litbang-default-rtdb.asia-southeast1.firebasedatabase.app/"
String id = "3";

void asyncCB(AsyncResult &aResult);
void printResult(AsyncResult &aResult);

DefaultNetwork network; // initilize with boolean parameter to enable/disable network reconnection
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;

using AsyncClient = AsyncClientClass;

WiFiClientSecure ssl_client2;
AsyncClient aClient2(ssl_client2, getNetwork(network));
RealtimeDatabase Database;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);  // UTC+7

unsigned long ms = 0;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(50);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
    Serial.println("Initializing app...");

    ssl_client2.setInsecure();

    initializeApp(aClient2, app, getAuth(user_auth), asyncCB, "authTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    Database.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
    timeClient.begin();  // Initialize NTP Client
}

void loop() {
    app.loop();
    Database.loop();
    
    if (millis() - ms > 10000 && app.ready()) {
        ms = millis();
        timeClient.update();  // Update NTP time
        String timestamp = String(timeClient.getEpochTime()); // Get current epoch time
        JsonWriter writer;
        object_t json, t, h, p, d, v, times;

        writer.create(t, "temp", random(20, 35));
        writer.create(h, "humi", random(40, 80));
        writer.create(p, "pres", random(1000, 1010));
        writer.create(d, "dew", random(20,25));
        writer.create(v, "volt",random(0,4));
        writer.create(times, "timestamp",timestamp);

        writer.join(json, 6, t, h, p, d, v, times);

        // Dynamically use timestamp in the path
        String dbPath = "/auto_weather_stat/id-0"+String(id)+"/data/" + timestamp;
        Database.set<object_t>(aClient2, dbPath.c_str(), json, asyncCB, "setTask");
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
