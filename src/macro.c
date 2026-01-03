#include "macro.h"
#include "gpio_button.h"
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>

// Forward declarations for flash and LED modules
extern bool macro_flash_save(uint8_t slot, const MacroFrame *frames, uint16_t frame_count);
extern bool macro_flash_load(uint8_t slot, MacroFrame *frames, uint16_t *frame_count);
extern void macro_led_set_recording(bool recording);
extern void macro_led_set_playing(bool playing);
extern void macro_led_flash_save_result(bool success);

// Global macro context
static MacroContext ctx;

// Slot playback mode
typedef enum {
    SLOT_MODE_SINGLE = 0,     // Single shot (default)
    SLOT_MODE_RAPID = 1,      // Rapid fire (hold button)
    SLOT_MODE_CONTINUOUS = 2  // Continuous (toggle)
} SlotMode;

// Per-slot mode (reset on power cycle)
static SlotMode slot_mode[MACRO_SLOT_COUNT] = {0};

// Check if report is neutral (no buttons, sticks centered)
static bool is_neutral_report(const SwitchOutReport *report) {
    return (report->buttons == 0 &&
            report->hat == SWITCH_HAT_NOTHING &&
            report->lx == SWITCH_JOYSTICK_MID &&
            report->ly == SWITCH_JOYSTICK_MID &&
            report->rx == SWITCH_JOYSTICK_MID &&
            report->ry == SWITCH_JOYSTICK_MID);
}

// Check if two reports have different button/hat states
static bool buttons_changed(const SwitchOutReport *a, const SwitchOutReport *b) {
    return (a->buttons != b->buttons || a->hat != b->hat);
}

// Check if two reports have different stick states
static bool sticks_changed(const SwitchOutReport *a, const SwitchOutReport *b) {
    return (a->lx != b->lx || a->ly != b->ly ||
            a->rx != b->rx || a->ry != b->ry);
}

// Convert SwitchOutReport to MacroFrame
static void report_to_frame(MacroFrame *frame, const SwitchOutReport *report, uint16_t delta_ms) {
    frame->delta_ms = delta_ms;
    frame->buttons = report->buttons;
    frame->hat = report->hat;
    frame->lx = report->lx;
    frame->ly = report->ly;
    frame->rx = report->rx;
    frame->ry = report->ry;
}

// Convert MacroFrame to SwitchOutReport
static void frame_to_report(SwitchOutReport *report, const MacroFrame *frame) {
    report->buttons = frame->buttons;
    report->hat = frame->hat;
    report->lx = frame->lx;
    report->ly = frame->ly;
    report->rx = frame->rx;
    report->ry = frame->ry;
}

// Merge stick axis values (larger deviation from center wins)
static uint8_t merge_stick_axis(uint8_t controller, uint8_t macro_val) {
    int ctrl_dev = (int)controller - SWITCH_JOYSTICK_MID;
    int macro_dev = (int)macro_val - SWITCH_JOYSTICK_MID;

    // Use absolute deviation comparison
    if (ctrl_dev < 0) ctrl_dev = -ctrl_dev;
    if (macro_dev < 0) macro_dev = -macro_dev;

    return (macro_dev > ctrl_dev) ? macro_val : controller;
}

void macro_init(void) {
    memset(&ctx, 0, sizeof(ctx));
    ctx.state = MACRO_STATE_IDLE;

    // Initialize neutral last report
    ctx.last_report.buttons = 0;
    ctx.last_report.hat = SWITCH_HAT_NOTHING;
    ctx.last_report.lx = SWITCH_JOYSTICK_MID;
    ctx.last_report.ly = SWITCH_JOYSTICK_MID;
    ctx.last_report.rx = SWITCH_JOYSTICK_MID;
    ctx.last_report.ry = SWITCH_JOYSTICK_MID;

    printf("Macro: System initialized\n");
}

// Start recording to a slot
static void start_recording(uint8_t slot, uint32_t now_ms) {
    ctx.state = MACRO_STATE_RECORDING;
    ctx.active_slot = slot;
    ctx.record_count = 0;
    ctx.record_start_time = now_ms;
    ctx.last_frame_time = now_ms;
    ctx.first_input_received = false;

    // Reset last report to neutral
    ctx.last_report.buttons = 0;
    ctx.last_report.hat = SWITCH_HAT_NOTHING;
    ctx.last_report.lx = SWITCH_JOYSTICK_MID;
    ctx.last_report.ly = SWITCH_JOYSTICK_MID;
    ctx.last_report.rx = SWITCH_JOYSTICK_MID;
    ctx.last_report.ry = SWITCH_JOYSTICK_MID;

    macro_led_set_recording(true);
    printf("Macro: Recording started for slot %d\n", slot);
}

// Stop recording and save to flash
static void stop_recording(uint32_t now_ms) {
    macro_led_set_recording(false);

    // Trim trailing neutral frames
    while (ctx.record_count > 0) {
        MacroFrame *last = &ctx.record_buffer[ctx.record_count - 1];
        if (last->buttons == 0 &&
            last->hat == SWITCH_HAT_NOTHING &&
            last->lx == SWITCH_JOYSTICK_MID &&
            last->ly == SWITCH_JOYSTICK_MID &&
            last->rx == SWITCH_JOYSTICK_MID &&
            last->ry == SWITCH_JOYSTICK_MID) {
            ctx.record_count--;
        } else {
            break;
        }
    }

    if (ctx.record_count > 0) {
        printf("Macro: Recording stopped, %d frames captured\n", ctx.record_count);

        // Save to flash
        bool success = macro_flash_save(ctx.active_slot, ctx.record_buffer, ctx.record_count);
        macro_led_flash_save_result(success);

        if (success) {
            printf("Macro: Saved to slot %d\n", ctx.active_slot);
        } else {
            printf("Macro: Failed to save to slot %d\n", ctx.active_slot);
        }
    } else {
        printf("Macro: Recording stopped, no frames captured\n");
    }

    ctx.state = MACRO_STATE_IDLE;
}

// Start playback from a slot
static void start_playback(uint8_t slot, uint32_t now_ms) {
    uint16_t frame_count = 0;

    if (!macro_flash_load(slot, ctx.play_buffer, &frame_count)) {
        printf("Macro: No macro in slot %d or load failed\n", slot);
        return;
    }

    if (frame_count == 0) {
        printf("Macro: Slot %d is empty\n", slot);
        return;
    }

    ctx.state = MACRO_STATE_PLAYING;
    ctx.active_slot = slot;
    ctx.play_frame_count = frame_count;
    ctx.play_index = 0;
    ctx.play_start_time = now_ms;

    // First frame plays immediately
    ctx.next_frame_time = now_ms;

    // Initialize current frame report
    frame_to_report(&ctx.current_frame_report, &ctx.play_buffer[0]);

    macro_led_set_playing(true);
    printf("Macro: Playback started for slot %d (%d frames)\n", slot, frame_count);
}

// Stop playback
static void stop_playback(void) {
    macro_led_set_playing(false);
    ctx.state = MACRO_STATE_IDLE;
    printf("Macro: Playback stopped\n");
}

void macro_process_buttons(uint32_t now_ms) {
    // Update all buttons once at the start
    gpio_button_update(now_ms);

    switch (ctx.state) {
        case MACRO_STATE_IDLE: {
            // Check for mode switch combo (GPIO16/17/18 + slot)
            uint8_t mode;
            int8_t mode_slot = gpio_button_check_mode_combo(&mode);
            if (mode_slot >= 0) {
                slot_mode[mode_slot] = (SlotMode)mode;
                const char *mode_names[] = {"single", "rapid", "continuous"};
                printf("Macro: Slot %d set to %s mode\n",
                       mode_slot, mode_names[mode]);
                return;
            }

            // Check for record combo (GPIO10 + slot)
            int8_t record_slot = gpio_button_check_record_combo();
            if (record_slot >= 0) {
                start_recording((uint8_t)record_slot, now_ms);
                return;
            }

            // Check for playback trigger (slot only)
            int8_t play_slot = gpio_button_check_playback_trigger();
            if (play_slot >= 0) {
                start_playback((uint8_t)play_slot, now_ms);
                return;
            }
            break;
        }

        case MACRO_STATE_RECORDING: {
            // Check timeout
            if ((now_ms - ctx.record_start_time) >= MACRO_MAX_DURATION_MS) {
                printf("Macro: Recording timeout\n");
                stop_recording(now_ms);
                return;
            }

            // Check if slot button pressed alone (GPIO10 released) to stop recording
            int8_t slot_pressed = gpio_button_check_playback_trigger();
            if (slot_pressed == ctx.active_slot) {
                stop_recording(now_ms);
                return;
            }
            break;
        }

        case MACRO_STATE_PLAYING:
            // Continuous mode: pressing slot button again stops playback
            if (slot_mode[ctx.active_slot] == SLOT_MODE_CONTINUOUS) {
                int8_t pressed_slot = gpio_button_check_playback_trigger();
                if (pressed_slot == ctx.active_slot) {
                    stop_playback();
                    return;
                }
            }
            break;
    }
}

void macro_tick(SwitchOutReport *output, const SwitchOutReport *controller_input, uint32_t now_ms) {
    switch (ctx.state) {
        case MACRO_STATE_IDLE:
            // Pass through controller input
            memcpy(output, controller_input, sizeof(SwitchOutReport));
            break;

        case MACRO_STATE_RECORDING: {
            // Pass through controller input
            memcpy(output, controller_input, sizeof(SwitchOutReport));

            // Skip if no input yet (trim leading silence)
            if (!ctx.first_input_received) {
                if (is_neutral_report(controller_input)) {
                    return;
                }
                ctx.first_input_received = true;
                ctx.last_frame_time = now_ms;
            }

            uint32_t delta = now_ms - ctx.last_frame_time;

            // Determine if we should record this frame
            bool should_record = false;

            if (buttons_changed(controller_input, &ctx.last_report)) {
                // Always record button changes immediately
                should_record = true;
            } else if (sticks_changed(controller_input, &ctx.last_report) &&
                       delta >= MACRO_FRAME_INTERVAL_MS) {
                // Thin stick movements to 100ms intervals
                should_record = true;
            } else if (delta >= MACRO_FRAME_INTERVAL_MS &&
                       !is_neutral_report(controller_input)) {
                // Also record periodically if there's any input
                should_record = true;
            }

            if (should_record && ctx.record_count < MACRO_MAX_FRAMES) {
                MacroFrame *frame = &ctx.record_buffer[ctx.record_count];
                uint16_t capped_delta = (delta > 65535) ? 65535 : (uint16_t)delta;
                report_to_frame(frame, controller_input, capped_delta);

                ctx.record_count++;
                ctx.last_frame_time = now_ms;
                memcpy(&ctx.last_report, controller_input, sizeof(SwitchOutReport));
            }

            // Check if buffer full
            if (ctx.record_count >= MACRO_MAX_FRAMES) {
                printf("Macro: Buffer full\n");
                stop_recording(now_ms);
            }
            break;
        }

        case MACRO_STATE_PLAYING: {
            // Rapid mode: stop immediately when button released
            if (slot_mode[ctx.active_slot] == SLOT_MODE_RAPID &&
                !gpio_button_is_slot_held(ctx.active_slot)) {
                stop_playback();
                memcpy(output, controller_input, sizeof(SwitchOutReport));
                break;
            }

            // Start with controller input
            memcpy(output, controller_input, sizeof(SwitchOutReport));

            // Advance frames based on timing
            while (ctx.play_index < ctx.play_frame_count && now_ms >= ctx.next_frame_time) {
                // Update current frame report
                frame_to_report(&ctx.current_frame_report, &ctx.play_buffer[ctx.play_index]);

                ctx.play_index++;

                if (ctx.play_index < ctx.play_frame_count) {
                    ctx.next_frame_time += ctx.play_buffer[ctx.play_index].delta_ms;
                }
            }

            // Merge macro output with controller input
            if (ctx.play_index > 0 && ctx.play_index <= ctx.play_frame_count) {
                // Merge buttons (OR)
                output->buttons |= ctx.current_frame_report.buttons;

                // Merge HAT (macro priority if not neutral)
                if (ctx.current_frame_report.hat != SWITCH_HAT_NOTHING) {
                    output->hat = ctx.current_frame_report.hat;
                }

                // Merge sticks (larger deviation wins)
                output->lx = merge_stick_axis(output->lx, ctx.current_frame_report.lx);
                output->ly = merge_stick_axis(output->ly, ctx.current_frame_report.ly);
                output->rx = merge_stick_axis(output->rx, ctx.current_frame_report.rx);
                output->ry = merge_stick_axis(output->ry, ctx.current_frame_report.ry);
            }

            // Check if playback complete
            if (ctx.play_index >= ctx.play_frame_count) {
                bool should_loop = false;

                if (slot_mode[ctx.active_slot] == SLOT_MODE_RAPID &&
                    gpio_button_is_slot_held(ctx.active_slot)) {
                    // Rapid mode: loop while button held
                    should_loop = true;
                } else if (slot_mode[ctx.active_slot] == SLOT_MODE_CONTINUOUS) {
                    // Continuous mode: always loop
                    should_loop = true;
                }

                if (should_loop) {
                    // Restart with a gap (neutral frame) before first frame
                    ctx.play_index = 0;
                    ctx.next_frame_time = now_ms + 100;  // 100ms gap between loops
                    // Set current frame to neutral during the gap
                    ctx.current_frame_report.buttons = 0;
                    ctx.current_frame_report.hat = SWITCH_HAT_NOTHING;
                    ctx.current_frame_report.lx = SWITCH_JOYSTICK_MID;
                    ctx.current_frame_report.ly = SWITCH_JOYSTICK_MID;
                    ctx.current_frame_report.rx = SWITCH_JOYSTICK_MID;
                    ctx.current_frame_report.ry = SWITCH_JOYSTICK_MID;
                } else {
                    stop_playback();
                }
            }
            break;
        }
    }
}

MacroState macro_get_state(void) {
    return ctx.state;
}

uint8_t macro_get_active_slot(void) {
    return ctx.active_slot;
}

bool macro_is_busy(void) {
    return ctx.state != MACRO_STATE_IDLE;
}

void macro_stop(void) {
    if (ctx.state == MACRO_STATE_RECORDING) {
        stop_recording(to_ms_since_boot(get_absolute_time()));
    } else if (ctx.state == MACRO_STATE_PLAYING) {
        stop_playback();
    }
}
