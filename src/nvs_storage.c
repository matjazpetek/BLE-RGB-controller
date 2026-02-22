/* Standard includes */
#include <stdint.h>

/* Includes */
#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "logic.h"

static const char *TAG = "storage";
static nvs_handle_t s_nvs_handle;

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_ERROR_CHECK(nvs_open("app", NVS_READWRITE, &s_nvs_handle));
}

void storage_save_state(void)
{
    nvs_state_t s = logic_get_state();

    nvs_set_blob(s_nvs_handle, "state", &s, sizeof(s));
    nvs_commit(s_nvs_handle);

    ESP_LOGI(TAG, "State saved");
}

void storage_load_state(void)
{
    nvs_state_t s;
    size_t size = sizeof(s);

    if (nvs_get_blob(s_nvs_handle, "state", &s, &size) == ESP_OK) {
        logic_restore_state(&s);
        ESP_LOGI(TAG, "State restored");
    } else {
        ESP_LOGI(TAG, "No saved state");
    }
}
