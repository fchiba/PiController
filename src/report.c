/*
 * Thread-safe gamepad report sharing between cores
 */

#include "report.h"

#include <string.h>
#include <pico/multicore.h>
#include <pico/async_context.h>
#include <pico/cyw43_arch.h>

// Shared report between cores
static SwitchOutReport shared_report = {
    .buttons = 0,
    .hat = SWITCH_HAT_NOTHING,
    .lx = SWITCH_JOYSTICK_MID,
    .ly = SWITCH_JOYSTICK_MID,
    .rx = SWITCH_JOYSTICK_MID,
    .ry = SWITCH_JOYSTICK_MID,
};

void set_global_gamepad_report(const SwitchOutReport *report) {
    if (!report) {
        return;
    }

    async_context_t *context = cyw43_arch_async_context();
    async_context_acquire_lock_blocking(context);
    memcpy(&shared_report, report, sizeof(shared_report));
    async_context_release_lock(context);

    // Signal USB core that new data is available
    multicore_fifo_push_timeout_us(0, 1);
}

static uint32_t fifo_unused;

void get_global_gamepad_report(SwitchOutReport *report) {
    // Wait for signal from Bluetooth core (with short timeout)
    multicore_fifo_pop_timeout_us(1, &fifo_unused);

    async_context_t *context = cyw43_arch_async_context();
    async_context_acquire_lock_blocking(context);
    memcpy(report, &shared_report, sizeof(*report));
    async_context_release_lock(context);
}
