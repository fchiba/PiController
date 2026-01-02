/*
 * picontroller2 - Bluetooth Gamepad to Nintendo Switch USB HID Adapter
 *
 * Dual-core architecture:
 * - Core 0: USB HID output (TinyUSB)
 * - Core 1: Bluetooth input (bluepad32 + BTstack)
 */

#include <btstack_run_loop.h>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <uni.h>

#include "sdkconfig.h"
#include "usb_task.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Defined in switch_platform.c
struct uni_platform *get_my_platform(void);

// Bluetooth task - runs on Core 1
static void bluetooth_core_task(void) {
    // Initialize CYW43 driver (enables Bluetooth)
    if (cyw43_arch_init()) {
        loge("Failed to initialize cyw43_arch\n");
        return;
    }

    // Turn on LED during initialization
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    // Set custom platform (must be called before uni_init)
    uni_platform_set_custom(get_my_platform());

    // Initialize bluepad32
    uni_init(0, NULL);

    // Run BTstack event loop (does not return)
    btstack_run_loop_execute();
}

int main(void) {
    stdio_init_all();

    // Launch Bluetooth on Core 1
    multicore_launch_core1(bluetooth_core_task);

    // Run USB on Core 0 (this core)
    usb_core_task();

    return 0;
}
