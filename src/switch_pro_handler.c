/*
 * Nintendo Switch Pro Controller Handler Implementation
 * Based on GP2040-CE SwitchProDriver
 * SPDX-License-Identifier: MIT
 */

#include "switch_pro_handler.h"
#include <stdio.h>
#include <string.h>
#include <pico/rand.h>
#include "usb_debug.h"

//--------------------------------------------------------------------+
// SPI Flash Read Emulation
//--------------------------------------------------------------------+

static void read_spi_flash(uint8_t *dest, uint32_t address, uint8_t size) {
    // Handle different SPI flash regions
    uint32_t bank = address & 0xFFFF00;
    uint32_t offset = address & 0x0000FF;

    // Default to 0xFF (empty flash)
    memset(dest, 0xFF, size);

    switch (bank) {
        case 0x6000:
            // Serial number and factory data region
            if (address >= SPI_FLASH_SERIAL_NUMBER &&
                address < SPI_FLASH_SERIAL_NUMBER + sizeof(spi_serial_number)) {
                uint32_t start = address - SPI_FLASH_SERIAL_NUMBER;
                uint8_t copy_size = size;
                if (start + copy_size > sizeof(spi_serial_number)) {
                    copy_size = sizeof(spi_serial_number) - start;
                }
                memcpy(dest, spi_serial_number + start, copy_size);
            }
            // Left stick calibration (0x603D)
            else if (address >= 0x603D && address < 0x603D + 9) {
                uint32_t start = address - 0x603D;
                uint8_t copy_size = (size < 9 - start) ? size : 9 - start;
                memcpy(dest, spi_factory_stick_calib + start, copy_size);
            }
            // Right stick calibration (0x6046)
            else if (address >= 0x6046 && address < 0x6046 + 9) {
                uint32_t start = address - 0x6046;
                uint8_t copy_size = (size < 9 - start) ? size : 9 - start;
                memcpy(dest, spi_factory_stick_calib + 9 + start, copy_size);
            }
            // Controller color (0x6050)
            else if (address >= SPI_FLASH_CONTROLLER_COLOR &&
                     address < SPI_FLASH_CONTROLLER_COLOR + sizeof(spi_controller_color)) {
                uint32_t start = address - SPI_FLASH_CONTROLLER_COLOR;
                uint8_t copy_size = size;
                if (start + copy_size > sizeof(spi_controller_color)) {
                    copy_size = sizeof(spi_controller_color) - start;
                }
                memcpy(dest, spi_controller_color + start, copy_size);
            }
            // Factory params (0x6080)
            else if (address >= SPI_FLASH_FACTORY_PARAMS &&
                     address < SPI_FLASH_FACTORY_PARAMS + sizeof(spi_factory_params)) {
                uint32_t start = address - SPI_FLASH_FACTORY_PARAMS;
                uint8_t copy_size = size;
                if (start + copy_size > sizeof(spi_factory_params)) {
                    copy_size = sizeof(spi_factory_params) - start;
                }
                memcpy(dest, spi_factory_params + start, copy_size);
            }
            break;

        case 0x8000:
            // User calibration region
            if (address >= SPI_FLASH_USER_CALIB &&
                address < SPI_FLASH_USER_CALIB + sizeof(spi_user_calib)) {
                uint32_t start = address - SPI_FLASH_USER_CALIB;
                uint8_t copy_size = size;
                if (start + copy_size > sizeof(spi_user_calib)) {
                    copy_size = sizeof(spi_user_calib) - start;
                }
                memcpy(dest, spi_user_calib + start, copy_size);
            }
            break;
    }
}

//--------------------------------------------------------------------+
// Configuration Report Handler (Report ID 0x80)
//--------------------------------------------------------------------+

static void handle_config_report(SwitchProHandler *handler,
                                  uint8_t subtype,
                                  const uint8_t *buffer,
                                  uint16_t bufsize) {
    memset(handler->responseBuffer, 0x00, 64);

    switch (subtype) {
        case SWITCH_SUBTYPE_IDENTIFY:
            printf("PRO: Config - IDENTIFY\n");
            switch_pro_handler_send_identify(handler);
            handler->isReportQueued = true;
            break;

        case SWITCH_SUBTYPE_HANDSHAKE:
            printf("PRO: Config - HANDSHAKE\n");
            handler->responseBuffer[0] = REPORT_USB_INPUT_81;
            handler->responseBuffer[1] = SWITCH_SUBTYPE_HANDSHAKE;
            handler->isReportQueued = true;
            break;

        case SWITCH_SUBTYPE_BAUD_RATE:
            printf("PRO: Config - BAUD_RATE\n");
            handler->responseBuffer[0] = REPORT_USB_INPUT_81;
            handler->responseBuffer[1] = SWITCH_SUBTYPE_BAUD_RATE;
            handler->isReportQueued = true;
            break;

        case SWITCH_SUBTYPE_DISABLE_USB_TIMEOUT:
            printf("PRO: Config - DISABLE_USB_TIMEOUT -> isReady\n");
            handler->responseBuffer[0] = REPORT_OUTPUT_30;
            handler->responseBuffer[1] = subtype;
            handler->isReady = true;
            handler->isReportQueued = true;
            break;

        case SWITCH_SUBTYPE_ENABLE_USB_TIMEOUT:
            printf("PRO: Config - ENABLE_USB_TIMEOUT\n");
            handler->responseBuffer[0] = REPORT_OUTPUT_30;
            handler->responseBuffer[1] = subtype;
            handler->isReportQueued = true;
            break;

        default:
            printf("PRO: Config - Unknown 0x%02x\n", subtype);
            handler->responseBuffer[0] = REPORT_OUTPUT_30;
            handler->responseBuffer[1] = subtype;
            handler->isReportQueued = true;
            break;
    }
}

//--------------------------------------------------------------------+
// Feature Report Handler (Report ID 0x01)
//--------------------------------------------------------------------+

static void handle_feature_report(SwitchProHandler *handler,
                                   const uint8_t *buffer,
                                   uint16_t bufsize) {
    uint8_t commandID = buffer[10];

    memset(handler->responseBuffer, 0x00, 64);

    // Build response header
    handler->responseBuffer[0] = REPORT_OUTPUT_21;
    handler->responseBuffer[1] = handler->reportCounter;

    // Copy current input state to response (bytes 2-11)
    memcpy(&handler->responseBuffer[2], &handler->inputReport.inputs, sizeof(SwitchInputReport));

    switch (commandID) {
        case SWITCH_CMD_GET_CONTROLLER_STATE:
            printf("PRO: Cmd - GET_CONTROLLER_STATE\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x03;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_BLUETOOTH_PAIR_REQUEST:
            printf("PRO: Cmd - BLUETOOTH_PAIR_REQUEST\n");
            handler->responseBuffer[13] = 0x81;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x03;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_REQUEST_DEVICE_INFO:
            printf("PRO: Cmd - REQUEST_DEVICE_INFO\n");
            handler->responseBuffer[13] = 0x82;
            handler->responseBuffer[14] = 0x02;
            memcpy(&handler->responseBuffer[15], &handler->deviceInfo, sizeof(SwitchDeviceInfo));
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_SET_MODE:
            handler->inputMode = buffer[11];
            printf("PRO: Cmd - SET_MODE: 0x%02x\n", handler->inputMode);
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = 0x03;
            handler->responseBuffer[15] = handler->inputMode;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_TRIGGER_BUTTONS:
            printf("PRO: Cmd - TRIGGER_BUTTONS\n");
            handler->responseBuffer[13] = 0x83;
            handler->responseBuffer[14] = 0x04;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_SET_SHIPMENT:
            printf("PRO: Cmd - SET_SHIPMENT\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_SPI_READ: {
            uint32_t spiAddress = (buffer[14] << 24) | (buffer[13] << 16) |
                                  (buffer[12] << 8) | buffer[11];
            uint8_t spiSize = buffer[15];
            printf("PRO: Cmd - SPI_READ @ 0x%08lx, size %d\n", spiAddress, spiSize);
            handler->responseBuffer[13] = 0x90;
            handler->responseBuffer[14] = buffer[10];  // Command ID
            handler->responseBuffer[15] = buffer[11];  // Address byte 0
            handler->responseBuffer[16] = buffer[12];  // Address byte 1
            handler->responseBuffer[17] = buffer[13];  // Address byte 2
            handler->responseBuffer[18] = buffer[14];  // Address byte 3
            handler->responseBuffer[19] = buffer[15];  // Size
            read_spi_flash(&handler->responseBuffer[20], spiAddress, spiSize);
            handler->isReportQueued = true;
            break;
        }

        case SWITCH_CMD_SET_NFC_IR_CONFIG:
            printf("PRO: Cmd - SET_NFC_IR_CONFIG\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_SET_NFC_IR_STATE:
            printf("PRO: Cmd - SET_NFC_IR_STATE\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_SET_PLAYER_LIGHTS:
            handler->playerID = buffer[11];
            printf("PRO: Cmd - SET_PLAYER_LIGHTS: %d\n", handler->playerID);
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_GET_PLAYER_LIGHTS:
            printf("PRO: Cmd - GET_PLAYER_LIGHTS\n");
            handler->responseBuffer[13] = 0xB0;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = handler->playerID;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_COMMAND_UNKNOWN_33:
            printf("PRO: Cmd - UNKNOWN_33 (Chromium)\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x03;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_SET_HOME_LIGHT:
            printf("PRO: Cmd - SET_HOME_LIGHT\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x00;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_TOGGLE_IMU:
            handler->imuEnabled = buffer[11];
            printf("PRO: Cmd - TOGGLE_IMU: %d\n", handler->imuEnabled);
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x00;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_IMU_SENSITIVITY:
            printf("PRO: Cmd - IMU_SENSITIVITY\n");
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_READ_IMU:
            printf("PRO: Cmd - READ_IMU\n");
            handler->responseBuffer[13] = 0xC0;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = buffer[11];
            handler->responseBuffer[16] = buffer[12];
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_ENABLE_VIBRATION:
            handler->vibrationEnabled = buffer[11];
            printf("PRO: Cmd - ENABLE_VIBRATION: %d\n", handler->vibrationEnabled);
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x00;
            handler->isReportQueued = true;
            break;

        case SWITCH_CMD_GET_VOLTAGE:
            printf("PRO: Cmd - GET_VOLTAGE\n");
            handler->responseBuffer[13] = 0xD0;
            handler->responseBuffer[14] = 0x50;
            handler->responseBuffer[15] = 0x83;
            handler->responseBuffer[16] = 0x06;
            handler->isReportQueued = true;
            break;

        default:
            printf("PRO: Cmd - Unknown 0x%02x\n", commandID);
            handler->responseBuffer[13] = 0x80;
            handler->responseBuffer[14] = commandID;
            handler->responseBuffer[15] = 0x03;
            handler->isReportQueued = true;
            break;
    }
}

//--------------------------------------------------------------------+
// Public API Implementation
//--------------------------------------------------------------------+

void switch_pro_handler_init(SwitchProHandler *handler) {
    memset(handler, 0, sizeof(SwitchProHandler));

    // Initialize device info
    handler->deviceInfo.majorVersion = 0x04;
    handler->deviceInfo.minorVersion = 0x91;
    handler->deviceInfo.controllerType = SWITCH_TYPE_PRO_CONTROLLER;
    handler->deviceInfo.unknown00 = 0x02;
    // Random MAC address
    handler->deviceInfo.macAddress[0] = 0x7c;
    handler->deviceInfo.macAddress[1] = 0xbb;
    handler->deviceInfo.macAddress[2] = 0x8a;
    handler->deviceInfo.macAddress[3] = (uint8_t)(get_rand_32() & 0xFF);
    handler->deviceInfo.macAddress[4] = (uint8_t)(get_rand_32() & 0xFF);
    handler->deviceInfo.macAddress[5] = (uint8_t)(get_rand_32() & 0xFF);
    handler->deviceInfo.unknown01 = 0x01;
    handler->deviceInfo.storedColors = 0x02;

    // Initialize input report
    handler->inputReport.reportID = 0x30;
    handler->inputReport.timestamp = 0;
    handler->inputReport.inputs.connectionInfo = 0;
    handler->inputReport.inputs.batteryLevel = 0x08;  // Full battery
    handler->inputReport.inputs.chargingGrip = 1;

    // Center sticks (0x7FF for 12-bit center)
    switch_analog_set_x(&handler->inputReport.inputs.leftStick, 0x7FF);
    switch_analog_set_y(&handler->inputReport.inputs.leftStick, 0x7FF);
    switch_analog_set_x(&handler->inputReport.inputs.rightStick, 0x7FF);
    switch_analog_set_y(&handler->inputReport.inputs.rightStick, 0x7FF);

    handler->inputReport.rumbleReport = 0x09;

    printf("PRO: Handler initialized\n");
}

void switch_pro_handler_send_identify(SwitchProHandler *handler) {
    memset(handler->responseBuffer, 0x00, 64);
    handler->responseBuffer[0] = REPORT_USB_INPUT_81;
    handler->responseBuffer[1] = SWITCH_SUBTYPE_IDENTIFY;
    handler->responseBuffer[2] = 0x00;
    handler->responseBuffer[3] = handler->deviceInfo.controllerType;
    // MAC address in reverse order
    for (uint8_t i = 0; i < 6; i++) {
        handler->responseBuffer[4 + i] = handler->deviceInfo.macAddress[5 - i];
    }
    handler->queuedReportID = 0;
}

void switch_pro_handler_set_report(SwitchProHandler *handler,
                                    uint8_t report_id,
                                    uint8_t report_type,
                                    const uint8_t *buffer,
                                    uint16_t bufsize) {
    // Only handle OUTPUT reports
    if (report_type != 0x02) {  // HID_REPORT_TYPE_OUTPUT
        return;
    }

    uint8_t switchReportID = buffer[0];
    uint8_t switchSubID = buffer[1];

    handler->queuedReportID = report_id;

    if (switchReportID == REPORT_FEATURE) {
        handle_feature_report(handler, buffer, bufsize);
    } else if (switchReportID == REPORT_CONFIGURATION) {
        handle_config_report(handler, switchSubID, buffer, bufsize);
    } else if (switchReportID == REPORT_OUTPUT_10) {
        // Rumble data - silently ignore (no response needed)
    } else if (switchReportID == REPORT_OUTPUT_00) {
        // Simple rumble - silently ignore
    } else {
        printf("PRO: Unknown report ID 0x%02x\n", switchReportID);
    }
}

uint16_t switch_pro_handler_get_report(SwitchProHandler *handler,
                                        uint8_t report_id,
                                        uint8_t report_type,
                                        uint8_t *buffer,
                                        uint16_t reqlen) {
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    return 0;
}

bool switch_pro_handler_has_response(SwitchProHandler *handler) {
    return handler->isReportQueued;
}

bool switch_pro_handler_get_response(SwitchProHandler *handler,
                                      uint8_t *buffer,
                                      uint16_t *size,
                                      uint8_t *report_id) {
    if (!handler->isReportQueued) {
        return false;
    }

    memcpy(buffer, handler->responseBuffer, 64);
    *size = 64;
    *report_id = handler->queuedReportID;
    handler->isReportQueued = false;

    USB_DBG_XFER("SEND response report_id=0x%02x", *report_id);
#if USB_DEBUG_HEXDUMP
    usb_debug_hexdump("SEND response", buffer, 64);
#endif

    return true;
}

void switch_pro_handler_update_input(SwitchProHandler *handler,
                                      const SwitchOutReport *legacy_report) {
    SwitchInputReport *inputs = &handler->inputReport.inputs;

    // Clear button state
    inputs->buttonY = 0;
    inputs->buttonX = 0;
    inputs->buttonB = 0;
    inputs->buttonA = 0;
    inputs->buttonR = 0;
    inputs->buttonZR = 0;
    inputs->buttonMinus = 0;
    inputs->buttonPlus = 0;
    inputs->buttonThumbR = 0;
    inputs->buttonThumbL = 0;
    inputs->buttonHome = 0;
    inputs->buttonCapture = 0;
    inputs->buttonL = 0;
    inputs->buttonZL = 0;

    // Map buttons from legacy format
    if (legacy_report->buttons & SWITCH_MASK_Y) inputs->buttonY = 1;
    if (legacy_report->buttons & SWITCH_MASK_X) inputs->buttonX = 1;
    if (legacy_report->buttons & SWITCH_MASK_B) inputs->buttonB = 1;
    if (legacy_report->buttons & SWITCH_MASK_A) inputs->buttonA = 1;
    if (legacy_report->buttons & SWITCH_MASK_L) inputs->buttonL = 1;
    if (legacy_report->buttons & SWITCH_MASK_R) inputs->buttonR = 1;
    if (legacy_report->buttons & SWITCH_MASK_ZL) inputs->buttonZL = 1;
    if (legacy_report->buttons & SWITCH_MASK_ZR) inputs->buttonZR = 1;
    if (legacy_report->buttons & SWITCH_MASK_MINUS) inputs->buttonMinus = 1;
    if (legacy_report->buttons & SWITCH_MASK_PLUS) inputs->buttonPlus = 1;
    if (legacy_report->buttons & SWITCH_MASK_L3) inputs->buttonThumbL = 1;
    if (legacy_report->buttons & SWITCH_MASK_R3) inputs->buttonThumbR = 1;
    if (legacy_report->buttons & SWITCH_MASK_HOME) inputs->buttonHome = 1;
    if (legacy_report->buttons & SWITCH_MASK_CAPTURE) inputs->buttonCapture = 1;

    // Map D-pad from HAT to individual buttons
    inputs->dpadUp = 0;
    inputs->dpadDown = 0;
    inputs->dpadLeft = 0;
    inputs->dpadRight = 0;

    switch (legacy_report->hat) {
        case SWITCH_HAT_UP:
            inputs->dpadUp = 1;
            break;
        case SWITCH_HAT_UPRIGHT:
            inputs->dpadUp = 1;
            inputs->dpadRight = 1;
            break;
        case SWITCH_HAT_RIGHT:
            inputs->dpadRight = 1;
            break;
        case SWITCH_HAT_DOWNRIGHT:
            inputs->dpadDown = 1;
            inputs->dpadRight = 1;
            break;
        case SWITCH_HAT_DOWN:
            inputs->dpadDown = 1;
            break;
        case SWITCH_HAT_DOWNLEFT:
            inputs->dpadDown = 1;
            inputs->dpadLeft = 1;
            break;
        case SWITCH_HAT_LEFT:
            inputs->dpadLeft = 1;
            break;
        case SWITCH_HAT_UPLEFT:
            inputs->dpadUp = 1;
            inputs->dpadLeft = 1;
            break;
    }

    // Map analog sticks: 8-bit (0-255) to 12-bit (0-4095)
    // Legacy mid = 0x80 (128), Pro mid = 0x7FF (2047)
    uint16_t lx = (uint16_t)legacy_report->lx * 16;  // 0-255 -> 0-4080
    uint16_t ly = (uint16_t)legacy_report->ly * 16;
    uint16_t rx = (uint16_t)legacy_report->rx * 16;
    uint16_t ry = (uint16_t)legacy_report->ry * 16;

    // Clamp to 12-bit max
    if (lx > 0xFFF) lx = 0xFFF;
    if (ly > 0xFFF) ly = 0xFFF;
    if (rx > 0xFFF) rx = 0xFFF;
    if (ry > 0xFFF) ry = 0xFFF;

    // Pro Controller Y axis is inverted
    ly = 0xFFF - ly;
    ry = 0xFFF - ry;

    switch_analog_set_x(&inputs->leftStick, lx);
    switch_analog_set_y(&inputs->leftStick, ly);
    switch_analog_set_x(&inputs->rightStick, rx);
    switch_analog_set_y(&inputs->rightStick, ry);

    // Update timestamp
    handler->inputReport.timestamp = handler->reportCounter;
}

const SwitchProReport *switch_pro_handler_get_input_report(SwitchProHandler *handler) {
    return &handler->inputReport;
}

void switch_pro_handler_increment_counter(SwitchProHandler *handler) {
    handler->reportCounter++;
}
