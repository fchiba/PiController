#ifndef MACRO_TYPES_H
#define MACRO_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "switch_descriptors.h"

// Macro configuration
#define MACRO_MAGIC             0x4D414352  // 'MACR'
#define MACRO_SLOT_COUNT        5
#define MACRO_MAX_FRAMES        1250        // 10 seconds at 125Hz (no thinning)
#define MACRO_FRAME_INTERVAL_MS 64          // Stick averaging interval
#define MACRO_MAX_DURATION_MS   10000       // 10 seconds max recording

// Flash storage configuration
// NOTE: BTstack uses last 8KB of flash for link key storage (PICO_FLASH_BANK)
// Macros must be placed before that to avoid overwriting pairing data
#define FLASH_SECTOR_SIZE       4096
#define MACRO_FLASH_SECTORS     16          // 64KB total for macros
#define BTSTACK_FLASH_SIZE      (2 * 4096)  // BTstack uses last 8KB
#define MACRO_SLOT_SIZE         12288       // Each slot: 12KB (header + 1250 frames)

// Single macro frame (9 bytes)
typedef struct __attribute__((packed)) {
    uint16_t delta_ms;  // Time since previous frame (0-65535ms)
    uint16_t buttons;   // Button state
    uint8_t hat;        // D-pad HAT
    uint8_t lx;         // Left stick X
    uint8_t ly;         // Left stick Y
    uint8_t rx;         // Right stick X
    uint8_t ry;         // Right stick Y
} MacroFrame;

// Macro header stored at start of each slot (12 bytes)
typedef struct __attribute__((packed)) {
    uint32_t magic;         // MACRO_MAGIC for validation
    uint16_t frame_count;   // Number of frames (0 = empty)
    uint16_t total_time_ms; // Total playback duration
    uint32_t checksum;      // CRC32 of frames
} MacroHeader;

// Macro system state machine
typedef enum {
    MACRO_STATE_IDLE,       // Normal operation
    MACRO_STATE_RECORDING,  // Recording input to a slot
    MACRO_STATE_PLAYING     // Playing back from a slot
} MacroState;

// Runtime context
typedef struct {
    MacroState state;
    uint8_t active_slot;        // 0-4 (maps to GPIO11-15)

    // Recording state
    MacroFrame record_buffer[MACRO_MAX_FRAMES];
    uint16_t record_count;
    uint32_t record_start_time;
    uint32_t last_frame_time;
    bool first_input_received;
    SwitchOutReport last_report;

    // Stick averaging accumulators
    int32_t lx_sum, ly_sum, rx_sum, ry_sum;
    uint16_t stick_sample_count;

    // Playback state
    MacroFrame play_buffer[MACRO_MAX_FRAMES];  // Copy from flash
    uint16_t play_frame_count;
    uint16_t play_index;
    uint32_t play_start_time;
    uint32_t next_frame_time;
    SwitchOutReport current_frame_report;  // Current frame's report for merging
} MacroContext;

#endif // MACRO_TYPES_H
