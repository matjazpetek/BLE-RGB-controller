/* Standard includes */
#include <stdint.h>

/* Includes */
#include "gatt_svc.h"
#include "led.h"
#include "esp_log.h"
#include "logic.h"

/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

/* Private variables */
static const char* TAG = "gatt_svc";


/* Private function declarations */
static int set_rgb_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
static int set_rgb_chr_dsc_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg);
static int set_brightness_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Set service */
static const ble_uuid128_t set_svc_uuid = 
    BLE_UUID128_INIT(0x7a, 0x82, 0xf6, 0x02, 0x3f, 0x98, 0x41, 0xbe, 0xee, 0x43, 0x5b, 0x61, 0x8d, 0x86, 0xdb, 0x9a);
    
/* Set RGB characteristic */
static const ble_uuid128_t set_rgb_chr_uuid =
    BLE_UUID128_INIT(0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x02);
static uint16_t set_rgb_chr_val_handle;

/* Set Brightness characteristic */
static const ble_uuid128_t set_brightness_chr_uuid =
    BLE_UUID128_INIT(0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x03);
static uint16_t set_brightness_chr_val_handle;

/* Descriptors */
static const struct ble_gatt_dsc_def set_rgb_chr_dsc[] = {
    {
        .uuid = BLE_UUID16_DECLARE(0x2901),  // Characteristic User Description
        .att_flags = BLE_ATT_F_READ,
        .access_cb = set_rgb_chr_dsc_access,     // Use the explicit callback
    },
    {0}
};

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* RGB Controller service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &set_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            /* Set RGB characteristic */
            {
                .uuid = &set_rgb_chr_uuid.u,
                .access_cb = set_rgb_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &set_rgb_chr_val_handle,
                .descriptors = set_rgb_chr_dsc
            },
            /* Set Brightness characteristic */
            {
                .uuid = &set_brightness_chr_uuid.u,
                .access_cb = set_brightness_chr_access,
                .flags = BLE_GATT_CHR_F_WRITE,
                .val_handle = &set_brightness_chr_val_handle,
                .descriptors = set_rgb_chr_dsc
            },
            {0} /* No more characteristics in this service. */
        }
    },
    {0} /* No more services. */
};

/* Private functions */
/* Set RGB characteristic access callback */
static int set_rgb_chr_access(uint16_t conn_handle, 
                              uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, 
                              void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (attr_handle != set_rgb_chr_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (ctxt->om->om_len != 3) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint8_t r = ctxt->om->om_data[0];
    uint8_t g = ctxt->om->om_data[1];
    uint8_t b = ctxt->om->om_data[2];

    logic_on_ble_rgb_change(r, g, b);

    ESP_LOGI(TAG, "BLE Service RGB write: R=%d G=%d B=%d", r, g, b);

    return 0;
}

/* Set RGB characteristic descriptor access callback */
static int set_rgb_chr_dsc_access(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char set_rgb_chr_name[] = "Set RGB LED Color";

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            // Send the descriptor value to the phone
            return os_mbuf_append(ctxt->om, set_rgb_chr_name, sizeof(set_rgb_chr_name) - 1);

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
}

/* Set Brightness characteristic access callback */
static int set_brightness_chr_access(uint16_t conn_handle, 
                                     uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt, 
                                     void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (attr_handle != set_brightness_chr_val_handle) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    if (ctxt->om->om_len != 1) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    uint8_t brightness = ctxt->om->om_data[0];

    if( brightness > 100 ){
        brightness = 100;
    }else if( brightness < 0 ){
        brightness = 0;
    }

    logic_on_ble_brightness_change(brightness);

    ESP_LOGI(TAG, "BLE Service Brightness write: %d", brightness);
    return 0;
}


/*
 *  Handle GATT attribute register events
 *      - Service register event
 *      - Characteristic register event
 *      - Descriptor register event
 */
void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    /* Local variables */
    char buf[BLE_UUID_STR_LEN];

    /* Handle GATT attributes register events */
    switch (ctxt->op) {

    /* Service register event */
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGD(TAG, "registered service %s with handle=%d",
                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                 ctxt->svc.handle);
        break;

    /* Characteristic register event */
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGD(TAG,
                 "registering characteristic %s with "
                 "def_handle=%d val_handle=%d",
                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                 ctxt->chr.def_handle, ctxt->chr.val_handle);
        break;

    /* Descriptor register event */
    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGD(TAG, "registering descriptor %s with handle=%d",
                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                 ctxt->dsc.handle);
        break;

    /* Unknown event */
    default:
        assert(0);
        break;
    }
}

void gatt_svr_subscribe_cb(struct ble_gap_event *event) {}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}
