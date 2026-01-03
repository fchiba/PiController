#include "slot_led.h"
#include "macro.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define SLOT_LED_COUNT 5

static const uint8_t slot_gpio[SLOT_LED_COUNT] = {28, 27, 26, 21, 18};
static bool slot_recording[SLOT_LED_COUNT] = {false};
static uint32_t recording_start_time[SLOT_LED_COUNT] = {0};

void slot_led_init(void) {
    for (int i = 0; i < SLOT_LED_COUNT; i++) {
        gpio_set_function(slot_gpio[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(slot_gpio[i]);
        pwm_set_wrap(slice, 255);  // 8-bit resolution
        pwm_set_enabled(slice, true);
        pwm_set_gpio_level(slot_gpio[i], 0);  // Start off
    }
}

void slot_led_tick(uint32_t now_ms) {
    for (int i = 0; i < SLOT_LED_COUNT; i++) {
        uint16_t brightness = 0;

        if (slot_recording[i]) {
            // Recording: slow fade (triangle wave, 2 second cycle)
            uint32_t elapsed = now_ms - recording_start_time[i];
            uint32_t phase = elapsed % 2000;
            if (phase < 1000) {
                brightness = (phase * 255) / 1000;  // Fade in
            } else {
                brightness = ((2000 - phase) * 255) / 1000;  // Fade out
            }
        } else {
            SlotMode mode = macro_get_slot_mode(i);
            switch (mode) {
                case SLOT_MODE_SINGLE:
                    brightness = 0;  // Off
                    break;
                case SLOT_MODE_RAPID:
                    brightness = 255;  // Full on
                    break;
                case SLOT_MODE_CONTINUOUS:
                    // Fast blink (100ms on/off)
                    brightness = ((now_ms / 100) % 2) ? 255 : 0;
                    break;
            }
        }

        pwm_set_gpio_level(slot_gpio[i], brightness);
    }
}

void slot_led_set_recording(uint8_t slot, bool recording) {
    if (slot >= SLOT_LED_COUNT) return;
    slot_recording[slot] = recording;
    if (recording) {
        recording_start_time[slot] = to_ms_since_boot(get_absolute_time());
    }
}
