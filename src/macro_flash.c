#include "macro_types.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/multicore.h>
#include <string.h>
#include <stdio.h>

// Flash storage at end of flash memory
#define FLASH_MACRO_OFFSET (PICO_FLASH_SIZE_BYTES - (MACRO_FLASH_SECTORS * FLASH_SECTOR_SIZE))
#define FLASH_MACRO_ADDR   (XIP_BASE + FLASH_MACRO_OFFSET)

// CRC32 calculation
static uint32_t crc32(const void *data, size_t len) {
    const uint8_t *buf = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }

    return ~crc;
}

// Buffer for all slots data (must preserve other slots during erase)
static uint8_t all_slots_data[MACRO_SLOT_COUNT * MACRO_SLOT_SIZE];

bool macro_flash_save(uint8_t slot, const MacroFrame *frames, uint16_t frame_count) {
    if (slot >= MACRO_SLOT_COUNT || frame_count > MACRO_MAX_FRAMES) {
        return false;
    }

    // Calculate total time
    uint32_t total_time = 0;
    for (uint16_t i = 0; i < frame_count; i++) {
        total_time += frames[i].delta_ms;
    }

    // Calculate checksum
    uint32_t checksum = crc32(frames, frame_count * sizeof(MacroFrame));

    MacroHeader header = {
        .magic = MACRO_MAGIC,
        .frame_count = frame_count,
        .total_time_ms = (total_time > 65535) ? 65535 : (uint16_t)total_time,
        .checksum = checksum
    };

    printf("Macro Flash: Saving slot %d, %d frames, %lu ms total\n",
           slot, frame_count, (unsigned long)total_time);

    // Read all existing slot data first (before erase)
    memcpy(all_slots_data, (const void *)FLASH_MACRO_ADDR, sizeof(all_slots_data));

    // Update the target slot in the buffer
    uint8_t *slot_data = &all_slots_data[slot * MACRO_SLOT_SIZE];
    memset(slot_data, 0xFF, MACRO_SLOT_SIZE);
    memcpy(slot_data, &header, sizeof(MacroHeader));
    if (frame_count > 0) {
        memcpy(slot_data + sizeof(MacroHeader), frames,
               frame_count * sizeof(MacroFrame));
    }

    // Pause Core 1 (Bluetooth) during flash operations
    // This prevents Core 1 from trying to execute code from flash while we modify it
    multicore_lockout_start_blocking();

    // Disable interrupts during flash operations
    uint32_t ints = save_and_disable_interrupts();

    // Erase both sectors (8KB total)
    flash_range_erase(FLASH_MACRO_OFFSET, MACRO_FLASH_SECTORS * FLASH_SECTOR_SIZE);

    // Program all slots back
    flash_range_program(FLASH_MACRO_OFFSET, all_slots_data, sizeof(all_slots_data));

    // Restore interrupts
    restore_interrupts(ints);

    // Resume Core 1
    multicore_lockout_end_blocking();

    printf("Macro Flash: Save complete\n");
    return true;
}

bool macro_flash_load(uint8_t slot, MacroFrame *frames, uint16_t *frame_count) {
    if (slot >= MACRO_SLOT_COUNT || frames == NULL || frame_count == NULL) {
        *frame_count = 0;
        return false;
    }

    const uint8_t *slot_addr = (const uint8_t *)(FLASH_MACRO_ADDR + slot * MACRO_SLOT_SIZE);
    const MacroHeader *header = (const MacroHeader *)slot_addr;

    // Validate magic
    if (header->magic != MACRO_MAGIC) {
        printf("Macro Flash: Slot %d has invalid magic (0x%08lx)\n",
               slot, (unsigned long)header->magic);
        *frame_count = 0;
        return false;
    }

    // Check frame count
    if (header->frame_count == 0 || header->frame_count > MACRO_MAX_FRAMES) {
        printf("Macro Flash: Slot %d has invalid frame count (%d)\n",
               slot, header->frame_count);
        *frame_count = 0;
        return false;
    }

    const MacroFrame *stored_frames = (const MacroFrame *)(slot_addr + sizeof(MacroHeader));

    // Validate checksum
    uint32_t checksum = crc32(stored_frames, header->frame_count * sizeof(MacroFrame));
    if (checksum != header->checksum) {
        printf("Macro Flash: Slot %d checksum mismatch (expected 0x%08lx, got 0x%08lx)\n",
               slot, (unsigned long)header->checksum, (unsigned long)checksum);
        *frame_count = 0;
        return false;
    }

    // Copy frames to output buffer
    memcpy(frames, stored_frames, header->frame_count * sizeof(MacroFrame));
    *frame_count = header->frame_count;

    printf("Macro Flash: Loaded slot %d, %d frames, %d ms total\n",
           slot, header->frame_count, header->total_time_ms);

    return true;
}

// Check if a slot has valid macro data
bool macro_flash_slot_valid(uint8_t slot) {
    if (slot >= MACRO_SLOT_COUNT) {
        return false;
    }

    const uint8_t *slot_addr = (const uint8_t *)(FLASH_MACRO_ADDR + slot * MACRO_SLOT_SIZE);
    const MacroHeader *header = (const MacroHeader *)slot_addr;

    if (header->magic != MACRO_MAGIC || header->frame_count == 0) {
        return false;
    }

    const MacroFrame *stored_frames = (const MacroFrame *)(slot_addr + sizeof(MacroHeader));
    uint32_t checksum = crc32(stored_frames, header->frame_count * sizeof(MacroFrame));

    return (checksum == header->checksum);
}

// Get info about a slot without loading all data
bool macro_flash_get_info(uint8_t slot, uint16_t *frame_count, uint16_t *total_time_ms) {
    if (slot >= MACRO_SLOT_COUNT) {
        return false;
    }

    const uint8_t *slot_addr = (const uint8_t *)(FLASH_MACRO_ADDR + slot * MACRO_SLOT_SIZE);
    const MacroHeader *header = (const MacroHeader *)slot_addr;

    if (header->magic != MACRO_MAGIC || header->frame_count == 0) {
        return false;
    }

    if (frame_count) *frame_count = header->frame_count;
    if (total_time_ms) *total_time_ms = header->total_time_ms;

    return true;
}
