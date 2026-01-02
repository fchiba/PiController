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

    // Button state tracking
    bool prev_btn10 = false;
    bool prev_btn15 = false;

    // Main loop
    while (1) {
        // Check button state changes
        bool btn10 = gpio_button_10_pressed();
        bool btn15 = gpio_button_15_pressed();

        if (btn10 != prev_btn10) {
            printf("Button GPIO10: %s\n", btn10 ? "PRESSED" : "RELEASED");
            prev_btn10 = btn10;
        }
        if (btn15 != prev_btn15) {
            printf("Button GPIO15: %s\n", btn15 ? "PRESSED" : "RELEASED");
            prev_btn15 = btn15;
        }

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
