# ESP32 Environmental Monitoring System (Data Engineering Ready)

A comprehensive IoT monitoring system built on ESP32 for data engineering applications. Collects environmental data from multiple sensors and stores it in Supabase cloud database with structured telemetry for ETL processing.

## Overview

This project monitors environmental conditions using an ESP32 microcontroller with the following capabilities:
- **Temperature & Humidity Detection** via DHT11 sensor
- **Soil Moisture Monitoring** across 3 different soil sensors
- **Cloud Data Synchronization** with automatic retry mechanism
- **Offline Data Storage** with automatic sync when WiFi reconnects
- **WiFi Management** with exponential backoff reconnection strategy
- **System Telemetry** - tracks upload success rates, device health, and connectivity metrics
- **Data Engineering Ready** - structured data with device IDs, timestamps, and metadata for ETL pipelines

## Data Structure for ETL

### Sensor Data (sensor_data table)
Each record contains:
- `device_id` (text): MAC address of the ESP32 device
- `created_at` (timestamp): Measurement timestamp (Unix epoch, sent from ESP32)
- `uptime` (integer): Device uptime in seconds
- `rssi` (integer): WiFi signal strength in dBm
- `event_type` (text): Always "sensor_data"
- `temperature` (float): Temperature in °C
- `humidity` (float): Air humidity in %
- `soil_moisture_1/2/3` (integer): Soil moisture percentages

### Telemetry Data (telemetry table)
Each record contains:
- `device_id` (text): MAC address of the ESP32 device
- `created_at` (timestamp): Telemetry timestamp (Unix epoch, sent from ESP32)
- `uptime` (integer): Device uptime in seconds
- `rssi` (integer): WiFi signal strength in dBm
- `event_type` (text): Always "telemetry"
- `esp_temperature` (float): Internal ESP32 temperature in °C
- `spiffs_used_bytes` (integer): SPIFFS flash memory used in bytes
- `spiffs_total_bytes` (integer): SPIFFS flash memory total capacity in bytes
- `heap_free_bytes` (integer): Current free heap memory in bytes
- `heap_min_free_bytes` (integer): Minimum free heap since startup in bytes
- `successful_uploads` (integer): Number of successful data uploads
- `failed_uploads` (integer): Number of failed data uploads
- `total_attempts` (integer): Total upload attempts

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
iot-environmental-monitoring/firmware/
├── platformio.ini           # PlatformIO configuration
├── README.md                # This file
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
- Telemetry tracking with periodic health reports including device uptime and WiFi RSSI
- Sends system metrics every 10 cycles (~50 minutes)
- Includes device identification and event type for data engineering

### Main Loop
```cpp
- Runs every 5 minutes (default), or 1 minute for 5 cycles if temperature changes by ≥1°C or humidity by ≥2%
- Runs every 5 minutes (default), or 1 minute for 5 cycles if temperature changes by ≥1°C or humidity by ≥3%
- Counter resets on new changes during fast mode
- Reads sensors (temperature, humidity, soil moisture)
- Syncs any stored offline data to Supabase
- Sends current readings to Supabase sensor_data table with metadata
- Sends telemetry metrics every 10 cycles (~50 minutes) with device health data
- Sends telemetry metrics every 12 cycles (~1 hour) with device health data
- Sends structured logs (ERROR/WARN/INFO) to logs table on events
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
Default: 5 minutes. Dynamic adjustment:
- **1 minute for 5 measurements** if temperature changes by ≥1°C or humidity by ≥2% (detects rapid environmental changes)
- Counter resets if another change is detected during fast mode
- **5 minutes** otherwise (conserves bandwidth and storage)

This optimizes data collection for data engineering — burst of data during events, less during stable periods.

## Setup & Installation

### 1. Supabase Database Setup

Create two tables in your Supabase project using SQL Editor:

**Sensor Data Table:**
```sql
create table sensor_data (
  id bigint primary key generated always as identity,
  created_at timestamp with time zone not null,
  device_id text not null,
  uptime integer not null,
  rssi integer not null,
  event_type text not null,
  temperature float not null,
  humidity float not null,
  soil_moisture_1 integer,
  soil_moisture_2 integer,
  soil_moisture_3 integer
);

create index idx_sensor_data_created_at on sensor_data(created_at desc);
create index idx_sensor_data_device_id on sensor_data(device_id);
alter table sensor_data enable row level security;
create policy "Allow inserts from app" on sensor_data for insert with check (true);
```

**Telemetry Table:**
```sql
create table telemetry (
  id bigint primary key generated always as identity,
  created_at timestamp with time zone not null,
  device_id text not null,
  uptime integer not null,
  rssi integer not null,
  event_type text not null,
  esp_temperature float not null,
  spiffs_used_bytes integer not null,
  spiffs_total_bytes bigint not null,
  heap_free_bytes bigint not null,
  heap_min_free_bytes bigint not null,
  successful_uploads integer not null,
  failed_uploads integer not null,
  total_attempts integer not null
);

create index idx_telemetry_created_at on telemetry(created_at desc);
create index idx_telemetry_device_id on telemetry(device_id);
alter table telemetry enable row level security;
create policy "Allow inserts from app" on telemetry for insert with check (true);
```

### 2. ESP32 Setup

1. **Install PlatformIO** on your system or VS Code extension

2. **Clone or download this project**

3. **Configure credentials** in `include/secrets.h`:
   - Get REST API URL from Supabase project settings
  - Append `/rest/v1/sensor_data`, `/rest/v1/telemetry`, and `/rest/v1/logs` to the base URL

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
✅ **Real-time monitoring**: 5-minute sensor read interval (dynamic: 1 min for 5 cycles on ≥1°C temp or ≥2% humidity change)  
✅ **Real-time monitoring**: 5-minute sensor read interval (dynamic: 1 min for 5 cycles on ≥1°C temp or ≥3% humidity change)
✅ **Multi-sensor support**: Temperature, humidity, and 3 soil moisture sensors  
✅ **Cloud integration**: Seamless Supabase synchronization  
✅ **Low power design**: Optimized delay between readings  
✅ **System telemetry**: Automatic health metrics collection with device metadata, ESP32 temp, and memory usage  
✅ **Data persistence**: SPIFFS offline storage with automatic retry  
✅ **Data Engineering Ready**: Structured data with device IDs, uptime, and RSSI for ETL

## Telemetry & Monitoring

### What Gets Tracked
- **Device ID** - MAC address for device identification
- **Uptime** - Device runtime in seconds
- **RSSI** - WiFi signal strength in dBm
- **Event Type** - Data type classification ("sensor_data" or "telemetry")
- **ESP Temperature** - Internal ESP32 temperature (°C) for overheating detection
- **Memory Usage** - SPIFFS storage usage (%) to monitor data accumulation
- **Successful uploads** - number of successful data sends to Supabase
- **Failed uploads** - number of failed data transmissions
- **Total attempts** - total number of upload attempts
- **Timestamps** - auto-generated by Supabase when data is inserted

### Telemetry Behavior
- Telemetry is automatically sent to Supabase every 10 measurement cycles (~50 minutes)
- Telemetry is automatically sent to Supabase every 12 measurement cycles (~1 hour)
- **Memory Usage** - Raw memory metrics in bytes for ETL analysis:
  - `spiffs_used_bytes` - SPIFFS flash memory currently used (bytes)
  - `spiffs_total_bytes` - Total SPIFFS flash capacity (bytes)  
  - `heap_free_bytes` - Current free heap memory (bytes)
  - `heap_min_free_bytes` - Minimum free heap since startup (bytes)
- Stored in the `telemetry` table for analysis
- Metrics reset when ESP32 restarts
- View real-time stats in serial monitor: `supabase.printTelemetry()`

### Use Cases for Telemetry Data
✅ **System Health Monitoring** - detect WiFi or connectivity issues  
✅ **Data Engineering** - analyze data quality and collection reliability  
✅ **Device Fleet Management** - track multiple ESP32 devices  
✅ **Network Analysis** - monitor WiFi signal strength over time  
✅ **Alerting** - set up notifications if success_rate drops below threshold  
✅ **Historical Analysis** - track system performance over time  
✅ **SLA Tracking** - measure uptime and reliability metrics

## Data Engineering & ETL Integration

This system is designed for data engineering workflows with structured data ready for ETL processing:

### Data Flow
1. **Extract**: ESP32 collects sensor data and telemetry with precise timestamps
2. **Load**: Data is stored in Supabase with measurement timestamps
3. **Transform**: Use ETL tools to process and analyze data

### ETL Recommendations
- **Success Rate Calculation**: Compute `success_rate = successful_uploads / total_attempts` in your ETL pipeline
- **Device Monitoring**: Track `uptime` and `rssi` for device health analysis
- **Time Series Analysis**: Use `created_at` for temporal aggregations
- **Multi-Device Support**: Filter by `device_id` for fleet management

### Example ETL Queries
```sql
-- Calculate success rate per device
SELECT 
  device_id,
  AVG(successful_uploads::float / total_attempts) as avg_success_rate,
  AVG(rssi) as avg_rssi
FROM telemetry 
GROUP BY device_id;

-- Daily sensor averages
SELECT 
  DATE(created_at) as date,
  device_id,
  AVG(temperature) as avg_temp,
  AVG(humidity) as avg_humidity
FROM sensor_data 
GROUP BY DATE(created_at), device_id
ORDER BY date DESC;

-- Device health monitoring
SELECT 
  device_id,
  created_at,
  esp_temperature,
  memory_usage_percent,
    spiffs_used_bytes,
    heap_free_bytes,
  rssi
FROM telemetry 
WHERE esp_temperature > 60 OR memory_usage_percent > 80
WHERE esp_temperature > 60 OR heap_free_bytes < 50000
ORDER BY created_at DESC;
```

## Contributing

This project is optimized for data engineering applications. For enhancements, consider:
- Additional sensor types
- Battery level monitoring
- Advanced telemetry metrics
- Integration with ETL tools like Apache Airflow or dbt
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
