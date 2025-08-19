# ESP32 Leak Detector with WiFi and MQTT

This project implements a leak detection system using an ESP32 microcontroller that connects to WiFi via WiFi Provisioning and publishes sensor data to an MQTT broker (HiveMQ Cloud)

# Project Purpose
The purpose of this project is to learn and create a working code for a device that connects to wifi by collecting the credentials via wifi provisioning and connecting to a MQTT cloud. The device then takes a measurement from a sensor and publishes the values to the MQTT Broker.

The context of this device is a IoT device that detects toilet leaks via a liquid level sensor. 

# Learning Lessons and Improvements
The device currently does not work well as it has not yet been fine tune/calibrated. The threshold has not been well selected as the primary objective of this project is to develop a device accomplishes WiFi Provisioning, connecting to WiFi, connecting to a MQTT cloud, and publishing data to the MQTT cloud. 

A liquid level sensor did not function very accurately via cyclic tests of dipping and removing the sensor from a body of water. The surface material of the sensor seems to be very hydrophobic and does not perform reliably. 

To increase the battery life of the device, implementation of deep sleep can be used to reduce the current draw of the overall device. The logic of this is to collect data samples every few minutes, perform leak detection logic, connect to wifi and upload to MQTT cloud only if leak detection true. To keep the user well aware of the state of the device, periodic wifi connection and uploading to MQTT for status updates can be done. 

# Docker

Docker image for this project can be found in the Docker Hub: sortdrop/smart_lead_detector
Link: https://hub.docker.com/r/sortdrop/smart_leak_detector

Docker Command to run docker image
docker run -it \
--device=/dev/ttyUSB0 \
--privileged \
sortdrop/smart_leak_detector:1.0.0 /bin/bash

# IoT Device
## Features

- WiFi connectivity with automatic reconnection
- MQTT client for publishing leak detection data
- Liquid level sensor integration
- JSON-formatted data publishing
- Configurable sensor reading intervals

## Configuration

### WiFi Settings
Uses WiFi Provisioning to connect to wifi. Done via ESP SoftAP Prov App

### MQTT Settings
Edit `main/iot_wifi.h` to configure your MQTT broker:
```c
#define MQTT_BROKER_URL "your_mqtt_broker_ip"
#define MQTT_BROKER_USRNAME "your_broker_username"
#define MQTT_BROKER_PASSWORD "your_broker_password"
#define MQTT_CLIENT_ID "your_broker_client_id"
#define MQTT_TOPIC "/your_broker_topic"
```
The current values are:
```c
#define MQTT_BROKER_URL "wss://6489701bc20a41caa3c7f6e6a6b9f9c9.s1.eu.hivemq.cloud:8884/mqtt"
#define MQTT_BROKER_USRNAME "leak_detector"
#define MQTT_BROKER_PASSWORD "leak_Detect0r"
#define MQTT_CLIENT_ID "esp32_leak_detector"
#define MQTT_TOPIC "/leak"
```

## Data Format

The ESP32 publishes leak detection data in JSON format to the MQTT topic:
```json
{
  "voltage": 1750mV
  "full_tank": true
  "leak_detected": false
  "flush_detected": false
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
2. Download the ESP SoftAP Prov App (for Apple Devices)
3. Connect to the ESP32 Hotspot Wifi
4. Input your 2.4 GHz WiFi credentials (WiFi SSID and Password)
5. Once connected, it will start publishing sensor data every 5 seconds
6. Monitor the serial output for connection status and published data

Note: If previously connected, the ESP32 will automatically connect to wifi. Upon 5 failed attempts, the ESP32 will need to be re-provisioned. 

GPIO35 is used for this project.

## Dependencies

- ESP-IDF v4.4 or later
- WiFi network access
- MQTT broker (e.g., Mosquitto, AWS IoT, etc.)

## Troubleshooting

- Check WiFi credentials in `iot_wifi.h`
- Verify MQTT broker is accessible from your network
- Monitor serial output for connection errors
- Ensure your ESP32 has proper power supply

## Project Creation

- This project was made sucessful by using and modifying the following ESP32 example code:

* provisioning -> wifi_prov_mgr
* protocols -> mqtt -> wss

In Menuconfig, the following are turned on/modified:

Component config
* ESP-TLS
   - Allow potentially insecure options
      Skip server certificate verification by defautl (WARNING: ONLY FOR TESTING PURPOSE, READ HELP)
* Hardware Settings
   Main XTAL Config
      Main XTAL frequency (26 MHz)

Serial flash config
* Flash size (4 MB)




