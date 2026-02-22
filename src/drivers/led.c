//#pragma message "PWM.C IS BEING COMPILED"
/* Standard includes */
#include <stdint.h>

/* Includes */
#include "led.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


static const char* TAG = "led";

static const gpio_num_t PWM_RED_PIN = GPIO_NUM_10;   // R_transistor_pin
static const gpio_num_t PWM_GREEN_PIN = GPIO_NUM_20; // G_transistor_pin
static const gpio_num_t PWM_BLUE_PIN = GPIO_NUM_21;  // B_transistor_pin


void pwm_init(void)
{
    // Configure Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_8_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 1024,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    // Calling the timer configuration function and passing my timer configuration
    ledc_timer_config(&ledc_timer);

    // Configure LEDC Channel for pin 0 (R_transistor_pin)
    ledc_channel_config_t configure_ch_0 = {
        .gpio_num = PWM_RED_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {.output_invert = 0}
    };
    // Apply the configuration
    ledc_channel_config(&configure_ch_0);  

    // Configure LEDC Channel for pin 1 (G_transistor_pin)
    ledc_channel_config_t configure_ch_1 = {
        .gpio_num = PWM_GREEN_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {.output_invert = 0}
    };
    // Apply the configuration
    ledc_channel_config(&configure_ch_1);  

    // Configure LEDC Channel for pin 2 (B_transistor_pin)
    ledc_channel_config_t configure_ch_2 = {
        .gpio_num = PWM_BLUE_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_2,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {.output_invert = 0}
    };
    // Apply the configuration
    ledc_channel_config(&configure_ch_2);  

    //ESP_LOGI(TAG, "PWM RGB initialized");
}

void led_set_rgb_scaled(uint16_t r, uint16_t g, uint16_t b, uint16_t brightness_percent)
{
    r = (r * brightness_percent) / 100;
    g = (g * brightness_percent) / 100;
    b = (b * brightness_percent) / 100;

    set_duty_cycle_to_pwm(r, g, b);
}

void set_duty_cycle_to_pwm(uint8_t r, uint8_t g, uint8_t b){

    // Set duty for Red channel 
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // Set duty for Green channel
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    // Set duty for Blue channel
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

    //ESP_LOGI(TAG, "Set PWM R:%d G:%d B:%d", r, g, b);
}

