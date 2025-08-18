idf_component_register(SRCS "main.c" "iot_wifi.c" "leak_sensor.c" "iot_mqtt.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_event nvs_flash mqtt esp_adc wifi_provisioning)
