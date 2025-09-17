/* Simple Logger for Firebase - Only sends log messages */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h"  // Include the secrets file
#include <time.h>  // Agrega para NTP

// Check if secrets.h is included and defined
#ifndef SECRETS_H
#error "secrets.h not found or not properly defined. Please create it with your sensitive data."
#endif

WiFiClientSecure client;

/**************************************************************************/
/*
    Sends a luminance value to Firebase
*/
/**************************************************************************/
void sendLuminanceToFirebase(float lux) {
  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure();
    HTTPClient http;

    // Get MAC and sanitize for use as Firebase key (remove colons)
    String mac = WiFi.macAddress();
    mac.replace(":", "");

    // Alias: from secrets if provided, else default to ESP-<chipId>
    String alias =
    #ifdef DEVICE_ALIAS
      String(DEVICE_ALIAS);
    #else
      String("ESP-") + String(ESP.getChipId(), HEX);
    #endif
    // very simple escape for quotes
    alias.replace("\"", "\\\"");

    // Build per-device endpoint: if SERVER_URL_LUX contains ".json", replace with "/<mac>.json"
    String url = String(SERVER_URL_LUX);
    int idx = url.indexOf(".json");
    if (idx != -1) {
      url = url.substring(0, idx) + "/" + mac + ".json";
    } else {
      if (url.endsWith("/")) url += mac + ".json"; else url += "/" + mac + ".json";
    }

    http.begin(client, url);  // https://.../luminance/<mac>.json
    http.addHeader("Content-Type", "application/json");

    // Use seconds to avoid overflow; webapp normalizes to ms
    time_t now = time(nullptr);
    String payload = "{"
      "\"mac\":\"" + mac + "\","
      "\"alias\":\"" + alias + "\","
      "\"lux\":" + String(lux, 2) + ","
      "\"timestamp\":" + String((unsigned long)now) +
    "}";

    int code = http.PUT(payload);
    Serial.printf("Luminance PUT -> code: %d, payload: %s\n", code, payload.c_str());

    if (code != 200) {
      Serial.printf("Error sending luminance data. Response: %d\n", code);
      String response = http.getString();
      Serial.println("Response body: " + response);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected, cannot send luminance");
  }
}

/**************************************************************************/
/*
    Setup function
*/
/**************************************************************************/
void setup(void) {
  Serial.begin(115200);

  // Connect to WiFi first
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());

  // Configura NTP para obtener tiempo Unix después de WiFi
  configTime(0, 0, "pool.ntp.org");  // UTC, sin offset
  Serial.print("Waiting for NTP time sync");
  while (time(nullptr) < 1000000000) {  // Espera hasta que se sincronice
    delay(1000);
    Serial.print(".");
  }
  Serial.println("NTP synced!");

  // Semilla para random (tras WiFi/NTP ya hay más entropía)
  randomSeed(micros());

  Serial.println("Simple Logger started!");
}

/**************************************************************************/
/*
    Loop function
*/
/**************************************************************************/
void loop(void) {
  // Send a random lux value between 0.00 and 1000.00
  float lux = random(0, 100000) / 100.0;  // 0.00 .. 1000.00
  sendLuminanceToFirebase(lux);
  
  // Add some debug output
  Serial.printf("Sent lux value: %.2f\n", lux);

  delay(2000);  // Send every 2 seconds
}
