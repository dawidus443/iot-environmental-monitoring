#include "WiFiManager.h"

void WiFiManager::connect(const char* ssid, const char* password) {
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(500);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (!isConnected()) {
    Serial.println("\nConnection failed! Status: " + String(WiFi.status()));
    return;
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::reconnectIfNeeded(const char* ssid, const char* password) {
  if (!isConnected()) {
    Serial.println("WiFi connection lost, connecting again...");
    connect(ssid, password);
  }
}