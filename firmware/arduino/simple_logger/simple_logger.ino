/* Simple Logger for Firebase - Only sends log messages */

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include "secrets.h"  // Include the secrets file
#include <time.h>  // Agrega para NTP

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

WiFiClientSecure client;

// --- NEW: geo globals & one-time flag ---
String geo_country = "";
String geo_region = "";
String geo_city = "";
bool geoSent = false;

/**************************************************************************/
/*
    Sends a luminance value to Firebase
*/
/**************************************************************************/
void sendLuminanceToFirebase(float lux) {
  if (WiFi.status() == WL_CONNECTED) {
    client.setInsecure();
    HTTPClient http;

    // Get MAC for path only
    String mac = WiFi.macAddress();
    mac.replace(":", "");

    // Build per-device endpoint
    String url = String(SERVER_URL_LUX);
    int idx = url.indexOf(".json");
    if (idx != -1) {
      url = url.substring(0, idx) + "/" + mac + ".json";
    } else {
      if (url.endsWith("/")) url += mac + ".json"; else url += "/" + mac + ".json";
    }

    http.begin(client, url);  // https://.../luminance/<mac>.json
    http.addHeader("Content-Type", "application/json");

    time_t now = time(nullptr);
    unsigned long ts = (now > 1000000000UL) ? (unsigned long)now : 0UL;

    String payload = "{";
    payload += "\"lux\":" + String(lux, 2) + ",";
    payload += "\"timestamp\":" + String(ts);
    payload += "}";

    // Use PATCH to preserve children (geo)
    int code = -1;
    #if defined(ESP8266)
      code = http.sendRequest("PATCH", payload);
    #elif defined(ESP32)
      code = http.PATCH(payload);
    #else
      code = http.sendRequest("PATCH", payload);
    #endif

    if (code <= 0) {
      Serial.println("PATCH failed or unsupported; falling back to PUT");
      code = http.PUT(payload);
    }

    Serial.printf("Luminance PATCH/PUT -> code: %d, payload: %s\n", code, payload.c_str());

    if (code != 200 && code != 201) {
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

  // perform IP-based geolocation and send geo+alias once
  geolocateByIP();
  sendGeoInfoToFirebaseOnce();

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

// NEW: simple HTTP GET helper that returns response body
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

// NEW: extract quoted string after key, returns false if not found
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

// NEW: geolocate by IP using ipapi.co and store country/region/city
void geolocateByIP() {
  Serial.println("Starting IP-based geolocation (ipapi.co)...");
  String url = "https://ipapi.co/json/";
  String resp = httpGet(url); // already prints response

  geo_country = "";
  geo_region = "";
  geo_city = "";
  String s;
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

// NEW: send alias + country/region/city once at startup to /<mac>/geo.json (no "mac" in payload)
void sendGeoInfoToFirebaseOnce() {
  if (geoSent) {
    Serial.println("Geo already sent; skipping.");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skip geo send");
    return;
  }
  if (geo_country.length() == 0 && geo_region.length() == 0 && geo_city.length() == 0) {
    Serial.println("No geo info to send");
    return;
  }

  client.setInsecure();
  HTTPClient http;

  String mac = WiFi.macAddress();
  mac.replace(":", "");

  // Build per-device geo endpoint
  String url = String(SERVER_URL_LUX);
  int idx = url.indexOf(".json");
  if (idx != -1) url = url.substring(0, idx);
  if (!url.endsWith("/")) url += "/";
  url += mac + "/geo.json";

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
