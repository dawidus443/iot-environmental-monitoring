#include "SupabaseClient.h"
#include <WiFi.h>
#include <SPIFFS.h>

SupabaseClient::SupabaseClient(const char* url, const char* key, const char* telemetryUrl)
  : _url(url), _key(key), _telemetryUrl(telemetryUrl) {}

void SupabaseClient::initStorage() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed!");
  } else {
    Serial.println("SPIFFS initialized");
  }
}

void SupabaseClient::saveSensorDataOffline(float temperature, float humidity, int soil1, int soil2, int soil3) {
  File file = SPIFFS.open(STORAGE_FILE, "a");
  if (!file) {
    Serial.println("Failed to open storage file for writing");
    return;
  }

  StaticJsonDocument<256> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["soil_moisture_1"] = soil1;
  doc["soil_moisture_2"] = soil2;
  doc["soil_moisture_3"] = soil3;

  serializeJson(doc, file);
  file.println();
  file.close();
  
  Serial.println("Data saved to storage");
}

void SupabaseClient::sendStoredData() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  File file = SPIFFS.open(STORAGE_FILE, "r");
  if (!file || file.size() == 0) {
    return;
  }

  int sentCount = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0) continue;

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, line);
    
    if (error) {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
      continue;
    }

    HTTPClient http;
    http.begin(_url);
    http.setTimeout(5000);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", _key);
    http.addHeader("Authorization", "Bearer " + String(_key));

    String body;
    serializeJson(doc, body);

    int responseCode = http.POST(body);
    
    if (responseCode == 200 || responseCode == 201) {
      _successfulUploads++;
      sentCount++;
      Serial.printf("Stored data sent: %d | Total uploads: %d\n", responseCode, _successfulUploads);
    } else {
      Serial.printf("Failed to send stored data: %d\n", responseCode);
    }
    http.end();
  }

  file.close();

  if (sentCount > 0) {
    SPIFFS.remove(STORAGE_FILE);
    Serial.printf("Cleared storage after sending %d records\n", sentCount);
  }
}

void SupabaseClient::sendSensorData(float temperature, float humidity, int soil1, int soil2, int soil3) {
  if (WiFi.status() != WL_CONNECTED) {
    _failedUploads++;
    saveSensorDataOffline(temperature, humidity, soil1, soil2, soil3);
    Serial.println("No WiFi! Data saved to storage. Failed uploads: " + String(_failedUploads));
    return;
  }

  HTTPClient http;
  http.begin(_url);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", _key);
  http.addHeader("Authorization", "Bearer " + String(_key));

  StaticJsonDocument<256> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["soil_moisture_1"] = soil1;
  doc["soil_moisture_2"] = soil2;
  doc["soil_moisture_3"] = soil3;

  String body;
  serializeJson(doc, body);

  int responseCode = http.POST(body);
  
  if (responseCode == 200 || responseCode == 201) {
    _successfulUploads++;
    Serial.printf("Supabase success: %d | Total uploads: %d\n", responseCode, _successfulUploads);
  } else {
    _failedUploads++;
    Serial.printf("Supabase error: %d | Failed uploads: %d\n", responseCode, _failedUploads);
  }
  http.end();
}

void SupabaseClient::sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (_telemetryUrl == nullptr) {
    Serial.println("Telemetry URL not configured");
    return;
  }

  uint32_t total = _successfulUploads + _failedUploads;
  if (total == 0) {
    return;
  }

  float successRate = (_successfulUploads * 100.0f) / total;

  HTTPClient http;
  http.begin(_telemetryUrl);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", _key);
  http.addHeader("Authorization", "Bearer " + String(_key));

  StaticJsonDocument<256> doc;
  doc["successful_uploads"] = _successfulUploads;
  doc["failed_uploads"] = _failedUploads;
  doc["success_rate"] = successRate;
  doc["total_attempts"] = total;

  String body;
  serializeJson(doc, body);

  int responseCode = http.POST(body);
  
  if (responseCode == 200 || responseCode == 201) {
    Serial.printf("Telemetry sent | Success rate: %.1f%% | Total: %d\n", successRate, total);
  } else {
    Serial.printf("Failed to send telemetry: %d\n", responseCode);
  }
  http.end();
}

void SupabaseClient::printTelemetry() const {
  uint32_t total = _successfulUploads + _failedUploads;
  if (total == 0) {
    Serial.println("--- Supabase Telemetry: No data sent yet ---");
    return;
  }
  float successRate = (_successfulUploads * 100.0f) / total;
  Serial.printf("--- Supabase Telemetry ---\n");
  Serial.printf("Successful: %d | Failed: %d | Success rate: %.1f%%\n", _successfulUploads, _failedUploads, successRate);
}