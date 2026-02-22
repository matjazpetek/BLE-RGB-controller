/*
MCU Pin Wiring:
RED LED PWM     GPIO 10
GREEN LED PWM   GPIO 20
BLUE LED PWM    GPIO 21
Button Up       GPIO 7
Button Down     GPIO 6
Potentiometer   GPIO 1 (ADC1_CH2)

┌────────────────────────────────────────────────┐
│ app_main.c                                     │  ← Initializes drivers and start tasks (BLE, PM, Buttons)
├────────────────────────────────────────────────┤
│ logic.c                                        │  ← Takes decisions on how to drive PWM, based on input drivers
├────────────────────────────────────────────────┤
│ drivers .c (BLE, Potentiometer, Buttons, PWM)  │  ← exposes read and write functions from hardware to logic.c
│                                                │  ← BLE, Buttons, PM imput drivers; PWM LED output driver
└────────────────────────────────────────────────┘


Version summary:
09-01-2026: Version 2.0

IMPLEMENTED FEATURES:
- General Access Profile to enable connections
- Generic Attribute Profile (GATT); gatt_svc.c; RGB Controller service. Set_RGB duty cycle 0-255 characteristic. 
  Write Brightness characteristic.
- Potentiometer reading via GPIO_1, ADC1, ADC_channel_1,
- Button MODE and Button TAP_TEMPO via GPIO_7 and GPIO_6 with interrupt and debouncing
- NVS storage; nvs_storage.c; to get and set last state (RGB, brightness, mode)
- logic.c; Static, fade and pallete mode implemented. 

CHANGES:
- PWM frequency 1000 -> 800 Hz
- ble brightness characteristic cb function commented out, since no implementation in QT. 


TO DO LIST:
- nvs_storage.c; to get and set last state need to also carry MODE amd tap tempo


*/


/* Includes */
#include "esp_log.h"

/* FreeRTOS APIs */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

/* Drivers Includes */
#include "ble_platform.h"
#include "buttons.h"
#include "led.h"
#include "pm.h"
#include "logic.h"
#include "nvs_storage.h"


/* Private variables */
static const char *TAG = "main";

void app_main(void) {

    ESP_LOGI(TAG, "System boot");
    
    /* Hardware initialization */
    storage_init();
    ble_platform_init();
    pwm_init();
    adc_init();
    buttons_init();
    

    /* Start the tasks */
    xTaskCreate(
        led_effect_task,
        "led_effect_task",
        4096,
        NULL,
        5,
        &led_task_handle
    );

    /* Start the BLE Task */
    ble_platform_start();

    xTaskCreate(
        buttons_task,
        "buttons_task",
        4096,
        NULL,
        5,
        NULL
    );

    /* Start PM task */
    xTaskCreate(
        pm_task,
        "pm_task",
        2048,
        NULL,
        5,
        NULL
    );
    
    ESP_LOGI(TAG, "System initialized");
}