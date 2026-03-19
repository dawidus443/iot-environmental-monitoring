# ESP32 Monitoring System (DHT22, Soil Sensors )

A comprehensive IoT monitoring system built on ESP32 that collects environmental data from multiple sensors and stores it in Supabase cloud database.

## Overview

This project monitors environmental conditions using an ESP32 microcontroller with the following capabilities:
- **Temperature & Humidity Detection** via DHT11 sensor
- **Soil Moisture Monitoring** across 3 different soil sensors
- **Cloud Data Synchronization** with automatic retry mechanism
- **Offline Data Storage** with automatic sync when WiFi reconnects
- **WiFi Management** with exponential backoff reconnection strategy
- **System Telemetry** - tracks upload success rates and system health metrics

## Hardware Requirements

- **ESP32 Development Board** (ESP32-DevKit-C)
- **DHT11 Temperature/Humidity Sensor** (connected to GPIO 13)
- **3x Soil Moisture Sensors** (connected to GPIO 34, 35, and 32)
- **Power Supply** (USB or 5V power adapter)

## Software Stack

- **Platform**: PlatformIO with Arduino Framework
- **Target Board**: ESP32 Dev Module
- **Baud Rate**: 115200
- **Key Libraries**:
  - [Adafruit DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
  - [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor)
  - [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

## Project Structure

```
esp32-dht22/
├── platformio.ini           # PlatformIO configuration
├── include/
│   ├── README
│   └── secrets.h           # WiFi and Supabase credentials -> .gitignore
├── src/
│   ├── main.cpp            # Main application logic
│   ├── SoilSensor.cpp      # Soil sensor implementation
│   ├── SoilSensor.h        # Soil sensor interface
│   ├── WiFiManager.cpp     # WiFi connection manager
│   ├── WiFiManager.h       # WiFi manager interface
│   ├── SupabaseClient.cpp  # Supabase API client
│   └── SupabaseClient.h    # Supabase client interface
├── lib/
│   └── README
└── test/
    └── README
```

## Key Components

### SoilSensor
Manages analog readings from soil moisture sensors with calibration support.
- Default calibration: 2600 (dry) to 830 (wet)
- Provides both raw ADC values and percentage moisture

### WiFiManager
Handles WiFi connectivity with intelligent retry logic.
- Exponential backoff on connection failures
- Automatic reconnection if signal is lost
- Tracks failed connection attempts

### SupabaseClient
Manages cloud data synchronization using Supabase REST API.
- Stores failed uploads locally using SPIFFS filesystem
- Automatic retry of previously failed uploads when WiFi reconnects
- Telemetry tracking with periodic health reports
- Sends system metrics every 10 cycles (~50 minutes)

### Main Loop
```cpp
- Runs every 5 minutes (~300 seconds)
- Reads sensors (temperature, humidity, soil moisture)
- Syncs any stored offline data to Supabase
- Sends current readings to Supabase sensor_data table
- Sends telemetry metrics every 10 cycles (~50 minutes)
- Automatically reconnects to WiFi if needed
```

## Configuration

### WiFi and Database Credentials
Update `include/secrets.h` with your credentials:
```cpp
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASSWORD "YOUR_PASSWORD"
#define SUPABASE_URL "YOUR_SUPABASE_REST_ENDPOINT/rest/v1/sensor_data"
#define SUPABASE_TELEMETRY_URL "YOUR_SUPABASE_REST_ENDPOINT/rest/v1/telemetry"
#define SUPABASE_KEY "YOUR_SUPABASE_PUBLIC_KEY"
```

**Note:** You need both endpoints - one for sensor data and one for telemetry metrics.

### Sensor Calibration
Adjust soil sensor calibration in `main.cpp`:
```cpp
SoilSensor soil1(34, 2600, 830);  // (pin, dryValue, wetValue)
```

### Measurement Interval
Change the loop delay in `main.cpp`:
```cpp
delay(300000);  // 5 minutes in milliseconds
```

## Setup & Installation

### 1. Supabase Database Setup

Create two tables in your Supabase project using SQL Editor:

**Sensor Data Table:**
```sql
create table sensor_data (
  id bigint primary key generated always as identity,
  created_at timestamp with time zone default now(),
  temperature float not null,
  humidity float not null,
  soil_moisture_1 int,
  soil_moisture_2 int,
  soil_moisture_3 int
);

create index idx_sensor_data_created_at on sensor_data(created_at desc);
alter table sensor_data enable row level security;
create policy "Allow inserts from app" on sensor_data for insert with check (true);
```

**Telemetry Table:**
```sql
create table telemetry (
  id bigint primary key generated always as identity,
  created_at timestamp with time zone default now(),
  successful_uploads int not null,
  failed_uploads int not null,
  success_rate float not null,
  total_attempts int not null
);

create index idx_telemetry_created_at on telemetry(created_at desc);
alter table telemetry enable row level security;
create policy "Allow inserts from app" on telemetry for insert with check (true);
```

### 2. ESP32 Setup

1. **Install PlatformIO** on your system or VS Code extension

2. **Clone or download this project**

3. **Configure credentials** in `include/secrets.h`:
   - Get REST API URL from Supabase project settings
   - Append `/rest/v1/sensor_data` and `/rest/v1/telemetry` to the base URL

4. **Upload to ESP32**:
   ```bash
   platformio run --target upload
   ```

5. **Monitor serial output** (115200 baud):
   ```bash
   platformio device monitor
   ```

## Features

✅ **Dual-mode operation**: Offline storage + Cloud sync  
✅ **Fault tolerance**: Automatic WiFi reconnection with backoff  
✅ **Real-time monitoring**: 5-minute sensor read interval  
✅ **Multi-sensor support**: Temperature, humidity, and 3 soil moisture sensors  
✅ **Cloud integration**: Seamless Supabase synchronization  
✅ **Low power design**: Optimized delay between readings  
✅ **System telemetry**: Automatic health metrics collection  
✅ **Data persistence**: SPIFFS offline storage with automatic retry

## Telemetry & Monitoring

### What Gets Tracked
- **Successful uploads** - number of successful data sends to Supabase
- **Failed uploads** - number of failed data transmissions
- **Success rate** - percentage of successful uploads
- **Total attempts** - total number of upload attempts
- **Timestamps** - when each telemetry snapshot was taken

### Telemetry Behavior
- Telemetry is automatically sent to Supabase every 10 measurement cycles (~50 minutes)
- Stored in the `telemetry` table for analysis
- Metrics reset when ESP32 restarts
- View real-time stats in serial monitor: `supabase.printTelemetry()`

### Use Cases for Telemetry Data
✅ **System Health Monitoring** - detect WiFi or connectivity issues  
✅ **Data Engineering** - analyze data quality and collection reliability  
✅ **Alerting** - set up notifications if success_rate drops below threshold  
✅ **Historical Analysis** - track system performance over time  
✅ **SLA Tracking** - measure uptime and reliability metrics

## Troubleshooting

### DHT Sensor Errors
- Check wiring on GPIO 13
- Ensure sensor is powered correctly
- Verify DHT11 (not DHT22) is selected

### WiFi Connection Issues
- Check SSID and password in `secrets.h`
- Monitor exponential backoff delays in serial output
- Check router WiFi signal strength
- Watch for reconnection patterns in telemetry

### Data Upload Failures
- Verify both Supabase URLs in `secrets.h` (sensor_data and telemetry endpoints)
- Check internet connectivity
- Review offline storage status in serial monitor
- Check `failed_uploads` count in telemetry table

### Telemetry Not Registering
- Ensure `SUPABASE_TELEMETRY_URL` is configured
- Check `telemetry` table exists in Supabase
- Verify Row Level Security policies are set to allow inserts
- Telemetry sends every 10 cycles (~50 min) - wait if needed

### Soil Sensor Calibration
- Record raw values for dry and wet soil
- Update calibration constants in `SoilSensor` constructor
- Test percentage readings against actual moisture levels
- Use serial monitor to view raw ADC values

## License

Open source project. Follow the terms of included license.

## Author

Created using ESP32, Arduino Framework, and Supabase

## Data Flow Architecture

```
┌─────────────────┐
│  Sensors        │
│ (DHT11 + 3x     │
│  Soil)          │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  ESP32          │
│  - Read values  │
│  - Check WiFi   │
└────────┬────────┘
         │
    ┌────┴────┐
    │          │
    ▼          ▼
Online?    Offline?
    │          │
    ▼          ▼
Supabase   SPIFFS
  Send     Storage
   Data      JSON
    │          │
    └────┬─────┘
         │
    ┌────┴──────────┐
    │                │
    ▼                ▼
sensor_data      telemetry
 (every 5m)    (every 50m)
```

---

**Last Updated**: March 2026  
**Version**: 2.0 (with Telemetry)
