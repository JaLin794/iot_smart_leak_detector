# leak_detector
IoT Device to Detect Toilet Leaks

# ESP32 Leak Detector with WiFi and MQTT

This project implements a leak detection system using an ESP32 microcontroller that connects to WiFi and publishes sensor data to an MQTT broker.

## Features

- WiFi connectivity with automatic reconnection
- MQTT client for publishing leak detection data
- Liquid level sensor integration
- JSON-formatted data publishing
- Configurable sensor reading intervals

## Configuration

### WiFi Settings
Edit `main/iot_wifi.h` to configure your WiFi credentials:
```c
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
```

### MQTT Settings
Edit `main/iot_wifi.h` to configure your MQTT broker:
```c
#define MQTT_BROKER_URL "your_mqtt_broker_ip"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "esp32_leak_detector"
#define MQTT_TOPIC "leak/detection"
```

## Data Format

The ESP32 publishes leak detection data in JSON format to the MQTT topic:
```json
{
  "sensor_value": 2048,
  "voltage": 2.5,
  "timestamp": 1234567890
}
```

## Building and Flashing

1. Set up your ESP-IDF environment
2. Navigate to the project directory
3. Build the project:
   ```bash
   idf.py build
   ```
4. Flash to your ESP32:
   ```bash
   idf.py flash monitor
   ```

## Usage

1. Power on the ESP32
2. The device will automatically connect to WiFi
3. Once connected, it will start publishing sensor data every 5 seconds
4. Monitor the serial output for connection status and published data

## Dependencies

- ESP-IDF v4.4 or later
- WiFi network access
- MQTT broker (e.g., Mosquitto, AWS IoT, etc.)

## Troubleshooting

- Check WiFi credentials in `iot_wifi.h`
- Verify MQTT broker is accessible from your network
- Monitor serial output for connection errors
- Ensure your ESP32 has proper power supply
