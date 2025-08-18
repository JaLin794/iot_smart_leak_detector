#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

#define CONFIG_EXAMPLE_PROV_MGR_CONNECTION_CNT 5

static const char *TAG = "IOT_WIFI";

/* Global variable to store the received SSID */
static char received_ssid[33] = {0}; /* WiFi SSID can be up to 32 characters + null terminator */
static bool ssid_received = false;

// Event group to signal when we are connected
static EventGroupHandle_t s_wifi_event_group;

// Event bit definitions
const int WIFI_CONNECTED_EVENT = BIT0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials" 
                              "\n\tSSID     : %s\n\tPassword : %s",
                                (const char *) wifi_sta_cfg->ssid,
                                (const char *) wifi_sta_cfg->password);

                /* Store the SSID for later use */
                strncpy(received_ssid, (const char *)wifi_sta_cfg->ssid, sizeof(received_ssid) - 1);
                received_ssid[sizeof(received_ssid) - 1] = '\0'; /* Ensure null termination */
                ssid_received = true;
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                              "\n\tPlease reset to factory and retry provisioning",
                                (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                              "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                wifi_prov_mgr_reset_sm_state_on_failure();
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                esp_err_t err = wifi_prov_mgr_deinit();
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to de-initialize provisioning manager: %s", esp_err_to_name(err));
                }
                break;
            default:
                break;
        }
    }
    else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "SoftAP transport: Connected!");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "SoftAP transport: Disconnected!");
                break;
            default:
                break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
            case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
                ESP_LOGI(TAG, "Secured session established!");
                break;
            case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
                ESP_LOGE(TAG, "Received invalid security parameters for establishing secure session!");
                break;
            case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
                ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing secure session!");
                break;
            default:
                break;
        }
    }
}

static void wifi_init_sta(void)
{
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}


void wifi_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        .wifi_prov_conn_cfg = {
            .wifi_conn_attempts =  CONFIG_EXAMPLE_PROV_MGR_CONNECTION_CNT,
         },
         .scheme = wifi_prov_scheme_softap,
         .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
        };

        /* Initialize provisioning manager with the
         * configuration parameters set above */
        ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
    
        bool provisioned = false;

        ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

        if (!provisioned) {
            ESP_LOGI(TAG, "Starting provisioning");
    
            /* What is the Device Service Name that we want
             * This translates to :
             *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
             *     - device name when scheme is wifi_prov_scheme_ble
             */
            char service_name[12];
            get_device_service_name(service_name, sizeof(service_name));

            /*      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
            *          using X25519 key exchange and proof of possession (pop) and AES-CTR
            *          for encryption/decryption of messages.
            */
            wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

            /* Do we want a proof-of-possession (ignored if Security 0 is selected):
            *      - this should be a string with length > 0
            *      - NULL if not used
            */
            const char *pop = "abcd1234";

            /* This is the structure for passing security parameters
            * for the protocomm security 1.
            */
            wifi_prov_security1_params_t *sec_params = pop;

            const char *username  = NULL;

            /* What is the service key (could be NULL)
            * This translates to :
            *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
            *          (Minimum expected length: 8, maximum 64 for WPA2-PSK)
            *     - simply ignored when scheme is wifi_prov_scheme_ble
            */
            const char *service_key = NULL;

            /* An optional endpoint that applications can create if they expect to
            * get some additional custom data during provisioning workflow.
            * The endpoint name can be anything of your choice.
            * This call must be made before starting the provisioning.
            */
            
            wifi_prov_mgr_disable_auto_stop(1000);
            /* Start provisioning service */
            ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, service_name, service_key));

        }
        else {
            ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
    
            /* We don't need the manager as device is already provisioned,
                * so let's release it's resources */
            ESP_ERROR_CHECK(wifi_prov_mgr_deinit());
    
            ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
            /* Start Wi-Fi station */
            wifi_init_sta();
        }
        
            /* Wait for Wi-Fi connection */
            xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_EVENT, true, true, portMAX_DELAY);
}