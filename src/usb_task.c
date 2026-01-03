/*
 * USB HID Task for Nintendo Switch Pro Controller output
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
#include "switch_pro_report.h"
#include "switch_pro_handler.h"
#include "gpio_button.h"
#include "macro.h"
#include "slot_led.h"

// External LED functions
extern void macro_led_init(void);
extern void macro_led_tick(void);

// Global handler instance (accessed by usb_descriptors.c)
SwitchProHandler g_switch_pro_handler;

// Last report timer for keepalive
static uint32_t last_report_timer = 0;
static uint8_t last_report[64] = {0};

void usb_core_task(void) {
    printf("USB: Initializing GPIO buttons...\n");
    gpio_button_init();

    printf("USB: Initializing TinyUSB...\n");
    tusb_init();

    // Initialize flash safety for multi-core operation
    printf("USB: Initializing flash safety for multi-core...\n");
    if (!flash_safe_execute_core_init()) {
        printf("USB: Warning - flash_safe_execute_core_init() failed\n");
    }

    // Initialize macro system
    printf("USB: Initializing macro system...\n");
    macro_led_init();
    macro_init();
    slot_led_init();

    // Initialize Pro Controller handler
    printf("USB: Initializing Pro Controller handler...\n");
    switch_pro_handler_init(&g_switch_pro_handler);

    printf("USB: Waiting for device to mount...\n");

    // Wait for USB to be mounted
    while (!tud_mounted()) {
        tud_task();
        sleep_ms(10);
    }

    printf("USB: Device mounted!\n");

    // Send initial IDENTIFY to start handshake
    printf("USB: Sending initial IDENTIFY...\n");
    switch_pro_handler_send_identify(&g_switch_pro_handler);

    uint8_t response_buffer[64];
    uint16_t response_size;
    uint8_t response_report_id;

    if (switch_pro_handler_get_response(&g_switch_pro_handler,
                                         response_buffer,
                                         &response_size,
                                         &response_report_id)) {
        // Send initial IDENTIFY report
        for (int i = 0; i < 10; i++) {
            tud_task();
            if (tud_hid_ready()) {
                if (tud_hid_report(response_report_id, response_buffer, response_size)) {
                    printf("USB: Initial IDENTIFY sent\n");
                    g_switch_pro_handler.isInitialized = true;
                    break;
                }
            }
            sleep_ms(100);
        }
    }

    last_report_timer = to_ms_since_boot(get_absolute_time());

    printf("USB: Entering main loop (waiting for handshake)\n");

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

        if (tud_suspended()) {
            tud_remote_wakeup();
            continue;
        }

        // Check for queued command responses
        if (switch_pro_handler_has_response(&g_switch_pro_handler)) {
            if ((now_ms - last_report_timer) > SWITCH_PRO_KEEPALIVE_TIMER_MS) {
                if (switch_pro_handler_get_response(&g_switch_pro_handler,
                                                     response_buffer,
                                                     &response_size,
                                                     &response_report_id)) {
                    if (tud_hid_ready()) {
                        tud_hid_report(response_report_id, response_buffer, response_size);
                    }
                }
                last_report_timer = now_ms;
            }
            continue;
        }

        // Only send input reports after handshake is complete
        if (g_switch_pro_handler.isReady) {
            if ((now_ms - last_report_timer) > SWITCH_PRO_KEEPALIVE_TIMER_MS) {
                // Update input from legacy report
                switch_pro_handler_update_input(&g_switch_pro_handler, &final_report);

                // Get Pro Controller format report
                const SwitchProReport *pro_report =
                    switch_pro_handler_get_input_report(&g_switch_pro_handler);

                // Only send if report changed or keepalive needed
                if (memcmp(last_report, pro_report, sizeof(SwitchProReport)) != 0) {
                    if (tud_hid_ready()) {
                        // Send with report ID 0 (report ID is in the data)
                        if (tud_hid_report(0, pro_report, sizeof(SwitchProReport))) {
                            memcpy(last_report, pro_report, sizeof(SwitchProReport));
                            switch_pro_handler_increment_counter(&g_switch_pro_handler);
                        }
                    }
                    last_report_timer = now_ms;
                }
            }
        } else {
            // Handshake not complete - resend IDENTIFY if needed
            if (!g_switch_pro_handler.isInitialized) {
                if ((now_ms - last_report_timer) > 1000) {
                    switch_pro_handler_send_identify(&g_switch_pro_handler);
                    if (switch_pro_handler_get_response(&g_switch_pro_handler,
                                                         response_buffer,
                                                         &response_size,
                                                         &response_report_id)) {
                        if (tud_hid_ready()) {
                            tud_hid_report(response_report_id, response_buffer, response_size);
                            g_switch_pro_handler.isInitialized = true;
                            printf("USB: Resent IDENTIFY\n");
                        }
                    }
                    last_report_timer = now_ms;
                }
            }
        }
    }
}
