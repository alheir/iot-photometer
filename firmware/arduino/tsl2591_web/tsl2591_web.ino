#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

// Try to include secrets.h if present (current dir or project's simple_logger)
#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "secrets.h not found. Define WIFI_SSID,WIFI_PASSWORD,SERVER_URL_LUX (and optional DEVICE_ALIAS) before compiling."
#endif
#else
  // Compiler doesn't support __has_include; try generic include and let the preprocessor handle missing file
#include "secrets.h"
#endif

// #define WIFI_SSID
// #define WIFI_PASSWORD
// #define SERVER_URL_LUX
// #define DEVICE_ALIAS  // Optional

// Ensure required secrets are present at compile time
#ifndef WIFI_SSID
#error "WIFI_SSID is not defined."
#endif
#ifndef WIFI_PASSWORD
#error "WIFI_PASSWORD is not defined."
#endif
#ifndef SERVER_URL_LUX
//   #error "SERVER_URL_LUX is not defined."
#define SERVER_URL_LUX "https://iot-photometer-default-rtdb.firebaseio.com/luminance.json"
#endif
#ifndef DEVICE_ALIAS
#warning "DEVICE_ALIAS is not defined. Using ESP-<chipId> as default alias."
#endif

Adafruit_TSL2591 tsl = Adafruit_TSL2591 (2591);
WiFiClientSecure client;

// Minimal sensor info display
void displaySensorDetails (void) {
    sensor_t sensor;
    tsl.getSensor (&sensor);
    Serial.println ("------------------------------------");
    Serial.print ("Sensor: "); Serial.println (sensor.name);
    Serial.print ("Max Value: "); Serial.print (sensor.max_value); Serial.println (" lux");
    Serial.println ("------------------------------------");
}

// Configure gain/timing (same sensible defaults)
void configureSensor (void) {
    tsl.setGain (TSL2591_GAIN_MED);
    tsl.setTiming (TSL2591_INTEGRATIONTIME_300MS);
    Serial.println ("Sensor configured: 25x gain, 300ms");
}

// Send luminance to webapp (Firebase-like endpoint)
void sendLuminanceToWebApp (float lux) {
    if (WiFi.status () != WL_CONNECTED) {
        Serial.println ("WiFi not connected, skip send");
        return;
    }

    client.setInsecure (); // for self-signed / no CA scenarios
    HTTPClient http;

    String mac = WiFi.macAddress ();
    mac.replace (":", "");

    String alias =
#ifdef DEVICE_ALIAS
        String (DEVICE_ALIAS);
#else
        String ("ESP-") + String (ESP.getChipId (), HEX);
#endif
    alias.replace ("\"", "\\\"");

    // Build per-device endpoint
    String url = String (SERVER_URL_LUX);
    int idx = url.indexOf (".json");
    if (idx != -1) {
        url = url.substring (0, idx) + "/" + mac + ".json";
    }
    else {
        if (url.endsWith ("/")) url += mac + ".json"; else url += "/" + mac + ".json";
    }

    http.begin (client, url);
    http.addHeader ("Content-Type", "application/json");

    time_t now = time (nullptr);
    String payload = "{";
    payload += "\"mac\":\"" + mac + "\",";
    payload += "\"alias\":\"" + alias + "\",";
    payload += "\"lux\":" + String (lux, 2) + ",";
    payload += "\"timestamp\":" + String ((unsigned long)now);
    payload += "}";

    int code = http.PUT (payload);
    Serial.printf ("PUT %s -> code: %d, payload: %s\n", url.c_str (), code, payload.c_str ());

    if (code != 200 && code != 201) {
        String resp = http.getString ();
        Serial.println ("Response body: " + resp);
    }

    http.end ();
}

void setup (void) {
    Serial.begin (115200);
    delay (500);

    Serial.println ("Starting TSL2591 web client...");

    // WiFi connect
    WiFi.begin (WIFI_SSID, WIFI_PASSWORD);
    Serial.print ("Connecting WiFi");
    while (WiFi.status () != WL_CONNECTED) {
        delay (500);
        Serial.print (".");
    }
    Serial.println ();
    Serial.print ("IP: "); Serial.println (WiFi.localIP ());

    // NTP setup (UTC)
    configTime (0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print ("Waiting NTP");
    while (time (nullptr) < 1000000000) { delay (500); Serial.print ("."); }
    Serial.println (" synced");

    // Initial send with -1.0 to mark device online without valid lux
    sendLuminanceToWebApp (-1.0);

    // Sensor init
    Wire.begin ();
    Serial.print ("Waiting TSL2591");
#ifdef LED_BUILTIN
    pinMode (LED_BUILTIN, OUTPUT);
#endif
    while (!tsl.begin ()) {
        Serial.print (".");
#ifdef LED_BUILTIN
        digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN));
#endif
        delay (500);
    }
#ifdef LED_BUILTIN
    digitalWrite (LED_BUILTIN, LOW);
#endif
    Serial.println ("\nTSL2591 detected");
    displaySensorDetails ();
    configureSensor ();
}

void loop (void) {
    sensors_event_t event;
    tsl.getEvent (&event);

    if (event.light) {
        float lux = event.light; // lux value as float
        Serial.printf ("Lux: %.2f\n", lux);
        sendLuminanceToWebApp (lux);
    }
    else {
        Serial.println ("Sensor overload / invalid reading");
    }

    delay (2000);
}
