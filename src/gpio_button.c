#include "gpio_button.h"
#include "pico/stdlib.h"

// Button state for debouncing
typedef struct {
    uint8_t gpio;
    bool raw_state;          // Current raw reading
    bool stable_state;       // Debounced stable state
    uint32_t last_change_time;
    bool just_pressed;       // Flag for rising edge detection
} ButtonState;

static ButtonState buttons[BUTTON_COUNT];
static const uint8_t gpio_pins[BUTTON_COUNT] = {
    BUTTON_GPIO_10, BUTTON_GPIO_11, BUTTON_GPIO_12,
    BUTTON_GPIO_13, BUTTON_GPIO_14, BUTTON_GPIO_15,
    BUTTON_GPIO_16, BUTTON_GPIO_17
};

// Track when GPIO10 was pressed for simultaneous detection
static uint32_t gpio10_press_time = 0;

void gpio_button_init(void) {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        gpio_init(gpio_pins[i]);
        gpio_set_dir(gpio_pins[i], GPIO_IN);
        gpio_pull_up(gpio_pins[i]);

        buttons[i].gpio = gpio_pins[i];
        buttons[i].raw_state = false;
        buttons[i].stable_state = false;
        buttons[i].last_change_time = 0;
        buttons[i].just_pressed = false;
    }
}

// Update a single button's state with debouncing
// Returns true if button just became pressed (rising edge)
static bool update_single_button(ButtonState *btn, uint32_t now_ms) {
    bool raw_pressed = !gpio_get(btn->gpio);  // Active low

    // Clear previous just_pressed flag
    btn->just_pressed = false;

    // Check if raw state changed
    if (raw_pressed != btn->raw_state) {
        btn->raw_state = raw_pressed;
        btn->last_change_time = now_ms;
    }

    // Check if debounce period has passed
    if ((now_ms - btn->last_change_time) >= DEBOUNCE_TIME_MS) {
        bool prev_stable = btn->stable_state;
        btn->stable_state = btn->raw_state;

        // Rising edge detection (became pressed)
        if (btn->stable_state && !prev_stable) {
            btn->just_pressed = true;
            return true;
        }
    }

    return false;
}

uint8_t gpio_button_update(uint32_t now_ms) {
    uint8_t pressed_mask = 0;

    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (update_single_button(&buttons[i], now_ms)) {
            pressed_mask |= (1 << i);
        }
    }

    // Track GPIO10 press time for simultaneous detection
    if (buttons[0].just_pressed) {
        gpio10_press_time = now_ms;
    }

    return pressed_mask;
}

bool gpio_button_is_pressed(uint8_t gpio) {
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (buttons[i].gpio == gpio) {
            return buttons[i].stable_state;
        }
    }
    return false;
}

// Check for record combo (GPIO10 + slot button)
// Must call gpio_button_update() before this
int8_t gpio_button_check_record_combo(void) {
    // Check if GPIO10 is currently held
    if (!buttons[0].stable_state) {
        return -1;
    }

    // Check each slot button for just_pressed
    for (int i = 1; i <= MACRO_SLOT_COUNT; i++) {
        if (buttons[i].just_pressed) {
            return (int8_t)(i - 1);  // Return slot 0-4
        }
    }

    return -1;
}

// Check for playback trigger (slot button only, GPIO10 not pressed)
// Must call gpio_button_update() before this
int8_t gpio_button_check_playback_trigger(void) {
    // Only trigger if GPIO10 is NOT pressed
    if (buttons[0].stable_state) {
        return -1;
    }

    // Check each slot button for just_pressed
    for (int i = 1; i <= MACRO_SLOT_COUNT; i++) {
        if (buttons[i].just_pressed) {
            return (int8_t)(i - 1);  // Return slot 0-4
        }
    }

    return -1;
}

// Check for mode switch combo (GPIO16/17 + slot button)
int8_t gpio_button_check_mode_combo(bool *is_rapid) {
    // GP16 (index 6) pressed = single mode
    if (buttons[6].stable_state) {
        for (int i = 1; i <= MACRO_SLOT_COUNT; i++) {
            if (buttons[i].just_pressed) {
                *is_rapid = false;
                return (int8_t)(i - 1);
            }
        }
    }
    // GP17 (index 7) pressed = rapid mode
    if (buttons[7].stable_state) {
        for (int i = 1; i <= MACRO_SLOT_COUNT; i++) {
            if (buttons[i].just_pressed) {
                *is_rapid = true;
                return (int8_t)(i - 1);
            }
        }
    }
    return -1;
}

// Check if a slot button is currently held
bool gpio_button_is_slot_held(uint8_t slot) {
    if (slot >= MACRO_SLOT_COUNT) return false;
    return buttons[slot + 1].stable_state;  // slot 0 = buttons[1] (GPIO11)
}

// Legacy functions for compatibility
bool gpio_button_10_pressed(void) {
    return gpio_button_is_pressed(BUTTON_GPIO_10);
}

bool gpio_button_15_pressed(void) {
    return gpio_button_is_pressed(BUTTON_GPIO_15);
}
