/*
 * Nintendo Switch Pro Controller Handler
 * Handles handshake, commands, and SPI flash emulation
 * SPDX-License-Identifier: MIT
 */

#ifndef _SWITCH_PRO_HANDLER_H_
#define _SWITCH_PRO_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>
#include "switch_pro_report.h"

//--------------------------------------------------------------------+
// Handler State
//--------------------------------------------------------------------+

typedef struct {
    // State flags
    bool isInitialized;     // Initial IDENTIFY sent
    bool isReady;           // Handshake complete, ready for input
    bool isReportQueued;    // Response report waiting to be sent

    // Device info
    SwitchDeviceInfo deviceInfo;

    // Report counter
    uint8_t reportCounter;

    // Player ID (set by SET_PLAYER_LIGHTS)
    uint8_t playerID;

    // Input mode (set by SET_MODE)
    uint8_t inputMode;

    // IMU/Vibration enabled
    bool imuEnabled;
    bool vibrationEnabled;

    // Current input report
    SwitchProReport inputReport;

    // Response buffer (64 bytes)
    uint8_t responseBuffer[64];
    uint8_t queuedReportID;

} SwitchProHandler;

//--------------------------------------------------------------------+
// API Functions
//--------------------------------------------------------------------+

// Initialize the handler
void switch_pro_handler_init(SwitchProHandler *handler);

// Process incoming SET_REPORT (called from tud_hid_set_report_cb)
void switch_pro_handler_set_report(SwitchProHandler *handler,
                                    uint8_t report_id,
                                    uint8_t report_type,
                                    const uint8_t *buffer,
                                    uint16_t bufsize);

// Process incoming GET_REPORT (called from tud_hid_get_report_cb)
uint16_t switch_pro_handler_get_report(SwitchProHandler *handler,
                                        uint8_t report_id,
                                        uint8_t report_type,
                                        uint8_t *buffer,
                                        uint16_t reqlen);

// Prepare IDENTIFY report for initial handshake
void switch_pro_handler_send_identify(SwitchProHandler *handler);

// Check if handler has a queued response to send
bool switch_pro_handler_has_response(SwitchProHandler *handler);

// Get the queued response (also clears the queue)
bool switch_pro_handler_get_response(SwitchProHandler *handler,
                                      uint8_t *buffer,
                                      uint16_t *size,
                                      uint8_t *report_id);

// Update input report from legacy format
void switch_pro_handler_update_input(SwitchProHandler *handler,
                                      const SwitchOutReport *legacy_report);

// Get pointer to current input report
const SwitchProReport *switch_pro_handler_get_input_report(SwitchProHandler *handler);

// Increment report counter
void switch_pro_handler_increment_counter(SwitchProHandler *handler);

#endif /* _SWITCH_PRO_HANDLER_H_ */
