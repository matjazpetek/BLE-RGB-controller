/* Standard includes */
#include <stdint.h>

/* Includes */
#include "ble_platform.h"
#include "gap.h"
#include "gatt_svc.h"
#include "esp_log.h"

/* NimBLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"


/* Library function declarations */ 
void ble_store_config_init(void);

/* Private function declarations */
static void ble_on_reset(int reason);
static void ble_on_sync(void);
static void ble_host_configure(void);
static void ble_host_task(void *param);

/* Private variables */
static const char* TAG = "ble_platform";

static void ble_on_reset(int reason)
{
    ESP_LOGW(TAG, "BLE stack reset; reason=%d", reason);
}

static void ble_on_sync(void)
{
    /* Start advertising once BLE is ready */
    adv_init();
}

static void ble_host_configure(void)
{
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_store_config_init();
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE host task started");

    nimble_port_run();

    ESP_LOGW(TAG, "BLE host task stopped");
    vTaskDelete(NULL);
}

void ble_platform_init(void)
{
    esp_err_t ret;
    int rc;

    /* Initialize NimBLE */
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nimble_port_init failed: %d", ret);
        abort();
    }

    /* Initialize standard services */
    ble_svc_gap_init();
    ble_svc_gatt_init();

    /* GAP and GATT services */
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "gap_init failed: %d", rc);
        abort();
    }

    rc = gatt_svc_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "gatt_svc_init failed: %d", rc);
        abort();
    }

    ble_host_configure();
}

void ble_platform_start(void)
{
    xTaskCreate(
        ble_host_task,
        "ble_host",
        4 * 1024,
        NULL,
        5,
        NULL
    );
}

void ble_platform_stop(void)
{
    nimble_port_stop();
    nimble_port_deinit();
}


