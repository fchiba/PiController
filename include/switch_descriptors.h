/*
 * Nintendo Switch Pro Controller USB HID Descriptors
 * Based on GP2040-CE SwitchProDescriptors
 * SPDX-License-Identifier: MIT
 */

#ifndef _SWITCH_DESCRIPTORS_H_
#define _SWITCH_DESCRIPTORS_H_

#include <stdint.h>

#define SWITCH_ENDPOINT_SIZE 64

// USB Descriptor strings
static const uint8_t switch_string_language[]     = { 0x09, 0x04 };
static const uint8_t switch_string_manufacturer[] = "Nintendo Co., Ltd.";
static const uint8_t switch_string_product[]      = "Pro Controller";
static const uint8_t switch_string_version[]      = "000000000001";

static const uint8_t *switch_string_descriptors[] = {
    switch_string_language,
    switch_string_manufacturer,
    switch_string_product,
    switch_string_version
};

// USB Device Descriptor - VID:0x057E PID:0x2009 (Nintendo Pro Controller)
static const uint8_t switch_device_descriptor[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0x00,       // bDeviceClass (Use class info in Interface Descriptors)
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0 64
    0x7E, 0x05, // idVendor 0x057E (Nintendo)
    0x09, 0x20, // idProduct 0x2009 (Pro Controller)
    0x10, 0x02, // bcdDevice 2.10
    0x01,       // iManufacturer (String Index)
    0x02,       // iProduct (String Index)
    0x03,       // iSerialNumber (String Index)
    0x01,       // bNumConfigurations 1
};

// USB HID Descriptor
static const uint8_t switch_hid_descriptor[] = {
    0x09,       // bLength
    0x21,       // bDescriptorType (HID)
    0x11, 0x01, // bcdHID 1.11
    0x00,       // bCountryCode
    0x01,       // bNumDescriptors
    0x22,       // bDescriptorType[0] (HID)
    0xCB, 0x00, // wDescriptorLength[0] 203
};

// USB Configuration Descriptor (single interface)
static const uint8_t switch_configuration_descriptor[] = {
    0x09,       // bLength
    0x02,       // bDescriptorType (Configuration)
    0x29, 0x00, // wTotalLength 41
    0x01,       // bNumInterfaces 1
    0x01,       // bConfigurationValue
    0x00,       // iConfiguration (String Index)
    0xA0,       // bmAttributes (Remote Wakeup)
    0xFA,       // bMaxPower 500mA

    0x09,       // bLength
    0x04,       // bDescriptorType (Interface)
    0x00,       // bInterfaceNumber 0
    0x00,       // bAlternateSetting
    0x02,       // bNumEndpoints 2
    0x03,       // bInterfaceClass (HID)
    0x00,       // bInterfaceSubClass
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface (String Index)

    0x09,       // bLength
    0x21,       // bDescriptorType (HID)
    0x11, 0x01, // bcdHID 1.11
    0x00,       // bCountryCode
    0x01,       // bNumDescriptors
    0x22,       // bDescriptorType[0] (HID)
    0xCB, 0x00, // wDescriptorLength[0] 203

    0x07,       // bLength
    0x05,       // bDescriptorType (Endpoint)
    0x81,       // bEndpointAddress (IN/D2H)
    0x03,       // bmAttributes (Interrupt)
    0x40, 0x00, // wMaxPacketSize 64
    0x08,       // bInterval 8 (8ms polling)

    0x07,       // bLength
    0x05,       // bDescriptorType (Endpoint)
    0x01,       // bEndpointAddress (OUT/H2D)
    0x03,       // bmAttributes (Interrupt)
    0x40, 0x00, // wMaxPacketSize 64
    0x08,       // bInterval 8 (8ms polling)
};

// USB HID Report Descriptor (203 bytes)
static const uint8_t switch_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x15, 0x00,        // Logical Minimum (0)
    0x09, 0x04,        // Usage (Joystick)
    0xA1, 0x01,        // Collection (Application)

    0x85, 0x30,        //   Report ID (48)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x0A,        //   Usage Maximum (0x0A)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0A,        //   Report Count (10)
    0x55, 0x00,        //   Unit Exponent (0)
    0x65, 0x00,        //   Unit (None)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x0B,        //   Usage Minimum (0x0B)
    0x29, 0x0E,        //   Usage Maximum (0x0E)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x03,        //   Input (Const,Var,Abs)
    0x0B, 0x01, 0x00, 0x01, 0x00,  //   Usage (0x010001)
    0xA1, 0x00,        //   Collection (Physical)
    0x0B, 0x30, 0x00, 0x01, 0x00,  //     Usage (0x010030)
    0x0B, 0x31, 0x00, 0x01, 0x00,  //     Usage (0x010031)
    0x0B, 0x32, 0x00, 0x01, 0x00,  //     Usage (0x010032)
    0x0B, 0x35, 0x00, 0x01, 0x00,  //     Usage (0x010035)
    0x15, 0x00,        //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x04,        //     Report Count (4)
    0x81, 0x02,        //     Input (Data,Var,Abs)
    0xC0,              //   End Collection
    0x0B, 0x39, 0x00, 0x01, 0x00,  //   Usage (0x010039)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x07,        //   Logical Maximum (7)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x65, 0x14,        //   Unit (Degrees)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x0F,        //   Usage Minimum (0x0F)
    0x29, 0x12,        //   Usage Maximum (0x12)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x34,        //   Report Count (52)
    0x81, 0x03,        //   Input (Const,Var,Abs)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)

    0x85, 0x21,        //   Report ID (33)
    0x09, 0x01,        //   Usage (0x01)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x81, 0x03,        //   Input (Const,Var,Abs)

    0x85, 0x81,        //   Report ID (129)
    0x09, 0x02,        //   Usage (0x02)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x81, 0x03,        //   Input (Const,Var,Abs)

    0x85, 0x01,        //   Report ID (1)
    0x09, 0x03,        //   Usage (0x03)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

    0x85, 0x10,        //   Report ID (16)
    0x09, 0x04,        //   Usage (0x04)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

    0x85, 0x80,        //   Report ID (128)
    0x09, 0x05,        //   Usage (0x05)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

    0x85, 0x82,        //   Report ID (130)
    0x09, 0x06,        //   Usage (0x06)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x91, 0x83,        //   Output (Const,Var,Abs,Volatile)

    0xC0,              // End Collection
};

#endif /* _SWITCH_DESCRIPTORS_H_ */
