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
#include "gpio_button.h"
#include "macro.h"
#include "slot_led.h"
#include "usb_debug.h"

// External LED functions
extern void macro_led_init(void);
extern void macro_led_tick(void);

void usb_init_and_wait_mount(void) {
    printf("USB: Initializing TinyUSB...\n");
    // Use tud_init(0) explicitly like GP2040-CE does
    bool usb_ok = tud_init(0);
    printf("USB: tud_init(0) returned %d\n", usb_ok);

    printf("USB: Waiting for device to mount (Bluetooth held off)...\n");

    // Wait for USB to be mounted before allowing Bluetooth to start
    // This ensures clean USB enumeration without Core1 interference
    while (!tud_mounted()) {
        tud_task();
        // sleep_ms(1);
    }
    printf("USB: Device mounted\n");
}

void usb_core_task(void) {
    // Note: tusb_init() and mount wait are done in usb_init_and_wait_mount()
    // called from main.c before Core1 launch

    printf("USB: Initializing GPIO buttons...\n");
    gpio_button_init();

    // Note: flash_safe_execute_core_init() is called in main.c before Core 1 launch
    // This ensures flash safety is set up before Bluetooth can attempt to store link keys

    // Initialize macro system
    printf("USB: Initializing macro system...\n");
    macro_led_init();
    macro_init();
    slot_led_init();

    // Initialize with neutral report (matching original: lx/ly/rx/ry = 0)
    SwitchOutReport report = {
        .buttons = 0,
        .hat = SWITCH_HAT_NOTHING,
        .lx = 0,
        .ly = 0,
        .rx = 0,
        .ry = 0,
    };

    printf("USB: Device already mounted, continuing...\n");

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

        // Update LEDs
        macro_led_tick();
        slot_led_tick(now_ms);

        // USB processing
        tud_task();

        // Skip if suspended (Remote Wakeup is not supported for Switch 2 compatibility)
        if (tud_suspended()) {
            continue;
        }

        if (tud_hid_ready()) {
            tud_hid_report(0, &final_report, sizeof(final_report));
        }
    }
}

//--------------------------------------------------------------------+
// TinyUSB Device Callbacks (for debug logging)
//--------------------------------------------------------------------+

void tud_mount_cb(void) {
    USB_DBG_STATE("MOUNTED");
}

void tud_umount_cb(void) {
    USB_DBG_STATE("UNMOUNTED");
}

void tud_suspend_cb(bool remote_wakeup_en) {
    USB_DBG_STATE("SUSPENDED (remote_wakeup=%d)", remote_wakeup_en);
}

void tud_resume_cb(void) {
    USB_DBG_STATE("RESUMED");
}
