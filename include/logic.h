#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* exposed functions */
void logic_on_potentiometer_change(uint16_t brightness_percent);
void logic_on_ble_brightness_change(uint16_t brightness_percent);
void logic_on_ble_rgb_change(uint8_t r, uint8_t g, uint8_t b);
void logic_on_button_mode_pressed(void);
void logic_on_button_tempo_pressed(void);

/* LED effect task */
static TaskHandle_t led_task_handle = NULL;
void led_effect_task(void *pvParameters);

void logic_notify_led_update(void);

/* NVS storage structure and exposed funcitons */
typedef struct {
    uint8_t r, g, b;
    uint16_t brightness_percent;
    uint8_t led_mode;
    uint16_t switching_speed_in_ms;
} nvs_state_t;

nvs_state_t logic_get_state(void);
void logic_restore_state(const nvs_state_t *s);
