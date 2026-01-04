/*
 * TinyUSB Descriptor callbacks for Nintendo Switch HID
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "switch_descriptors.h"
#include "usb_debug.h"

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+

uint8_t const *tud_descriptor_device_cb(void) {
    USB_DBG_DESC("device_cb called");
#if USB_DEBUG_HEXDUMP
    usb_debug_hexdump("device_desc", switch_device_descriptor, sizeof(switch_device_descriptor));
#endif
    return switch_device_descriptor;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    USB_DBG_DESC("report_desc_cb instance=%d len=%zu", instance, sizeof(switch_report_descriptor));
#if USB_DEBUG_HEXDUMP
    usb_debug_hexdump("report_desc", switch_report_descriptor, sizeof(switch_report_descriptor));
#endif
    (void)instance;
    return switch_report_descriptor;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
// Use static descriptor from switch_descriptors.h for Switch 2 compatibility
// (bmAttributes must be 0x80, not 0xA0 with Remote Wakeup)
//--------------------------------------------------------------------+

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    USB_DBG_DESC("config_cb index=%d len=%zu", index, sizeof(switch_configuration_descriptor));
#if USB_DEBUG_HEXDUMP
    usb_debug_hexdump("config_desc", switch_configuration_descriptor, sizeof(switch_configuration_descriptor));
#endif
    (void)index;
    return switch_configuration_descriptor;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    USB_DBG_DESC("string_cb index=%d langid=0x%04x", index, langid);

    (void)langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], switch_string_descriptors[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(switch_string_descriptors) / sizeof(switch_string_descriptors[0])) {
            return NULL;
        }

        const char *str = (const char *)switch_string_descriptors[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) {
            chr_count = 31;
        }

        // Convert ASCII string into UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // First byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}

//--------------------------------------------------------------------+
// HID Callbacks (required by TinyUSB)
//--------------------------------------------------------------------+

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance,
                                uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t *buffer,
                                uint16_t reqlen) {
    USB_DBG_XFER("GET_REPORT instance=%d id=0x%02x type=%d reqlen=%d", instance, report_id, report_type, reqlen);
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

// Invoked when received SET_REPORT control request or data on OUT endpoint
void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize) {
    USB_DBG_XFER("SET_REPORT instance=%d id=0x%02x type=%d len=%d", instance, report_id, report_type, bufsize);
#if USB_DEBUG_HEXDUMP
    usb_debug_hexdump("SET_REPORT data", buffer, bufsize);
#endif
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
