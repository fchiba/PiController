#ifndef MACRO_H
#define MACRO_H

#include <stdint.h>
#include <stdbool.h>
#include "switch_descriptors.h"
#include "macro_types.h"

// Initialize macro system (call once at startup from Core 0)
void macro_init(void);

// Process GPIO buttons for macro control (call every loop iteration)
// Handles record/playback triggers based on button states
void macro_process_buttons(uint32_t now_ms);

// Process macro recording/playback (call every loop iteration)
// Takes controller input, returns merged output
void macro_tick(SwitchOutReport *output, const SwitchOutReport *controller_input, uint32_t now_ms);

// Get current macro state
MacroState macro_get_state(void);

// Get active slot (valid when recording or playing)
uint8_t macro_get_active_slot(void);

// Check if macro system is busy (recording or playing)
bool macro_is_busy(void);

// Force stop recording/playback
void macro_stop(void);

#endif // MACRO_H
