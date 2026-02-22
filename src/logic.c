/* Standard includes */
#include <stdint.h>

/* Includes */
#include "logic.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "esp_timer.h"

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* Drivers Includes */
#include "ble_platform.h"
#include "buttons.h"
#include "led.h"
#include "pm.h"
#include "nvs_storage.h"

/* Private function declarations */
void logic_on_potentiometer_change(uint16_t brightness_percent);
void logic_on_ble_brightness_change(uint16_t brightness_percent);
void logic_on_ble_rgb_change(uint8_t r_new, uint8_t g_new, uint8_t b_new);
void logic_on_button_mode_pressed(void);
void logic_on_button_tempo_pressed(void);

/* Private variables */
typedef enum{
    PM_SOURCE_NONE = 0,
    PM_SOURCE_POTENTIOMETER,
    PM_SOURCE_BLE
} pm_source_t;

typedef enum{
    LED_MODE_STATIC = 0,
    LED_MODE_PALETTE,
    LED_MODE_FADE
} led_mode_t;

static const char *TAG = "logic";
// Original RGB values - these are set via BLE and NEVER modified elsewhere
static uint8_t r = 255, g = 255, b = 255; // 0-255
static pm_source_t current_pm_source = PM_SOURCE_NONE;
static uint16_t current_brightness_percent = 100; // 0-100%

// Button LED Mode
static led_mode_t current_led_mode = LED_MODE_STATIC;
// Button Tempo
uint16_t switching_speeds_in_ms[2] = {20, 200};
static uint16_t current_switching_speed_in_ms = 50;
static uint8_t i = 0;

/* Pallete variables */
static uint8_t base_r, base_g, base_b;
static uint8_t dominant_channel;
static int palette_step = 0;
static int palette_dir = 1;
static const int pallete_steps = 60;

/* Fade variables */
static int fade_step = 0;
static int fade_dir = -1;
static const int fade_steps = 60;

/* Auto Dim after inactivity Variables */
#define AUTO_DIM_TIMEOUT_SEC   (2 * 60 * 60)  // 1 hour
#define AUTO_DIM_BRIGHTNESS    10             // 10%

static int64_t last_user_activity_us = 0;
static bool auto_dim_active = false;
static uint16_t saved_brightness_before_dim = 100;
static led_mode_t saved_mode_before_dim = LED_MODE_STATIC;

/* Private functions */
static void logic_user_activity(void)
{
    last_user_activity_us = esp_timer_get_time();

    if (auto_dim_active) {
        auto_dim_active = false;
        current_brightness_percent = saved_brightness_before_dim;
        current_led_mode = saved_mode_before_dim;
        logic_notify_led_update();
    }
}

void logic_on_potentiometer_change(uint16_t brightness_percent)
{
    // Check for user activity
    logic_user_activity();
    current_pm_source = PM_SOURCE_POTENTIOMETER;
    current_brightness_percent = brightness_percent;

    logic_notify_led_update();
    storage_save_state();
    
    ESP_LOGI(TAG, "Brightness set via Potentiometer: %d%%", brightness_percent);
}

void logic_on_ble_brightness_change(uint16_t brightness_percent)
{
    // Check for user activity
    logic_user_activity();
    current_pm_source = PM_SOURCE_BLE;
    current_brightness_percent = brightness_percent;

    logic_notify_led_update();
    storage_save_state();

    ESP_LOGI(TAG, "Brightness set via BLE: %d%%", brightness_percent);
}

void reinitialize_palette_and_fade(void)
{
    // Always copy from the original RGB values that were set via BLE
    base_r = r;
    base_g = g;
    base_b = b;

    if (r >= g && r >= b) dominant_channel = 0;
    else if (g >= r && g >= b) dominant_channel = 1;
    else dominant_channel = 2;

    palette_step = 0;
    palette_dir = 1;

    fade_step = 0;
    fade_dir = -1;
    
    ESP_LOGD(TAG, "Palette reinitialized: base RGB=(%d,%d,%d), dominant_channel=%d", base_r, base_g, base_b, dominant_channel);
}

void logic_on_ble_rgb_change(uint8_t r_new, uint8_t g_new, uint8_t b_new)
{   
    // Check for user activity
    logic_user_activity();
    
    ESP_LOGD(TAG, "BLE RGB change received: R=%d G=%d B=%d", r_new, g_new, b_new);
    
    // Store the original RGB values - these must NEVER be modified elsewhere
    r = r_new;
    g = g_new;
    b = b_new;
    
    ESP_LOGI(TAG, "RGB set via BLE: R=%d G=%d B=%d (stored in r,g,b)", r, g, b);
    
    // Reinitialize palette/fade with the new colors
    reinitialize_palette_and_fade();
    logic_notify_led_update();
    storage_save_state();
}

void logic_on_button_mode_pressed(void){
    // Check for user activity
    logic_user_activity();

    current_led_mode = (current_led_mode + 1) % 3;

    ESP_LOGI(TAG, "MODE Button pressed, switching to mode: %d, current RGB=(r=%d, g=%d, b=%d)", current_led_mode, r, g, b);

    if (current_led_mode == LED_MODE_FADE) {
        reinitialize_palette_and_fade();
        fade_step = 0;
        fade_dir = -1;
    }

    if (current_led_mode == LED_MODE_PALETTE) {
        reinitialize_palette_and_fade();
    }

    logic_notify_led_update();
    storage_save_state();
}
void logic_on_button_tempo_pressed(void){
    // Check for user activity
    logic_user_activity();

    i = (i + 1) % 2;
    current_switching_speed_in_ms = switching_speeds_in_ms[i];

    ESP_LOGI(TAG, "Tempo Button pressed, new switching speed: %d ms", current_switching_speed_in_ms);
    logic_notify_led_update();
}

nvs_state_t logic_get_state(void)
{
    return (nvs_state_t){
        .r = r,
        .g = g,
        .b = b,
        .brightness_percent = current_brightness_percent,
        .led_mode = current_led_mode,
        .switching_speed_in_ms = current_switching_speed_in_ms
    };
}

void logic_restore_state(const nvs_state_t *s)
{
    r = s->r;
    g = s->g;
    b = s->b;
    current_brightness_percent = s->brightness_percent;
    current_led_mode = s->led_mode;
    current_switching_speed_in_ms = s->switching_speed_in_ms;
    logic_notify_led_update();
}

// Helper function for checking auto-dim condition in the Task. IF auto-dim is active, do nothing, else dim to AUTO_DIM_BRIGHTNESS after timeout
static void logic_check_auto_dim(void)
{
    if (auto_dim_active) return;

    int64_t now = esp_timer_get_time();
    if ((now - last_user_activity_us) >= (AUTO_DIM_TIMEOUT_SEC * 1000000LL)) {

        saved_brightness_before_dim = current_brightness_percent;
        saved_mode_before_dim = current_led_mode;
        current_brightness_percent = AUTO_DIM_BRIGHTNESS;
        current_led_mode = LED_MODE_STATIC;
        auto_dim_active = true;

        ESP_LOGI(TAG, "Auto-dim activated");
        logic_notify_led_update();
    }
}

uint8_t scale_percent(uint8_t val, uint16_t pct)
{
    return (uint8_t)(((uint16_t)(val) * (pct)) / 100);
}



void logic_notify_led_update(void)
{
    if (led_task_handle) {
        xTaskNotify(led_task_handle, (1 << 0), eSetBits);
    }
}


void led_effect_task(void *pvParameters)
{
    uint32_t notify_value;
    led_task_handle = xTaskGetCurrentTaskHandle();

    storage_load_state();
    reinitialize_palette_and_fade();

    while (1) {
        TickType_t timeout;

        if (current_led_mode == LED_MODE_STATIC) {
            timeout = pdMS_TO_TICKS(1000);   // auto-dim tick
        } else {
            timeout = pdMS_TO_TICKS(current_switching_speed_in_ms);
        }

        xTaskNotifyWait(
            0,
            UINT32_MAX,
            &notify_value,
            timeout
        );

        logic_check_auto_dim();


        /* React to wake up based on mode */
        switch (current_led_mode) {

            case LED_MODE_STATIC:
                led_set_rgb_scaled(r, g, b, current_brightness_percent);
                ESP_LOGI(TAG, "R:   %d   %d   %d", r, g, b);
            break;

            case LED_MODE_PALETTE: 
                /* Get values of R G B that have been set by user */
                uint8_t out_r = base_r;
                uint8_t out_g = base_g;
                uint8_t out_b = base_b;

                /* Calculate new values for changing RGBs, based on range of movement from 5% to 100%, and based on current step in the movement */
                uint16_t pct_pallete = 5 + (palette_step * 95) / pallete_steps;

                /* set the calculated values for non-dominant leds */
                if (dominant_channel != 0) out_r = scale_percent(base_r, pct_pallete);
                if (dominant_channel != 1) out_g = scale_percent(base_g, pct_pallete);
                if (dominant_channel != 2) out_b = scale_percent(base_b, pct_pallete);

                /* Now set with new values */
                led_set_rgb_scaled(out_r, out_g, out_b, current_brightness_percent);

                /* Increase the step in proper direction for next loop*/
                palette_step += palette_dir;

                /* reverse at edges */
                if (palette_step >= pallete_steps) {
                    palette_step = pallete_steps;
                    palette_dir = -1;
                }
                if (palette_step <= 0) {
                    palette_step = 0;
                    palette_dir = +1;
                }

                /* Print to terminal*/
                ESP_LOGI(TAG, "R:   %d   %d   %d", out_r, out_g, out_b);
            break;

            case LED_MODE_FADE:
                /* Get values of R G B that have been set by user */
                out_r = base_r;
                out_g = base_g;
                out_b = base_b;

                /* Calculate new values for changing RGBs, based on range of movement from 5% to 100%, and based on current step in the movement */
                    //const uint16_t min_brightness = 10;
                    //const uint16_t max_brightness = current_brightness_percent;
                uint16_t pct_fade = 5 + (fade_step * 95) / fade_steps;

                /* set the calculated values for all leds */
                out_r = scale_percent(base_r, pct_fade);
                out_g = scale_percent(base_g, pct_fade);
                out_b = scale_percent(base_b, pct_fade);

                /* Now set with new values */
                led_set_rgb_scaled(out_r, out_g, out_b, current_brightness_percent);

                    // Calculate brightness based on fade_step (0 to fade_steps)
                    //uint16_t fade_range = (max_brightness > min_brightness) ? (max_brightness - min_brightness) : 1;
                    //uint16_t current_fade_brightness = min_brightness + (fade_range * fade_step) / fade_steps;

                    //led_set_rgb_scaled(out_r, out_g, out_b, current_fade_brightness);

                /* Increase the step in proper direction for next loop*/
                fade_step += fade_dir;

                /* reverse at edges */
                if (fade_step >= fade_steps) {
                    fade_step = fade_steps;
                    fade_dir = -1;
                }
                if (fade_step <= 0) {
                    fade_step = 0;
                    fade_dir = +1;
                }       

                /* Print to terminal*/
                ESP_LOGI(TAG, "R:   %d   %d   %d", out_r, out_g, out_b);
            break;
        }
        ESP_LOGI(TAG, "mode=%d, switch_time=%d\n", current_led_mode, current_switching_speed_in_ms);
    }
}