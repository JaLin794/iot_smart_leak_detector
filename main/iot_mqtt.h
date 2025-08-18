#ifndef IOT_MQTT_H 
#define IOT_MQTT_H

// MQTT configuration
#define MQTT_BROKER_URL "wss://6489701bc20a41caa3c7f6e6a6b9f9c9.s1.eu.hivemq.cloud:8884/mqtt"
#define MQTT_BROKER_USRNAME "leak_detector"
#define MQTT_BROKER_PASSWORD "leak_Detect0r"
#define MQTT_CLIENT_ID "esp32_leak_detector"
#define MQTT_TOPIC "/leak"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_init();
void mqtt_publish_leak_data(const char* data);

#endif
