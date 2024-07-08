#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <TinyGPS++.h>
#include <time.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// GPS module connection
#define GPS_BAUDRATE 9600
#define RXD2 16  // ESP32 RX2 pin connected to GPS TX
#define TXD2 17  // ESP32 TX2 pin connected to GPS RX

#define WIFI_SSID "POCO X3 GT"
#define WIFI_PASSWORD "12121212"
#define API_KEY "AIzaSyCmvvsdssUXycHNNtql6Xnov1-4vJflF-g"
#define DATABASE_URL "https://dogsgpsnew-default-rtdb.firebaseio.com/"
#define USER_EMAIL "schreiber.omer@gmail.com"
#define USER_PASSWORD "123456"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

TinyGPSPlus gps;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;  // GMT+2
const int   daylightOffset_sec = 7200;
unsigned long sendDataPrevMillis = 0;
int counter=0;

void setup() {
   
  Serial.begin(115200);
  Serial2.begin(GPS_BAUDRATE, SERIAL_8N1, RXD2, TXD2);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // if (Firebase.signUp(&config, &auth, "schreiber.omer@gmail.com", "123456")) {
  //   Serial.println("Firebase sign up ok");
  // } else {
  //   Serial.printf("Firebase sign up failed: %s\n", config.signer.signupError.message.c_str());
  // }
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectWiFi(true);
  
  // Firebase.setMaxRetry(fbdo, 3);
  // Firebase.setMaxErrorQueue(fbdo, 30);
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  
  while (!time(nullptr)) {
    delay(1000);
    Serial.println("Waiting for time sync...");
  }
  Serial.println("Signing in...");
  Firebase.begin(&config, &auth);
  config.token_status_callback = tokenStatusCallback;
}

void loop() {
   
 if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
 }
   
  // Read GPS data
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid() && Firebase.ready()) {
        double latitude = gps.location.lat();
        double longitude = gps.location.lng();
        
        FirebaseJson content;
        time_t now;
        struct tm timeinfo;
        if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
        }
        time(&now);

        // Convert the time to a string
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));


        content.set("fields/latitude/doubleValue", latitude);
        content.set("fields/longitude/doubleValue", longitude);
        content.set("fields/timestamp/stringValue", timestamp);

        String documentPath = "gps_data";
        documentPath+="/";

        if (Firebase.Firestore.createDocument(&fbdo, "dogsgpsnew", "", documentPath, content.raw())) {
          Serial.println("GPS data sent successfully");
          Serial.println(timestamp);
          Serial.printf("Latitude: %f, Longitude: %f\n", latitude, longitude);
          
        } else {
          Serial.println("Failed to send GPS data");
          Serial.print("Reason: ");
          Serial.println(fbdo.errorReason());
        }
        
        delay(60000); // Wait for min before sending data again
      }
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("No GPS detected: check wiring.");
  }
  
}