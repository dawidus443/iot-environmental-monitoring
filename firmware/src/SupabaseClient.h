#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class SupabaseClient {
public:
  SupabaseClient(const char* url, const char* key, const char* telemetryUrl = nullptr);
  void sendSensorData(float temperature, float humidity, int soil1, int soil2, int soil3);
  void saveSensorDataOffline(float temperature, float humidity, int soil1, int soil2, int soil3);
  void sendStoredData();
  void sendTelemetry();
  void initStorage();
  uint32_t getFailedUploads() const { return _failedUploads; }
  uint32_t getSuccessfulUploads() const { return _successfulUploads; }
  void printTelemetry() const;

private:
  const char* _url;
  const char* _key;
  const char* _telemetryUrl;
  uint32_t _failedUploads = 0;
  uint32_t _successfulUploads = 0;
  const char* STORAGE_FILE = "/data.json";
};