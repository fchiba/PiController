/*
 * Bluepad32 Platform Implementation for Nintendo Switch output
 * Handles Bluetooth gamepad input and converts to Switch HID format
 */

#include <stdio.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/multicore.h>
#include <uni.h>

#include "sdkconfig.h"
#include "report.h"
#include "switch_descriptors.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Deadzone for analog sticks to prevent drift
#define AXIS_DEADZONE 0x0a

// Current gamepad report
static SwitchOutReport current_report;

// Controller connection state
static bool controller_connected = false;


//
// Helper functions
//

static void empty_gamepad_report(SwitchOutReport *report) {
    report->buttons = 0;
    report->hat = SWITCH_HAT_NOTHING;
    report->lx = SWITCH_JOYSTICK_MID;
    report->ly = SWITCH_JOYSTICK_MID;
    report->rx = SWITCH_JOYSTICK_MID;
    report->ry = SWITCH_JOYSTICK_MID;
}

static uint8_t convert_to_switch_axis(int32_t bluepad_axis) {
    // bluepad32 reports from -512 to 511 as int32_t
    // Switch reports from 0 to 255 as uint8_t (mid = 0x80)

    bluepad_axis += 513;  // now range is 1 to 1024
    bluepad_axis /= 4;    // now range is 0 to 256

    if (bluepad_axis < SWITCH_JOYSTICK_MIN) {
        bluepad_axis = SWITCH_JOYSTICK_MIN;
    } else if ((bluepad_axis > (SWITCH_JOYSTICK_MID - AXIS_DEADZONE)) &&
               (bluepad_axis < (SWITCH_JOYSTICK_MID + AXIS_DEADZONE))) {
        // Apply deadzone - center the stick
        bluepad_axis = SWITCH_JOYSTICK_MID;
    } else if (bluepad_axis > SWITCH_JOYSTICK_MAX) {
        bluepad_axis = SWITCH_JOYSTICK_MAX;
    }

    return (uint8_t)bluepad_axis;
}

static void fill_gamepad_report(uni_gamepad_t *gp) {
    empty_gamepad_report(&current_report);

    // Face buttons
    if (gp->buttons & BUTTON_A) {
        current_report.buttons |= SWITCH_MASK_A;
    }
    if (gp->buttons & BUTTON_B) {
        current_report.buttons |= SWITCH_MASK_B;
    }
    if (gp->buttons & BUTTON_X) {
        current_report.buttons |= SWITCH_MASK_X;
    }
    if (gp->buttons & BUTTON_Y) {
        current_report.buttons |= SWITCH_MASK_Y;
    }

    // Shoulder buttons
    if (gp->buttons & BUTTON_SHOULDER_L) {
        current_report.buttons |= SWITCH_MASK_L;
    }
    if (gp->buttons & BUTTON_SHOULDER_R) {
        current_report.buttons |= SWITCH_MASK_R;
    }

    // D-pad
    switch (gp->dpad) {
        case DPAD_UP:
            current_report.hat = SWITCH_HAT_UP;
            break;
        case DPAD_DOWN:
            current_report.hat = SWITCH_HAT_DOWN;
            break;
        case DPAD_LEFT:
            current_report.hat = SWITCH_HAT_LEFT;
            break;
        case DPAD_RIGHT:
            current_report.hat = SWITCH_HAT_RIGHT;
            break;
        case DPAD_UP | DPAD_RIGHT:
            current_report.hat = SWITCH_HAT_UPRIGHT;
            break;
        case DPAD_DOWN | DPAD_RIGHT:
            current_report.hat = SWITCH_HAT_DOWNRIGHT;
            break;
        case DPAD_DOWN | DPAD_LEFT:
            current_report.hat = SWITCH_HAT_DOWNLEFT;
            break;
        case DPAD_UP | DPAD_LEFT:
            current_report.hat = SWITCH_HAT_UPLEFT;
            break;
        default:
            current_report.hat = SWITCH_HAT_NOTHING;
            break;
    }

    // Analog sticks
    current_report.lx = convert_to_switch_axis(gp->axis_x);
    current_report.ly = convert_to_switch_axis(gp->axis_y);
    current_report.rx = convert_to_switch_axis(gp->axis_rx);
    current_report.ry = convert_to_switch_axis(gp->axis_ry);

    // Thumb buttons (L3/R3)
    if (gp->buttons & BUTTON_THUMB_L) {
        current_report.buttons |= SWITCH_MASK_L3;
    }
    if (gp->buttons & BUTTON_THUMB_R) {
        current_report.buttons |= SWITCH_MASK_R3;
    }

    // Triggers (ZL/ZR) - check both analog and digital
    if (gp->brake || (gp->buttons & BUTTON_TRIGGER_L)) {
        current_report.buttons |= SWITCH_MASK_ZL;
    }
    if (gp->throttle || (gp->buttons & BUTTON_TRIGGER_R)) {
        current_report.buttons |= SWITCH_MASK_ZR;
    }

    // Misc buttons
    if (gp->misc_buttons & MISC_BUTTON_SYSTEM) {
        current_report.buttons |= SWITCH_MASK_HOME;
    }
    if (gp->misc_buttons & MISC_BUTTON_CAPTURE) {
        current_report.buttons |= SWITCH_MASK_CAPTURE;
    }
    if (gp->misc_buttons & MISC_BUTTON_BACK) {
        current_report.buttons |= SWITCH_MASK_MINUS;
    }
    if (gp->misc_buttons & MISC_BUTTON_HOME) {
        current_report.buttons |= SWITCH_MASK_PLUS;
    }
}

static void update_led_status(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, controller_connected ? 1 : 0);
}

//
// Platform Overrides
//

static void switch_platform_init(int argc, const char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("switch_platform: init()\n");

    controller_connected = false;

    // Set up button mappings for Switch layout
    // Swap A/B and X/Y to match Nintendo convention
    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;
    mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;
    mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    mappings.button_y = UNI_GAMEPAD_MAPPINGS_BUTTON_X;
    mappings.button_x = UNI_GAMEPAD_MAPPINGS_BUTTON_Y;
    uni_gamepad_set_mappings(&mappings);

    // Initialize report with neutral values
    empty_gamepad_report(&current_report);
    set_global_gamepad_report(&current_report);
}

static void switch_platform_on_init_complete(void) {
    logi("switch_platform: on_init_complete()\n");

    // Start scanning for controllers and auto-connect
    uni_bt_start_scanning_and_autoconnect_unsafe();

    // Delete stored Bluetooth keys on startup (force re-pairing)
    // uni_bt_del_keys_unsafe();

    // Turn off LED until controller connects
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    logi("switch_platform: ready for controller connection\n");

    // Signal USB core that Bluetooth is ready
    multicore_fifo_push_blocking(0);
}

static uni_error_t switch_platform_on_device_discovered(bd_addr_t addr,
                                                         const char *name,
                                                         uint16_t cod,
                                                         uint8_t rssi) {
    ARG_UNUSED(addr);
    ARG_UNUSED(name);
    ARG_UNUSED(rssi);

    // Filter out keyboards and mice - only accept gamepads
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("switch_platform: ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void switch_platform_on_device_connected(uni_hid_device_t *d) {
    logi("switch_platform: device connected: %p\n", d);
}

static void switch_platform_on_device_disconnected(uni_hid_device_t *d) {
    logi("switch_platform: device disconnected: %p\n", d);

    // Reset report to neutral on disconnect
    empty_gamepad_report(&current_report);
    set_global_gamepad_report(&current_report);

    controller_connected = false;
    update_led_status();
}

static uni_error_t switch_platform_on_device_ready(uni_hid_device_t *d) {
    logi("switch_platform: device ready: %p\n", d);

    controller_connected = true;
    update_led_status();

    return UNI_ERROR_SUCCESS;
}

static void switch_platform_on_controller_data(uni_hid_device_t *d,
                                                uni_controller_t *ctl) {
    ARG_UNUSED(d);

    // Only process gamepad data
    if (ctl->klass != UNI_CONTROLLER_CLASS_GAMEPAD) {
        return;
    }

    uni_gamepad_t *gp = &ctl->gamepad;

    fill_gamepad_report(gp);
    set_global_gamepad_report(&current_report);
}

static const uni_property_t *switch_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void switch_platform_on_oob_event(uni_platform_oob_event_t event,
                                          void *data) {
    ARG_UNUSED(event);
    ARG_UNUSED(data);
}

//
// Entry Point
//

struct uni_platform *get_my_platform(void) {
    static struct uni_platform plat = {
        .name = "Switch Platform",
        .init = switch_platform_init,
        .on_init_complete = switch_platform_on_init_complete,
        .on_device_discovered = switch_platform_on_device_discovered,
        .on_device_connected = switch_platform_on_device_connected,
        .on_device_disconnected = switch_platform_on_device_disconnected,
        .on_device_ready = switch_platform_on_device_ready,
        .on_oob_event = switch_platform_on_oob_event,
        .on_controller_data = switch_platform_on_controller_data,
        .get_property = switch_platform_get_property,
    };

    return &plat;
}
