#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// LED pattern states
typedef enum {
    LED_PATTERN_IDLE,           // Follow controller connection state
    LED_PATTERN_RECORDING,      // Fast blink (200ms on/200ms off)
    LED_PATTERN_PLAYING,        // Medium blink (500ms on/500ms off)
    LED_PATTERN_SAVE_SUCCESS,   // 3 quick flashes
    LED_PATTERN_SAVE_ERROR      // Long solid then return
} LedPattern;

static LedPattern current_pattern = LED_PATTERN_IDLE;
static uint32_t pattern_start_time = 0;
static bool controller_connected = false;
static bool recording_active = false;
static bool playing_active = false;

void macro_led_init(void) {
    current_pattern = LED_PATTERN_IDLE;
    pattern_start_time = 0;
    controller_connected = false;
    recording_active = false;
    playing_active = false;
}

void macro_led_set_controller_connected(bool connected) {
    controller_connected = connected;
}

void macro_led_set_recording(bool recording) {
    recording_active = recording;
    if (recording) {
        current_pattern = LED_PATTERN_RECORDING;
        pattern_start_time = to_ms_since_boot(get_absolute_time());
    } else if (!playing_active) {
        current_pattern = LED_PATTERN_IDLE;
    }
}

void macro_led_set_playing(bool playing) {
    playing_active = playing;
    if (playing) {
        current_pattern = LED_PATTERN_PLAYING;
        pattern_start_time = to_ms_since_boot(get_absolute_time());
    } else if (!recording_active) {
        current_pattern = LED_PATTERN_IDLE;
    }
}

void macro_led_flash_save_result(bool success) {
    current_pattern = success ? LED_PATTERN_SAVE_SUCCESS : LED_PATTERN_SAVE_ERROR;
    pattern_start_time = to_ms_since_boot(get_absolute_time());
}

void macro_led_tick(void) {
    uint32_t now = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed = now - pattern_start_time;
    bool led_on = false;

    switch (current_pattern) {
        case LED_PATTERN_IDLE:
            // Follow controller connection state
            led_on = controller_connected;
            break;

        case LED_PATTERN_RECORDING:
            // Fast blink: 200ms on, 200ms off (2.5Hz)
            led_on = ((elapsed / 200) % 2) == 0;
            break;

        case LED_PATTERN_PLAYING:
            // Medium blink: 500ms on, 500ms off (1Hz)
            led_on = ((elapsed / 500) % 2) == 0;
            break;

        case LED_PATTERN_SAVE_SUCCESS:
            // 3 quick flashes (100ms on, 100ms off each)
            if (elapsed < 600) {
                led_on = ((elapsed / 100) % 2) == 0;
            } else {
                // Return to appropriate state
                if (recording_active) {
                    current_pattern = LED_PATTERN_RECORDING;
                    pattern_start_time = now;
                } else if (playing_active) {
                    current_pattern = LED_PATTERN_PLAYING;
                    pattern_start_time = now;
                } else {
                    current_pattern = LED_PATTERN_IDLE;
                }
                led_on = controller_connected;
            }
            break;

        case LED_PATTERN_SAVE_ERROR:
            // Long solid (1 second), then return
            if (elapsed < 1000) {
                led_on = true;
            } else {
                // Return to appropriate state
                if (recording_active) {
                    current_pattern = LED_PATTERN_RECORDING;
                    pattern_start_time = now;
                } else if (playing_active) {
                    current_pattern = LED_PATTERN_PLAYING;
                    pattern_start_time = now;
                } else {
                    current_pattern = LED_PATTERN_IDLE;
                }
                led_on = controller_connected;
            }
            break;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on ? 1 : 0);
}
