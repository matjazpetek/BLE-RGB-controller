/* Standard includes */
#include <stdint.h>
#include <stdlib.h>

/* Includes */
#include "pm.h"
#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "logic.h"
#include "led.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Private function declarations */
uint16_t adc_read_pot(void);

/* Private variables */
static const char* TAG = "pm"; 

/* Private functions*/
static adc_oneshot_unit_handle_t adc_handle = NULL;

void adc_init(void)
{   
    // ADC oneshot unit configuration
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    
    // Set up ADC oneshot unit and check and handle error, stopping task if fails
    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc_handle);
    
    if (err != ESP_OK) {
    ESP_LOGE(TAG, "ADC init failed: %s", esp_err_to_name(err));
    vTaskDelete(NULL);   // stops current task cleanly
    }

    // ADC oneshot channel configuration
    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,  // Full range 0-3.3V
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_1, &channel_config));
}

uint16_t adc_read_pot(void)
{
    int adc_raw;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &adc_raw);
    adc_raw = 4096 - adc_raw;  // Invert reading
    if (adc_raw < 20) adc_raw = 0;
    if (adc_raw > 4050) adc_raw = 4095;
    return (uint16_t)adc_raw;
}

#define PM_SAMPLE_PERIOD_MS 50      // Read more frequently (every 50ms instead of 200ms)
#define PM_DELTA_THRESHOLD 20       // Smaller threshold = smoother changes (was 100)
#define PM_FILTER_SIZE 5            // Moving average filter to smooth noise

static uint16_t last_adc = 0;
static uint16_t filter_buffer[PM_FILTER_SIZE] = {0};
static uint8_t filter_index = 0;

// Simple moving average filter
uint16_t apply_filter(uint16_t new_sample)
{
    filter_buffer[filter_index] = new_sample;
    filter_index = (filter_index + 1) % PM_FILTER_SIZE;
    
    uint32_t sum = 0;
    for (int i = 0; i < PM_FILTER_SIZE; i++) {
        sum += filter_buffer[i];
    }
    return sum / PM_FILTER_SIZE;
}

void pm_task(void *pvParameters)
{
    while (1) {
        uint16_t raw = adc_read_pot();
        uint16_t filtered = apply_filter(raw);  // Apply smoothing filter

        if (abs(filtered - last_adc) > PM_DELTA_THRESHOLD) {
            last_adc = filtered;

            uint16_t brightness_percent = (filtered * 100) / 4095; // Scale 0-4095 to 0-100%

            logic_on_potentiometer_change(brightness_percent);
        }

        vTaskDelay(pdMS_TO_TICKS(PM_SAMPLE_PERIOD_MS));
    }
}
