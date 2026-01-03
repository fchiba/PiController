/*
 * Nintendo Switch Pro Controller Report Structures
 * Based on GP2040-CE SwitchProDescriptors
 * SPDX-License-Identifier: MIT
 */

#ifndef _SWITCH_PRO_REPORT_H_
#define _SWITCH_PRO_REPORT_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

//--------------------------------------------------------------------+
// Constants
//--------------------------------------------------------------------+

#define SWITCH_PRO_ENDPOINT_SIZE 64
#define SWITCH_PRO_KEEPALIVE_TIMER_MS 5

// Joystick values (16-bit for Pro Controller)
#define SWITCH_PRO_JOYSTICK_MIN 0x0000
#define SWITCH_PRO_JOYSTICK_MID 0x7FFF
#define SWITCH_PRO_JOYSTICK_MAX 0xFFFF

//--------------------------------------------------------------------+
// Report IDs
//--------------------------------------------------------------------+

typedef enum {
    REPORT_OUTPUT_00 = 0x00,
    REPORT_FEATURE = 0x01,
    REPORT_OUTPUT_10 = 0x10,
    REPORT_OUTPUT_21 = 0x21,
    REPORT_OUTPUT_30 = 0x30,
    REPORT_CONFIGURATION = 0x80,
    REPORT_USB_INPUT_81 = 0x81,
} SwitchReportID;

//--------------------------------------------------------------------+
// Configuration Report Subtypes (Report ID 0x80)
//--------------------------------------------------------------------+

typedef enum {
    SWITCH_SUBTYPE_IDENTIFY = 0x01,
    SWITCH_SUBTYPE_HANDSHAKE = 0x02,
    SWITCH_SUBTYPE_BAUD_RATE = 0x03,
    SWITCH_SUBTYPE_DISABLE_USB_TIMEOUT = 0x04,
    SWITCH_SUBTYPE_ENABLE_USB_TIMEOUT = 0x05,
} SwitchOutputSubtypes;

//--------------------------------------------------------------------+
// Feature Commands (Report ID 0x01)
//--------------------------------------------------------------------+

typedef enum {
    SWITCH_CMD_GET_CONTROLLER_STATE = 0x00,
    SWITCH_CMD_BLUETOOTH_PAIR_REQUEST = 0x01,
    SWITCH_CMD_REQUEST_DEVICE_INFO = 0x02,
    SWITCH_CMD_SET_MODE = 0x03,
    SWITCH_CMD_TRIGGER_BUTTONS = 0x04,
    SWITCH_CMD_SET_SHIPMENT = 0x08,
    SWITCH_CMD_SPI_READ = 0x10,
    SWITCH_CMD_SET_NFC_IR_CONFIG = 0x21,
    SWITCH_CMD_SET_NFC_IR_STATE = 0x22,
    SWITCH_CMD_SET_PLAYER_LIGHTS = 0x30,
    SWITCH_CMD_GET_PLAYER_LIGHTS = 0x31,
    SWITCH_CMD_COMMAND_UNKNOWN_33 = 0x33,
    SWITCH_CMD_SET_HOME_LIGHT = 0x38,
    SWITCH_CMD_TOGGLE_IMU = 0x40,
    SWITCH_CMD_IMU_SENSITIVITY = 0x41,
    SWITCH_CMD_READ_IMU = 0x43,
    SWITCH_CMD_ENABLE_VIBRATION = 0x48,
    SWITCH_CMD_GET_VOLTAGE = 0x50,
} SwitchCommands;

//--------------------------------------------------------------------+
// Controller Types
//--------------------------------------------------------------------+

typedef enum {
    SWITCH_TYPE_LEFT_JOYCON = 0x01,
    SWITCH_TYPE_RIGHT_JOYCON = 0x02,
    SWITCH_TYPE_PRO_CONTROLLER = 0x03,
} SwitchControllerType;

//--------------------------------------------------------------------+
// Analog Stick (12-bit X/Y packed in 3 bytes)
//--------------------------------------------------------------------+

typedef struct {
    uint8_t data[3];
} SwitchAnalog;

static inline void switch_analog_set_x(SwitchAnalog *analog, uint16_t x) {
    analog->data[0] = x & 0xFF;
    analog->data[1] = (analog->data[1] & 0xF0) | ((x >> 8) & 0x0F);
}

static inline void switch_analog_set_y(SwitchAnalog *analog, uint16_t y) {
    analog->data[1] = (analog->data[1] & 0x0F) | ((y & 0x0F) << 4);
    analog->data[2] = (y >> 4) & 0xFF;
}

static inline uint16_t switch_analog_get_x(const SwitchAnalog *analog) {
    return (uint16_t)(analog->data[0]) | ((analog->data[1] & 0x0F) << 8);
}

static inline uint16_t switch_analog_get_y(const SwitchAnalog *analog) {
    return (uint16_t)((analog->data[1] >> 4)) | (analog->data[2] << 4);
}

//--------------------------------------------------------------------+
// Input Report (9 bytes of button/stick data)
//--------------------------------------------------------------------+

typedef struct __attribute__((packed)) {
    // Byte 0: Battery/Connection info
    uint8_t connectionInfo : 4;
    uint8_t batteryLevel : 4;

    // Byte 1: Right side buttons
    uint8_t buttonY : 1;
    uint8_t buttonX : 1;
    uint8_t buttonB : 1;
    uint8_t buttonA : 1;
    uint8_t buttonRightSR : 1;
    uint8_t buttonRightSL : 1;
    uint8_t buttonR : 1;
    uint8_t buttonZR : 1;

    // Byte 2: System buttons
    uint8_t buttonMinus : 1;
    uint8_t buttonPlus : 1;
    uint8_t buttonThumbR : 1;
    uint8_t buttonThumbL : 1;
    uint8_t buttonHome : 1;
    uint8_t buttonCapture : 1;
    uint8_t dummy : 1;
    uint8_t chargingGrip : 1;

    // Byte 3: Left side buttons + D-pad
    uint8_t dpadDown : 1;
    uint8_t dpadUp : 1;
    uint8_t dpadRight : 1;
    uint8_t dpadLeft : 1;
    uint8_t buttonLeftSL : 1;
    uint8_t buttonLeftSR : 1;
    uint8_t buttonL : 1;
    uint8_t buttonZL : 1;

    // Bytes 4-6: Left stick (12-bit X/Y packed)
    SwitchAnalog leftStick;

    // Bytes 7-9: Right stick (12-bit X/Y packed)
    SwitchAnalog rightStick;
} SwitchInputReport;

//--------------------------------------------------------------------+
// Full Pro Controller Report (64 bytes, Report ID 0x30)
//--------------------------------------------------------------------+

typedef struct __attribute__((packed)) {
    uint8_t reportID;           // 0x30
    uint8_t timestamp;          // 0-255 rolling counter
    SwitchInputReport inputs;   // 10 bytes
    uint8_t rumbleReport;       // 1 byte
    uint8_t imuData[36];        // IMU data (zeros for us)
    uint8_t padding[15];        // Padding to 64 bytes
} SwitchProReport;

//--------------------------------------------------------------------+
// Device Info (for REQUEST_DEVICE_INFO response)
//--------------------------------------------------------------------+

typedef struct __attribute__((packed)) {
    uint8_t majorVersion;
    uint8_t minorVersion;
    uint8_t controllerType;
    uint8_t unknown00;
    uint8_t macAddress[6];
    uint8_t unknown01;
    uint8_t storedColors;
} SwitchDeviceInfo;

//--------------------------------------------------------------------+
// SPI Flash Data Structures (for SPI_READ emulation)
//--------------------------------------------------------------------+

// Factory calibration data at 0x6020 (left stick) and 0x603D (right stick)
typedef struct __attribute__((packed)) {
    uint8_t data[9];
} SwitchStickCalibration;

// Color definition
typedef struct __attribute__((packed)) {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} SwitchColorDefinition;

//--------------------------------------------------------------------+
// SPI Flash Address Map
//--------------------------------------------------------------------+

#define SPI_FLASH_SERIAL_NUMBER     0x6000
#define SPI_FLASH_CONTROLLER_COLOR  0x6050
#define SPI_FLASH_FACTORY_PARAMS    0x6080
#define SPI_FLASH_LEFT_STICK_CALIB  0x6020
#define SPI_FLASH_RIGHT_STICK_CALIB 0x603D
#define SPI_FLASH_MOTION_CALIB      0x6020
#define SPI_FLASH_USER_CALIB        0x8010
#define SPI_FLASH_USER_LEFT_STICK   0x8012
#define SPI_FLASH_USER_RIGHT_STICK  0x801D

//--------------------------------------------------------------------+
// Default SPI Flash Data
//--------------------------------------------------------------------+

// Serial number (16 bytes at 0x6000)
static const uint8_t spi_serial_number[16] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Controller color (12 bytes at 0x6050)
static const uint8_t spi_controller_color[12] = {
    0x32, 0x32, 0x32,  // Body color (dark gray)
    0xff, 0xff, 0xff,  // Button color (white)
    0x32, 0x32, 0x32,  // Left grip color
    0x32, 0x32, 0x32   // Right grip color
};

// Factory stick parameters (18 bytes at 0x6080)
static const uint8_t spi_factory_params[18] = {
    0x50, 0xfd, 0x00, 0x00, 0xc6, 0x0f,
    0x0f, 0x30, 0x61, 0x96, 0x30, 0xf3,
    0xd4, 0x14, 0x54, 0x41, 0x15, 0x54
};

// Factory stick calibration (left: 9 bytes at 0x603D, right: 9 bytes at 0x6046)
static const uint8_t spi_factory_stick_calib[18] = {
    // Left stick calibration
    0x00, 0x07, 0x70, 0x00, 0x08, 0x80, 0x00, 0x07, 0x70,
    // Right stick calibration
    0x00, 0x08, 0x80, 0x00, 0x07, 0x70, 0x00, 0x07, 0x70
};

// User calibration (magic + left + magic + right = 22 bytes at 0x8010)
static const uint8_t spi_user_calib[22] = {
    0xff, 0xff,  // Left magic (no user calibration)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Left stick
    0xff, 0xff,  // Right magic (no user calibration)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // Right stick
};

//--------------------------------------------------------------------+
// Legacy report structure (for macro compatibility)
//--------------------------------------------------------------------+

typedef struct {
    uint16_t buttons;
    uint8_t hat;
    uint8_t lx;
    uint8_t ly;
    uint8_t rx;
    uint8_t ry;
} SwitchOutReport;

// Button masks for legacy format
#define SWITCH_MASK_Y       (1U << 0)
#define SWITCH_MASK_B       (1U << 1)
#define SWITCH_MASK_A       (1U << 2)
#define SWITCH_MASK_X       (1U << 3)
#define SWITCH_MASK_L       (1U << 4)
#define SWITCH_MASK_R       (1U << 5)
#define SWITCH_MASK_ZL      (1U << 6)
#define SWITCH_MASK_ZR      (1U << 7)
#define SWITCH_MASK_MINUS   (1U << 8)
#define SWITCH_MASK_PLUS    (1U << 9)
#define SWITCH_MASK_L3      (1U << 10)
#define SWITCH_MASK_R3      (1U << 11)
#define SWITCH_MASK_HOME    (1U << 12)
#define SWITCH_MASK_CAPTURE (1U << 13)

// HAT values for legacy format
#define SWITCH_HAT_UP        0x00
#define SWITCH_HAT_UPRIGHT   0x01
#define SWITCH_HAT_RIGHT     0x02
#define SWITCH_HAT_DOWNRIGHT 0x03
#define SWITCH_HAT_DOWN      0x04
#define SWITCH_HAT_DOWNLEFT  0x05
#define SWITCH_HAT_LEFT      0x06
#define SWITCH_HAT_UPLEFT    0x07
#define SWITCH_HAT_NOTHING   0x08

// Legacy joystick values (8-bit)
#define SWITCH_JOYSTICK_MIN 0x00
#define SWITCH_JOYSTICK_MID 0x80
#define SWITCH_JOYSTICK_MAX 0xFF

#endif /* _SWITCH_PRO_REPORT_H_ */
