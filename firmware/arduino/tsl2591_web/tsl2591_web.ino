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

// NEW globals for geo and one-time send flag
String geo_country = "";
String geo_region = "";
String geo_city = "";
bool geoSent = false; // only send geo+alias once at startup

// --- NEW: geolocation helpers ----------------------------------------------
String httpPostJSON(const String &url, const String &body) {
  HTTPClient http;
  client.setInsecure();
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(body);
  Serial.printf("POST %s -> %d\n", url.c_str(), code);
  String resp = http.getString();
  Serial.println("Response body: " + resp);
  http.end();
  return resp;
}

String httpGet(const String &url) {
  HTTPClient http;
  client.setInsecure();
  http.begin(client, url);
  int code = http.GET();
  Serial.printf("GET %s -> %d\n", url.c_str(), code);
  String resp = http.getString();
  Serial.println("Response body: " + resp);
  http.end();
  return resp;
}

// Helper: extract a floating number after a key like "\"lat\"" or "\"longitude\""
bool extractNumberAfterKey(const String &json, const char *key, float &out) {
  int k = json.indexOf(key);
  if (k < 0) return false;
  int colon = json.indexOf(':', k);
  if (colon < 0) return false;
  int i = colon + 1;
  while (i < json.length() && isSpace(json[i])) i++;
  int start = i;
  bool seen = false;
  while (i < json.length()) {
    char c = json[i];
    if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E') {
      seen = true; i++;
    } else break;
  }
  if (!seen) return false;
  String numStr = json.substring(start, i);
  out = numStr.toFloat();
  return true;
}

// Helper: extract a quoted string after a key like "\"country_name\"" or "\"city\""
bool extractStringAfterKey(const String &json, const char *key, String &out) {
  int k = json.indexOf(key);
  if (k < 0) return false;
  int colon = json.indexOf(':', k);
  if (colon < 0) return false;
  int q1 = json.indexOf('\"', colon);
  if (q1 < 0) return false;
  int q2 = json.indexOf('\"', q1 + 1);
  if (q2 < 0) return false;
  out = json.substring(q1 + 1, q2);
  return true;
}

// Geolocate by IP using ipapi.co (prints and stores parsed JSON fields)
void geolocateByIP() {
  Serial.println("Starting IP-based geolocation (ipapi.co)...");
  String url = "https://ipapi.co/json/";
  String resp = httpGet(url); // httpGet already prints response

  // parse strings: country_name, region, city (ipapi uses "country_name","region","city")
  String s;
  geo_country = "";
  geo_region = "";
  geo_city = "";
  if (extractStringAfterKey(resp, "\"country_name\"", s) || extractStringAfterKey(resp, "country_name", s)) {
    geo_country = s;
    Serial.println("Country: " + geo_country);
  }
  if (extractStringAfterKey(resp, "\"region\"", s) || extractStringAfterKey(resp, "region", s)) {
    geo_region = s;
    Serial.println("Region: " + geo_region);
  }
  if (extractStringAfterKey(resp, "\"city\"", s) || extractStringAfterKey(resp, "city", s)) {
    geo_city = s;
    Serial.println("City: " + geo_city);
  }
}

// send only country/region/city + alias once under per-device path <mac>/geo.json
void sendGeoInfoToWebApp() {
  if (geoSent) {
    Serial.println("Geo already sent; skipping.");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skip geo send");
    return;
  }
  if (geo_country.length()==0 && geo_region.length()==0 && geo_city.length()==0) {
    Serial.println("No geo info to send");
    return;
  }

  client.setInsecure();
  HTTPClient http;

  String mac = WiFi.macAddress();
  mac.replace(":", "");

  // Build per-device geo endpoint (same as before)
  String base = String(SERVER_URL_LUX);
  int idx = base.indexOf(".json");
  if (idx != -1) base = base.substring(0, idx);
  if (!base.endsWith("/")) base += "/";
  String url = base + mac + "/geo.json";

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  time_t now = time(nullptr);
  unsigned long ts = (now > 1000000000UL) ? (unsigned long)now : 0UL;

  String alias =
#ifdef DEVICE_ALIAS
    String(DEVICE_ALIAS);
#else
    String("ESP-") + String(ESP.getChipId(), HEX);
#endif
  alias.replace("\"", "\\\"");

  String payload = "{";
  payload += "\"alias\":\"" + alias + "\",";
  payload += "\"country\":\"" + geo_country + "\",";
  payload += "\"region\":\"" + geo_region + "\",";
  payload += "\"city\":\"" + geo_city + "\",";
  payload += "\"timestamp\":" + String(ts);
  payload += "}";

  int code = http.PUT(payload);
  Serial.printf("Geo PUT %s -> code: %d, payload: %s\n", url.c_str(), code, payload.c_str());
  if (code != 200 && code != 201) {
    String resp = http.getString();
    Serial.println("Geo response: " + resp);
  }
  http.end();

  geoSent = true;
}

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

// Send luminance to webapp
void sendLuminanceToWebApp (float lux) {
    if (WiFi.status () != WL_CONNECTED) {
        Serial.println ("WiFi not connected, skip send");
        return;
    }

    client.setInsecure ();
    HTTPClient http;

    String mac = WiFi.macAddress ();
    mac.replace (":", "");

    // Build per-device endpoint (same path), but payload will NOT contain mac/alias
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
    unsigned long ts = (now > 1000000000UL) ? (unsigned long)now : 0UL;

    String payload = "{";
    payload += "\"lux\":" + String (lux, 2) + ",";
    payload += "\"timestamp\":" + String (ts);
    payload += "}";

    // Use PATCH to update only these fields (preserve children like "geo").
    int code = -1;
    #if defined(ESP8266)
      // sendRequest exists on ESP8266 HTTPClient
      code = http.sendRequest("PATCH", payload);
    #elif defined(ESP32)
      // ESP32 HTTPClient has PATCH method
      code = http.PATCH(payload);
    #else
      // generic try
      code = http.sendRequest("PATCH", payload);
    #endif

    // Fallback: if PATCH failed or unsupported, try PUT (compat fallback)
    if (code <= 0) {
        Serial.println("PATCH failed or unsupported; falling back to PUT");
        code = http.PUT(payload);
    }

    Serial.printf ("PATCH/PUT %s -> code: %d, payload: %s\n", url.c_str (), code, payload.c_str ());

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

    // perform IP-based geolocation at startup, then send geo+alias once
    geolocateByIP();
    sendGeoInfoToWebApp();

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
