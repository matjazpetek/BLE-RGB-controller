#pragma once
#include <stdint.h>

void pwm_init(void);
void led_set_rgb_scaled(uint16_t r, uint16_t g, uint16_t b, uint16_t brightness_percent);
void set_duty_cycle_to_pwm(uint8_t r, uint8_t g, uint8_t b);