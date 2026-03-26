#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

class SupabaseClient {
public:
  SupabaseClient(const char* url, const char* key, const char* telemetryUrl = nullptr, const char* logsUrl = nullptr);

  void initStorage();
  void setDeviceId(const String& deviceId) { _deviceId = deviceId; }

  void sendSensorData(float temperature, float humidity, int soil1, int soil2, int soil3);
  void saveSensorDataOffline(float temperature, float humidity, int soil1, int soil2, int soil3);
  void sendStoredData();
  void sendTelemetry();
  void printTelemetry() const;
  void sendLog(const char* level, const char* source, const char* message, const char* context = nullptr);

  uint32_t getFailedUploads() const { return _failedUploads; }
  uint32_t getSuccessfulUploads() const { return _successfulUploads; }

  bool isTimeReady();  // 🔥 DODAJ TO

private:
  String getISOTime();

  const char* _url;
  const char* _key;
  const char* _telemetryUrl;
  const char* _logsUrl;

  uint32_t _failedUploads = 0;
  uint32_t _successfulUploads = 0;

  WiFiUDP _ntpUDP;
  NTPClient _timeClient;

  String _deviceId;
};