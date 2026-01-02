/*
 * USB HID Task for Nintendo Switch output
 * Runs on Core 0
 */

#include "usb_task.h"

#include <stdio.h>
#include <tusb.h>
#include <stdint.h>
#include <stdbool.h>

#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/flash.h>

#include "report.h"
#include "switch_descriptors.h"
#include "gpio_button.h"
#include "macro.h"

// External LED functions
extern void macro_led_init(void);
extern void macro_led_tick(void);

void usb_core_task(void) {
    printf("USB: Initializing GPIO buttons...\n");
    gpio_button_init();

    printf("USB: Initializing TinyUSB...\n");
    tusb_init();

    // Initialize flash safety for multi-core operation
    // This allows Core 1 (Bluetooth) to safely write to flash for link key storage
    printf("USB: Initializing flash safety for multi-core...\n");
    if (!flash_safe_execute_core_init()) {
        printf("USB: Warning - flash_safe_execute_core_init() failed\n");
    }

    // Initialize macro system
    printf("USB: Initializing macro system...\n");
    macro_led_init();
    macro_init();

    // Initialize with neutral report (matching original: lx/ly/rx/ry = 0)
    SwitchOutReport report = {
        .buttons = 0,
        .hat = SWITCH_HAT_NOTHING,
        .lx = 0,
        .ly = 0,
        .rx = 0,
        .ry = 0,
    };

    printf("USB: Waiting for device to mount...\n");

    // Wait for USB to be mounted
    while (!tud_mounted()) {
        tud_task();
        sleep_ms(10);
    }

    printf("USB: Device mounted!\n");

    // Send empty reports for ~5 seconds to ensure Switch recognizes the device
    // Original code runs exactly 50 iterations regardless of FIFO status
    printf("USB: Sending init reports...\n");
    for (uint8_t runs = 50; runs > 0; runs--) {
        tud_task();
        if (tud_hid_ready()) {
            tud_hid_report(0, &report, sizeof(report));
        }
        sleep_ms(100);
    }

    printf("USB: Init complete, entering main loop\n");

    // Main loop
    while (1) {
        uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        // Process macro buttons (record/playback triggers)
        macro_process_buttons(now_ms);

        // Get controller input from Bluetooth core
        SwitchOutReport controller_report;
        get_global_gamepad_report(&controller_report);

        // Process macro (recording captures input, playback merges output)
        SwitchOutReport final_report;
        macro_tick(&final_report, &controller_report, now_ms);

        // Update LED
        macro_led_tick();

        // USB processing
        tud_task();

        if (tud_suspended()) {
            tud_remote_wakeup();
            continue;
        }

        if (tud_hid_ready()) {
            tud_hid_report(0, &final_report, sizeof(final_report));
        }
    }
}
