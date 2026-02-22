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
- Write Brightness characteristic.
- Potentiometer reading via GPIO_1, ADC1, ADC_channel_1,
- Button MODE and Button TAP_TEMPO via GPIO_7 and GPIO_6 with interrupt and debouncing
- NVS storage; nvs_storage.c; to get and set last state (RGB, brightness, mode)
- logic.c; Static, fade and pallete mode implemented. 
- automatic dimming after inactivity implemented.
