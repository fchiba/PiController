/**
 * USB Debug Logging Implementation
 */

#include "usb_debug.h"

#if USB_DEBUG_HEXDUMP

void usb_debug_hexdump(const char *label, const uint8_t *data, size_t len) {
    if (!data || len == 0) {
        printf("USB_DUMP: [%s] (empty)\n", label);
        return;
    }

    printf("USB_DUMP: [%s] %zu bytes:\n", label, len);

    for (size_t i = 0; i < len; i += 16) {
        // Print offset
        printf("  %04zx: ", i);

        // Print hex values
        for (size_t j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%02x ", data[i + j]);
            } else {
                printf("   ");
            }
            // Add extra space in the middle
            if (j == 7) {
                printf(" ");
            }
        }

        // Print ASCII representation
        printf(" |");
        for (size_t j = 0; j < 16 && i + j < len; j++) {
            uint8_t c = data[i + j];
            printf("%c", (c >= 0x20 && c < 0x7f) ? c : '.');
        }
        printf("|\n");
    }
}

#else

// Empty implementation when debug is disabled
void usb_debug_hexdump(const char *label, const uint8_t *data, size_t len) {
    (void)label;
    (void)data;
    (void)len;
}

#endif
