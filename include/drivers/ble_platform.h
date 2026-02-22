#pragma once

/* Initialize BLE stack and services */
void ble_platform_init(void);

/* Start BLE operation (tasks, advertising) */
void ble_platform_start(void);

/* Optional future hooks */
void ble_platform_stop(void);

