/**
 * USB Debug Logging
 *
 * Define USB_DEBUG_ENABLE to enable debug output.
 * Can be defined in CMakeLists.txt or before including this header.
 */

#ifndef USB_DEBUG_H
#define USB_DEBUG_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// Uncomment to enable debug output (or define in CMakeLists.txt)
// #define USB_DEBUG_ENABLE

#ifdef USB_DEBUG_ENABLE
  #define USB_DEBUG_DESCRIPTOR  1  // descriptor callback logging
  #define USB_DEBUG_STATE       1  // mount/suspend/resume logging
  #define USB_DEBUG_TRANSFER    1  // IN/OUT transfer logging
  #define USB_DEBUG_HEXDUMP     1  // hex dump of data
#else
  #define USB_DEBUG_DESCRIPTOR  0
  #define USB_DEBUG_STATE       0
  #define USB_DEBUG_TRANSFER    0
  #define USB_DEBUG_HEXDUMP     0
#endif

// General USB debug log
#ifdef USB_DEBUG_ENABLE
  #define USB_DBG(fmt, ...) printf("USB: " fmt "\n", ##__VA_ARGS__)
#else
  #define USB_DBG(fmt, ...) ((void)0)
#endif

// Descriptor callback logging
#if USB_DEBUG_DESCRIPTOR
  #define USB_DBG_DESC(fmt, ...) printf("USB_DESC: " fmt "\n", ##__VA_ARGS__)
#else
  #define USB_DBG_DESC(fmt, ...) ((void)0)
#endif

// USB state change logging
#if USB_DEBUG_STATE
  #define USB_DBG_STATE(fmt, ...) printf("USB_STATE: " fmt "\n", ##__VA_ARGS__)
#else
  #define USB_DBG_STATE(fmt, ...) ((void)0)
#endif

// HID transfer logging
#if USB_DEBUG_TRANSFER
  #define USB_DBG_XFER(fmt, ...) printf("USB_XFER: " fmt "\n", ##__VA_ARGS__)
#else
  #define USB_DBG_XFER(fmt, ...) ((void)0)
#endif

/**
 * Print hex dump of data buffer
 *
 * @param label Description label for the dump
 * @param data  Pointer to data buffer
 * @param len   Length of data in bytes
 */
void usb_debug_hexdump(const char *label, const uint8_t *data, size_t len);

#endif // USB_DEBUG_H
