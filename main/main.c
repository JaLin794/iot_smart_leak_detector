#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "iot_wifi.h"
#include "iot_mqtt.h"
#include "leak_sensor.h"

static const char *TAG = "MAIN";

// Task handles
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t mqtt_task_handle = NULL;

// Shared data structure for sensor readings
typedef struct {
    float sensor_value;      // For internal tracking/logging
    float voltage;           // For MQTT publishing
    bool leak_detected;      // For MQTT publishing
    uint64_t timestamp;      // For internal tracking/logging
} sensor_data_t;

// Global variables
static sensor_data_t current_sensor_data = {0};
static SemaphoreHandle_t sensor_data_mutex = NULL;

// Task function declarations
static void sensor_monitoring_task(void *pvParameters);
static void mqtt_publishing_task(void *pvParameters);

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Toilet Leak Detector...");
    
    // Create mutex for protecting shared sensor data
    sensor_data_mutex = xSemaphoreCreateMutex();
    if (sensor_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return;
    }
    
    // Initialize liquid level sensor
    liquid_level_sensor_init();
    ESP_LOGI(TAG, "Liquid level sensor initialized");
    
    // Initialize WiFi
    wifi_init();
    ESP_LOGI(TAG, "WiFi initialization started");
    
    // Initialize MQTT
    mqtt_init();
    ESP_LOGI(TAG, "MQTT initialization started");
    
    // Create sensor monitoring task
    xTaskCreate(
        sensor_monitoring_task,      // Task function
        "sensor_monitor",           // Task name
        4096,                       // Stack size (bytes)
        NULL,                       // Task parameters
        5,                          // Task priority
        NULL         // Task handle
    );
    
    // Create MQTT publishing task
    xTaskCreate(
        mqtt_publishing_task,       // Task function
        "mqtt_publisher",          // Task name
        4096,                       // Stack size (bytes)
        NULL,                       // Task parameters
        4,                          // Task priority
        NULL           // Task handle
    );
    
    ESP_LOGI(TAG, "All tasks created successfully");
}

// Task to monitor liquid level sensor
static void sensor_monitoring_task(void *pvParameters)
{
    const float LEAK_THRESHOLD_VOLTAGE = 1.60;  // Voltage threshold for leak detection
    const int SENSOR_CHECK_INTERVAL_MS = 1500; // Check every 1.5 seconds
    
    ESP_LOGI(TAG, "Sensor monitoring task started");
    
    while (1) {
        // Read sensor value
        float sensor_value = liquid_level_sensor_read();
        float voltage = liquid_level_sensor_voltage(sensor_value);
        bool leak_detected = (voltage > LEAK_THRESHOLD_VOLTAGE);
        
        ESP_LOGD(TAG, "Sensor value: %d, Voltage: %.2fV", sensor_value, voltage);
        
        // Update shared data with mutex protection
        if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            current_sensor_data.sensor_value = sensor_value;
            current_sensor_data.voltage = voltage;
            current_sensor_data.timestamp = esp_timer_get_time() / 1000000; // Convert to seconds
            
            // Check for leak state change
            if (leak_detected && !current_sensor_data.leak_detected) {
                current_sensor_data.leak_detected = true;
                ESP_LOGW(TAG, "LEAK DETECTED! Sensor value: %.2f, Voltage: %.2fV, Time: %llds", 
                        sensor_value, voltage, current_sensor_data.timestamp);
            } else if (!leak_detected && current_sensor_data.leak_detected) {
                current_sensor_data.leak_detected = false;
                ESP_LOGI(TAG, "Leak condition cleared. Sensor value: %.2f, Time: %llds", 
                        sensor_value, current_sensor_data.timestamp);
            }
            
            xSemaphoreGive(sensor_data_mutex);
        }
        
        // Wait before next reading
        vTaskDelay(pdMS_TO_TICKS(SENSOR_CHECK_INTERVAL_MS));
    }
}

// Task to publish MQTT data
static void mqtt_publishing_task(void *pvParameters)
{
    const int MQTT_PUBLISH_INTERVAL_MS = 5000; // Publish every 5 seconds
    sensor_data_t data_to_publish;
    char mqtt_message[128];
    
    ESP_LOGI(TAG, "MQTT publishing task started");
    
    while (1) {
        // Get current sensor data with mutex protection
        if (xSemaphoreTake(sensor_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            data_to_publish = current_sensor_data; // Copy the data
            xSemaphoreGive(sensor_data_mutex);
            
            // Debug log with all internal data
            ESP_LOGD(TAG, "Internal data - Sensor: %.2f, Voltage: %.2fV, Leak: %s, Time: %llds", 
                    data_to_publish.sensor_value, data_to_publish.voltage, 
                    data_to_publish.leak_detected ? "true" : "false", data_to_publish.timestamp);
            
            // Create JSON message with only voltage and leak_detected for MQTT
            snprintf(mqtt_message, sizeof(mqtt_message),
                    "{\"voltage\":%.2f,\"leak_detected\":%s}",
                    data_to_publish.voltage,
                    data_to_publish.leak_detected ? "true" : "false");
            
            // Publish to MQTT
            mqtt_publish_leak_data(mqtt_message);
            ESP_LOGI(TAG, "Published to MQTT: %s", mqtt_message);
        } else {
            ESP_LOGW(TAG, "Failed to acquire mutex for MQTT publishing");
        }
        
        // Wait before next publish
        vTaskDelay(pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL_MS));
    }
}
