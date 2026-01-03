#ifndef GPIO_BUTTON_H
#define GPIO_BUTTON_H

#include <stdbool.h>
#include <stdint.h>

// GPIO pin definitions
#define BUTTON_GPIO_10  10   // Record modifier button
#define BUTTON_GPIO_11  11   // Macro slot 0
#define BUTTON_GPIO_12  12   // Macro slot 1
#define BUTTON_GPIO_13  13   // Macro slot 2
#define BUTTON_GPIO_14  14   // Macro slot 3
#define BUTTON_GPIO_15  15   // Macro slot 4
#define BUTTON_GPIO_16  16   // Single mode modifier
#define BUTTON_GPIO_17  17   // Rapid mode modifier

#define BUTTON_COUNT 8
#define SLOT_BUTTON_FIRST BUTTON_GPIO_11
#define SLOT_BUTTON_LAST  BUTTON_GPIO_15
#define MACRO_SLOT_COUNT  5

// Timing constants
#define DEBOUNCE_TIME_MS        50   // Debounce period
#define SIMULTANEOUS_WINDOW_MS  100  // Window for detecting simultaneous press

// Initialize all GPIO buttons (GPIO10-15)
void gpio_button_init(void);

// Update button states with debouncing (call every loop iteration)
// Returns bitmask of buttons that just became pressed (rising edge)
uint8_t gpio_button_update(uint32_t now_ms);

// Check if a specific button is currently pressed (after debounce)
bool gpio_button_is_pressed(uint8_t gpio);

// Check for record combo (GPIO10 + slot button)
// Must call gpio_button_update() before this
// Returns slot number (0-4) if recording should start, -1 otherwise
int8_t gpio_button_check_record_combo(void);

// Check for playback trigger (slot button only, GPIO10 not pressed)
// Must call gpio_button_update() before this
// Returns slot number (0-4) if playback should start, -1 otherwise
int8_t gpio_button_check_playback_trigger(void);

// Check for mode switch combo (GPIO16/17 + slot button)
// Must call gpio_button_update() before this
// Returns slot number (0-4), -1 if none
// *is_rapid = true if GP17 was pressed, false if GP16
int8_t gpio_button_check_mode_combo(bool *is_rapid);

// Check if a slot button is currently held (slot: 0-4)
bool gpio_button_is_slot_held(uint8_t slot);

// Legacy functions for compatibility
bool gpio_button_10_pressed(void);
bool gpio_button_15_pressed(void);

#endif
