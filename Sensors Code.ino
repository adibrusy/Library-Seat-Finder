#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include "RadarSensor.h"

// ----------- 1. CONFIGURATION -----------
#define WIFI_SSID "Ais iPhone 16"
#define WIFI_PASSWORD "mmmmmmmm"
#define FIREBASE_HOST "utp-library-seat-finder-default-rtdb.asia-southeast1.firebasedatabase.app" 
#define FIREBASE_AUTH "4FpjVkKu1JPKHQlJksjREnMhz52WJi7Z0n0ECcCt"

// ----------- 2. PIN SETUP -----------
// Seat 1: Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18
const int ultraDetectionLimit = 100; // 100cm

// Seat 2: Radar Sensor
#define RXD_RADAR 16
#define TXD_RADAR 17
const int radarDetectionLimit = 1000; // 1000mm (100cm)

// ----------- 3. OBJECTS -----------
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;
RadarSensor radar(Serial1);
RadarTarget target;

// Helper function for Seat 1 (Ultrasonic)
float getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  // Distance formula: (duration * speed of sound) / 2
  float distance = duration * 0.034 / 2;
  return distance;
}

void setup() {
  Serial.begin(115200);
  
  // Initialize Ultrasonic Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize Radar
  Serial1.begin(256000, SERIAL_8N1, RXD_RADAR, TXD_RADAR);
  radar.begin();

  // WiFi Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // Initialize Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  static unsigned long lastPush = 0;

  // Only run logic every 2 seconds to keep it stable
  if (millis() - lastPush > 2000) {
    
    // --- SEAT 1 LOGIC (Ultrasonic) ---
    float ultraDist = getUltrasonicDistance();
    String status1 = (ultraDist > 0 && ultraDist <= ultraDetectionLimit) ? "occupied" : "available";
    
    if (Firebase.setString(firebaseData, "/seats/seat1", status1)) {
      Serial.println("Cloud Sync -> Seat 1 (Ultra): " + status1 + " (" + String(ultraDist) + "cm)");
    }

    // --- SEAT 2 LOGIC (Radar) ---
    if (radar.update()) {
      target = radar.getTarget();
      String status2 = (target.distance > 0 && target.distance <= radarDetectionLimit) ? "occupied" : "available";
      
      if (Firebase.setString(firebaseData, "/seats/seat2", status2)) {
        Serial.println("Cloud Sync -> Seat 2 (Radar): " + status2);
      }
    }

    lastPush = millis();
  }
}
