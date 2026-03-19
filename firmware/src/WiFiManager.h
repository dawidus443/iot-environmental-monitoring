#pragma once
#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
  void connect(const char* ssid, const char* password);
  bool isConnected();
  void reconnectIfNeeded(const char* ssid, const char* password);
  uint32_t getFailedAttempts() const { return _failedAttempts; }
  void resetFailureTracking() { _failedAttempts = 0; _backoffMultiplier = 1; }

private:
  uint32_t _failedAttempts = 0;
  uint32_t _backoffMultiplier = 1;
  uint32_t getBackoffDelay() const;
};