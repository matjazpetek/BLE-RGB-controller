/* Standard includes */
#include <stdint.h>

/* Includes */
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "logic.h"

/* FreeRTOS */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


/* Private variables */
static const char* TAG = "Buttons.c";

static const gpio_num_t BUTTON_MODE = GPIO_NUM_7;   // Button 1: MODE
static const gpio_num_t BUTTON_TAP_TEMPO = GPIO_NUM_6; // Button 2: TAP_TEMPO

static uint32_t last_press_time_mode = 0;
static uint32_t last_press_time_tempo = 0;
static const uint16_t DEBOUNCE_MS = 300;

static TaskHandle_t buttons_task_handle = NULL;

/* ---------- ISR ---------- */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    BaseType_t higher_prio_woken = pdFALSE;

    // Debug: Check if task handle is valid
    if (buttons_task_handle == NULL) {
        return;  // Task not ready yet, ignore interrupt
    }

    xTaskNotifyFromISR(
        buttons_task_handle,
        (1 << gpio_num),
        eSetBits,
        &higher_prio_woken
    );

    if (higher_prio_woken) {
        portYIELD_FROM_ISR();
    }
}

/* ---------- Init ---------- */
void buttons_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    io_conf.pin_bit_mask = (1ULL << BUTTON_MODE);
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << BUTTON_TAP_TEMPO);
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    // NOTE: ISR handlers are registered in buttons_task after task handle is set
    // This prevents crashes from interrupts firing before the task exists

    ESP_LOGI(TAG, "Buttons initialized");
}

/* ---------- Task ---------- */
void buttons_task(void *pvParameters)
{
    uint32_t notify_bits;
    uint32_t current_time_ms;

    // Get and store the task handle
    buttons_task_handle = xTaskGetCurrentTaskHandle();

    // NOW it's safe to register interrupt handlers (handle is valid)
    gpio_isr_handler_add(BUTTON_MODE, gpio_isr_handler, (void *)BUTTON_MODE);
    gpio_isr_handler_add(BUTTON_TAP_TEMPO, gpio_isr_handler, (void *)BUTTON_TAP_TEMPO);

    ESP_LOGI(TAG, "Button ISR handlers registered");

    while (1) {
        // Sleep until a button interrupt arrives
        // Task wakes up when xTaskNotifyFromISR is called
        xTaskNotifyWait(
            0x00,              // Don't clear any bits on entry
            UINT32_MAX,        // Clear all bits on exit
            &notify_bits,      // Store which pin triggered (bit position = GPIO number)
            portMAX_DELAY      // Wait indefinitely for interrupt
        );

        current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);

        // Check if MODE button (GPIO 7) triggered the interrupt
        if (notify_bits & (1ULL << BUTTON_MODE)) {
            //ESP_LOGI(TAG, "MODE button interrupt detected");
            // Debounce: only process if DEBOUNCE_MS has passed since last press
            if ((current_time_ms - last_press_time_mode) >= DEBOUNCE_MS) {
                //ESP_LOGI(TAG, "MODE button passed debounce, calling handler");
                last_press_time_mode = current_time_ms;
                logic_on_button_mode_pressed();
                //ESP_LOGI(TAG, "Button MODE pressed");
            }
        }

        // Check if TAP_TEMPO button (GPIO 6) triggered the interrupt
        if (notify_bits & (1ULL << BUTTON_TAP_TEMPO)) {
            //ESP_LOGI(TAG, "TAP_TEMPO button interrupt detected");
            // Debounce: only process if DEBOUNCE_MS has passed since last press
            if ((current_time_ms - last_press_time_tempo) >= DEBOUNCE_MS) {
                //ESP_LOGI(TAG, "TAP_TEMPO button passed debounce, calling handler");
                last_press_time_tempo = current_time_ms;
                logic_on_button_tempo_pressed();
                //ESP_LOGI(TAG, "Button TAP_TEMPO pressed");
            }
        }
    }
}