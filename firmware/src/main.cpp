#include <Arduino.h>
#include <DHT.h>
#include "secrets.h"
#include "SoilSensor.h"
#include "WiFiManager.h"
#include "SupabaseClient.h"

#define DHTPIN 13
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
SoilSensor soil1(34);
SoilSensor soil2(35);
SoilSensor soil3(32);
WiFiManager wifi;
SupabaseClient supabase(SUPABASE_URL, SUPABASE_KEY, SUPABASE_TELEMETRY_URL);

void setup() {
  Serial.begin(115200);
  dht.begin();
  soil1.begin();

  supabase.initStorage();
  Serial.println("MAC: " + WiFi.macAddress());
  wifi.connect(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  delay(300000); //around 5 mins

  wifi.reconnectIfNeeded(WIFI_SSID, WIFI_PASSWORD);
  
  // Send any stored data from previous offline periods
  supabase.sendStoredData();

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT sensor error!");
    return;
  }

  int s1 = soil1.readPercent();
  int s2 = soil2.readPercent();
  int s3 = soil3.readPercent();

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" C  |  Air humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  supabase.sendSensorData(temperature, humidity, s1, s2, s3);

  // Send telemetry every 10 cycles (~50 minutes)
  static uint32_t cycle = 0;
  if (++cycle % 10 == 0) {
    supabase.sendTelemetry();
  }
}