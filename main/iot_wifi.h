#ifndef IOT_WIFI_H
#define IOT_WIFI_H

#include <esp_event.h>

// Function declarations
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void wifi_init_sta(void);
static void get_device_service_name(char *service_name, size_t max);
void wifi_init();

#endif // IOT_WIFI_H
