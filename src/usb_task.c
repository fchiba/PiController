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

#include "report.h"
#include "switch_descriptors.h"

void usb_core_task(void) {
    printf("USB: Initializing TinyUSB...\n");
    tusb_init();

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
        get_global_gamepad_report(&report);

        tud_task();

        if (tud_suspended()) {
            tud_remote_wakeup();
            continue;
        }

        if (tud_hid_ready()) {
            tud_hid_report(0, &report, sizeof(report));
        }
    }
}
