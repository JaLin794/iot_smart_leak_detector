#include "leak_sensor.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#include "mqtt_client.h"

static const char *TAG = "LEAK_SENSOR";

// ADC configuration
#define ADC_UNIT ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_7  // GPIO35
#define ADC_ATTEN ADC_ATTEN_DB_12 // DB_11 = 0-3.3V Range
#define ADC_BITWIDTH ADC_BITWIDTH_12 // 12-bit resolution (range is 9-bit resolution - 12-bit resolution)

// ADC handle
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;

// Calibration parameters
static bool do_calibration1 = false;

void liquid_level_sensor_init(void)
{
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    //-------------ADC1 Calibration Init---------------//
    if (ADC_CALI_SCHEME_VER_LINE_FITTING) {
        ESP_LOGI(TAG, "Using Line Fitting Calibration");
        adc_cali_handle_t handle = NULL;
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH,
        };
        if (adc_cali_create_scheme_line_fitting(&cali_config, &handle) == ESP_OK) {
            adc1_cali_handle = handle;
            do_calibration1 = true;
        }
    }

    ESP_LOGI(TAG, "Liquid level sensor initialized");
}

int liquid_level_sensor_read(void)
{
    int adc_raw = 0;
    int voltage = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));
    
    if (do_calibration1) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &voltage));
    }
    
    ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d, Voltage: %dmV", ADC_UNIT, ADC_CHANNEL, adc_raw, voltage);
    
    return adc_raw;
}

float liquid_level_sensor_voltage(int sensor_value)
{
    float voltage = 0.0;
    
    if (do_calibration1) {
        int voltage_mv = 0;
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, sensor_value, &voltage_mv));
        voltage = (float)voltage_mv / 1000.0; // Convert mV to V
    } else {
        // If calibration is not available, use a simple conversion
        // Assuming 3.3V reference and 12-bit ADC (4095 max value)
        voltage = (float)sensor_value * 3.3 / 4095.0;
    }
    
    return voltage;
}
