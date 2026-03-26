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
SupabaseClient supabase(SUPABASE_URL, SUPABASE_KEY, SUPABASE_TELEMETRY_URL, SUPABASE_LOGS_URL);

// State
float prevTemperature = 0.0;
float prevHumidity = 0.0;
bool firstRun = true;
int fastModeCounter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();
  soil1.begin();
  soil2.begin();
  soil3.begin();

  // WiFi first
  wifi.connect(WIFI_SSID, WIFI_PASSWORD);

  String mac = WiFi.macAddress();
  Serial.println("Device ID (MAC): " + mac);

  supabase.initStorage();
  supabase.setDeviceId(mac);

  // 🔥 Poczekaj na sensowny czas (NTP)
  Serial.println("Syncing time...");
  while (!supabase.isTimeReady()) {
    delay(500);
  }
  Serial.println("Time synced");

  // Set Poland timezone (CET/CEST with DST)
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0", 1);
  tzset();
}

void loop() {

  // 1. reconnect WiFi
  wifi.reconnectIfNeeded(WIFI_SSID, WIFI_PASSWORD);

  // 2. retry offline data
  supabase.sendStoredData();

  // 3. read sensors
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT sensor error!");
    supabase.sendLog("ERROR", "sensor", "DHT read failed");
    delay(10000);
    return;
  }

  int s1 = soil1.readPercent();
  int s2 = soil2.readPercent();
  int s3 = soil3.readPercent();

  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print(" C | Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // 4. send data (event time = teraz, bo nie ma delay przed)
  supabase.sendSensorData(temperature, humidity, s1, s2, s3);

  // 5. telemetry co ~1h (12 cykli)
  static uint32_t cycle = 0;
  if (++cycle % 12 == 0) {
    supabase.sendTelemetry();
  }

  // 6. dynamic interval logic
  unsigned long delayTime = 300000; // default 5 min

  if (firstRun) {
    firstRun = false;
  } else {
    float tempDiff = abs(temperature - prevTemperature);
    float humDiff = abs(humidity - prevHumidity);

    if (tempDiff >= 1.0 || humDiff >= 3.0) {
      fastModeCounter = 5;
      const char* reason = (tempDiff >= 1.0) ? "{\"reason\":\"temp_change\"}" : "{\"reason\":\"humidity_change\"}";
      supabase.sendLog("INFO", "sensor", "Fast mode triggered", reason);
    }
  }

  if (fastModeCounter > 0) {
    delayTime = 60000; // 1 min
    fastModeCounter--;
  }

  // update previous values
  prevTemperature = temperature;
  prevHumidity = humidity;

  Serial.print("Next measurement in ");
  Serial.print(delayTime / 1000);
  Serial.println(" seconds");

  // 7. delay NA KOŃCU (kluczowe)
  delay(delayTime);
}