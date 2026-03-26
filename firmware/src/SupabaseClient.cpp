#include "SupabaseClient.h"
#include <WiFi.h>
#include <SPIFFS.h>
#include <time.h>  

#define STORAGE_FILE "/data.txt"
#define TEMP_FILE "/temp.txt"

SupabaseClient::SupabaseClient(const char* url, const char* key, const char* telemetryUrl, const char* logsUrl)
  : _url(url), _key(key), _telemetryUrl(telemetryUrl), _logsUrl(logsUrl), _timeClient(_ntpUDP) {}

void SupabaseClient::initStorage() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
  } else {
    Serial.println("SPIFFS initialized");
  }
  _timeClient.begin();
  _timeClient.setTimeOffset(0);
}

// 🔹 helper: ISO8601 time (local timezone)
String SupabaseClient::getISOTime() {
  _timeClient.update();
  time_t now = _timeClient.getEpochTime();
  struct tm *timeinfo = localtime(&now);  // ✔ Changed from gmtime to localtime

  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo);
  return String(buffer);
}

// 🔹 zapis offline (JUŻ z created_at)
void SupabaseClient::saveSensorDataOffline(float temperature, float humidity, int soil1, int soil2, int soil3) {
  File file = SPIFFS.open(STORAGE_FILE, "a");
  if (!file) {
    Serial.println("Failed to open storage file");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["device_id"] = _deviceId;
  doc["created_at"] = getISOTime();   // ✔ KLUCZOWE
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["soil_moisture_1"] = soil1;
  doc["soil_moisture_2"] = soil2;
  doc["soil_moisture_3"] = soil3;

  serializeJson(doc, file);
  file.println();
  file.close();

  Serial.println("Saved offline");
}

// 🔹 wysyłanie z retry (BEZ utraty danych)
void SupabaseClient::sendStoredData() {
  if (WiFi.status() != WL_CONNECTED) return;

  File input = SPIFFS.open(STORAGE_FILE, "r");
  if (!input || input.size() == 0) return;

  File temp = SPIFFS.open(TEMP_FILE, "w");
  if (!temp) {
    Serial.println("Failed to open temp file");
    return;
  }

  while (input.available()) {
    String line = input.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    HTTPClient http;
    http.begin(_url);
    http.setTimeout(5000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", _key);
    http.addHeader("Authorization", "Bearer " + String(_key));

    int code = http.POST(line);

    if (code == 200 || code == 201) {
      _successfulUploads++;
    } else {
      _failedUploads++;
      temp.println(line); // ✔ zachowaj nieudane
    }

    http.end();
  }

  input.close();
  temp.close();

  SPIFFS.remove(STORAGE_FILE);
  SPIFFS.rename(TEMP_FILE, STORAGE_FILE);

  Serial.println("Retry cycle finished");
}

// 🔹 wysyłka danych live
void SupabaseClient::sendSensorData(float temperature, float humidity, int soil1, int soil2, int soil3) {
  if (WiFi.status() != WL_CONNECTED) {
    _failedUploads++;
    sendLog("WARN", "wifi", "No WiFi, saving offline");
    saveSensorDataOffline(temperature, humidity, soil1, soil2, soil3);
    return;
  }

  StaticJsonDocument<256> doc;
  doc["device_id"] = _deviceId;
  doc["created_at"] = getISOTime();
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["soil_moisture_1"] = soil1;
  doc["soil_moisture_2"] = soil2;
  doc["soil_moisture_3"] = soil3;

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.begin(_url);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", _key);
  http.addHeader("Authorization", "Bearer " + String(_key));

  int code = http.POST(body);
  String response = http.getString();  // 🔥 DODAJ

  if (code == 200 || code == 201) {
    _successfulUploads++;
    Serial.printf("Data sent: %d | Success: %d\n", code, _successfulUploads);
  } else {
    _failedUploads++;
    Serial.printf("Send failed: %d | Failed: %d\n", code, _failedUploads);
    char ctx[64];
    snprintf(ctx, sizeof(ctx), "{\"http_code\":%d}", code);
    sendLog("ERROR", "supabase", "HTTP POST failed", ctx);
    saveSensorDataOffline(temperature, humidity, soil1, soil2, soil3);
  }

  // 🔥 DODAJ ZAWSZE (debug)
  Serial.println("Response: " + response);

  http.end();
}

// 🔹 telemetry (rozszerzona)
void SupabaseClient::sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED || _telemetryUrl == nullptr) return;

  uint32_t total = _successfulUploads + _failedUploads;
  if (total == 0) return;

  float espTempF = temperatureRead();
  float espTemp = (espTempF - 32) / 1.8;
  
  // Raw memory data
  uint32_t spiffsUsed = SPIFFS.usedBytes();
  uint32_t spiffsTotal = SPIFFS.totalBytes();
  uint32_t heapFree = ESP.getFreeHeap();
  uint32_t heapMin = ESP.getMinFreeHeap();

  StaticJsonDocument<512> doc;
  doc["device_id"] = _deviceId;
  doc["created_at"] = getISOTime();
  doc["uptime"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();
  doc["esp_temperature"] = espTemp;
  doc["spiffs_used_bytes"] = spiffsUsed;
  doc["spiffs_total_bytes"] = spiffsTotal;
  doc["heap_free_bytes"] = heapFree;
  doc["heap_min_free_bytes"] = heapMin;
  doc["successful_uploads"] = _successfulUploads;
  doc["failed_uploads"] = _failedUploads;
  doc["total_attempts"] = total;

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.begin(_telemetryUrl);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", _key);
  http.addHeader("Authorization", "Bearer " + String(_key));

  int code = http.POST(body);

  if (code != 200 && code != 201) {
    Serial.printf("Telemetry failed: %d\n", code);
    char ctx[64];
    snprintf(ctx, sizeof(ctx), "{\"http_code\":%d}", code);
    sendLog("ERROR", "supabase", "Telemetry POST failed", ctx);
  }

  http.end();
}

// 🔹 debug
void SupabaseClient::printTelemetry() const {
  uint32_t total = _successfulUploads + _failedUploads;

  Serial.println("--- Telemetry ---");
  Serial.printf("OK: %d | FAIL: %d | TOTAL: %d\n",
                _successfulUploads,
                _failedUploads,
                total);
}

bool SupabaseClient::isTimeReady() {
  _timeClient.update();
  return _timeClient.getEpochTime() > 100000;
}

void SupabaseClient::sendLog(const char* level, const char* source, const char* message, const char* context) {
  if (WiFi.status() != WL_CONNECTED || _logsUrl == nullptr) return; // logi nie są buforowane offline

  StaticJsonDocument<512> doc;
  doc["device_id"] = _deviceId;
  doc["created_at"] = getISOTime();
  doc["level"] = level;
  doc["source"] = source;
  doc["message"] = message;
  if (context != nullptr) doc["context"] = context; // np. JSON jako string

  String body;
  serializeJson(doc, body);

  HTTPClient http;
  http.begin(_logsUrl); // osobny endpoint: /rest/v1/logs
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", _key);
  http.addHeader("Authorization", "Bearer " + String(_key));
  http.POST(body);
  http.end();
}