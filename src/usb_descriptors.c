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
// Switch requires both IN and OUT endpoints - use TUD_HID_INOUT_DESCRIPTOR
//--------------------------------------------------------------------+

enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

// TUD_HID_INOUT_DESCRIPTOR length: 9 (interface) + 9 (HID) + 7 (EP OUT) + 7 (EP IN) = 32
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)
#define EPNUM_HID_OUT 0x02
#define EPNUM_HID_IN  0x81

static uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1,
                          ITF_NUM_TOTAL,
                          0,
                          CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                          500),

    // Interface number, string index, protocol, report descriptor len, EP OUT, EP IN, size & polling interval
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID,
                              0,
                              HID_ITF_PROTOCOL_NONE,
                              sizeof(switch_report_descriptor),
                              EPNUM_HID_OUT,
                              EPNUM_HID_IN,
                              64,  // EP size must be 64 for Switch
                              1),  // Polling interval 1ms
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    USB_DBG_DESC("config_cb index=%d len=%zu", index, sizeof(desc_configuration));
#if USB_DEBUG_HEXDUMP
    usb_debug_hexdump("config_desc", desc_configuration, sizeof(desc_configuration));
#endif
    (void)index;
    return desc_configuration;
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
