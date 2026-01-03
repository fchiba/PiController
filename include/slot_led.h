#ifndef SLOT_LED_H
#define SLOT_LED_H

#include <stdint.h>
#include <stdbool.h>

// Initialize slot LEDs (GPIO 19,20,21,22,26 with PWM)
void slot_led_init(void);

// Update LED patterns (call every loop iteration)
void slot_led_tick(uint32_t now_ms);

// Set recording state for a slot (overrides mode display)
void slot_led_set_recording(uint8_t slot, bool recording);

#endif
