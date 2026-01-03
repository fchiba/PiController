# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**picontroller2** is a Bluetooth Gamepad to Nintendo Switch USB HID Adapter built on Raspberry Pi Pico W / Pico 2 W. It allows Bluetooth controllers (DualSense, Xbox, Joy-Con, etc.) to be used with Nintendo Switch by presenting as a HORI POKKEN Controller over USB.

- **Language:** C (C11 standard)
- **Platform:** Raspberry Pi Pico W / Pico 2 W (ARM Cortex-M0+ / M33)
- **Build System:** CMake + Ninja
- **USB Stack:** TinyUSB (HID device mode)
- **Bluetooth Stack:** BTstack + bluepad32

## Build Commands

```bash
# Configure (first time or after CMakeLists.txt changes)
cmake -G Ninja -B build -DPICO_BOARD=pico_w

# Build
ninja -C build

# Flash via USB (device must be in BOOTSEL mode or using picotool)
picotool load build/picontroller2.uf2
```

In VSCode:
- **Compile:** Run "Compile Project" task (Ctrl+Shift+B)
- **Flash:** Run "Run Project" task (loads via picotool)
- **Debug:** Use "Pico Debug (Cortex-Debug)" launch config with CMSIS-DAP probe

## Architecture

### Dual-Core Design

- **Core 0:** USB HID output (TinyUSB) - presents as HORI POKKEN Controller
- **Core 1:** Bluetooth input (bluepad32 + BTstack) - receives gamepad data

### Directory Structure

```
picontroller2/
├── src/
│   ├── main.c              # Entry point, launches dual-core tasks
│   ├── switch_platform.c   # Bluepad32 platform: BT input → Switch format
│   ├── usb_task.c          # USB HID task (Core 0), macro integration
│   ├── usb_descriptors.c   # TinyUSB descriptor callbacks
│   ├── report.c            # Thread-safe report sharing between cores
│   ├── macro.c             # Macro state machine (record/play)
│   ├── macro_flash.c       # Macro flash storage (5 slots, last 8KB)
│   ├── macro_led.c         # LED feedback patterns for macro state
│   ├── gpio_button.c       # GPIO button handling (GPIO10-15)
│   ├── btstack_config.h    # BTstack configuration
│   └── sdkconfig.h         # Bluepad32 configuration
├── include/
│   ├── switch_descriptors.h # Switch HID report format & USB descriptors
│   ├── report.h            # Report sharing API
│   ├── usb_task.h          # USB task API
│   ├── macro.h             # Macro system API
│   ├── macro_types.h       # Macro data structures and constants
│   ├── gpio_button.h       # GPIO button API
│   └── tusb_config.h       # TinyUSB configuration
├── CMakeLists.txt          # Build configuration
├── pico_sdk_import.cmake   # Pico SDK integration
├── bluepad32/              # Git submodule: Bluetooth gamepad library
└── build/                  # Build artifacts (generated)
```

### Key Components

- **switch_platform.c** - Bluepad32 custom platform that converts gamepad input to Switch HID format. Handles button remapping (A/B, X/Y swap for Nintendo layout), analog stick conversion, and deadzone filtering.
- **usb_descriptors.c** - TinyUSB callbacks providing HORI POKKEN USB descriptors (VID:0x0F0D PID:0x0092)
- **report.c** - Spin-lock protected report buffer for inter-core communication
- **macro.c** - Macro recording/playback state machine with frame thinning (100ms intervals)
- **macro_flash.c** - Persistent storage in last 8KB of flash, 5 slots of up to 100 frames each
- **gpio_button.c** - Debounced GPIO input handling for macro control buttons

### Dependencies

- **Pico SDK v2.2.0** - Hardware abstraction layer, expected at `~/.pico-sdk/sdk/2.2.0`
- **bluepad32** - Bluetooth controller host library (submodule), supports DualSense, Xbox, Joy-Con, and other Bluetooth controllers
- **TinyUSB** - USB device stack (included in Pico SDK)
- **BTstack** - Bluetooth stack (included in Pico SDK)
- **ARM GCC toolchain** - Cross-compiler at `~/.pico-sdk/toolchain/14_2_Rel1/bin/`

### Build Outputs

- `build/picontroller2.uf2` - Drag-and-drop flash format
- `build/picontroller2.elf` - Debuggable executable
- `build/picontroller2.hex` / `.bin` - Alternative flash formats

## Usage

1. Flash the firmware to Pico W
2. Connect Pico W USB to Nintendo Switch dock
3. Put Bluetooth controller in pairing mode
4. Pico W LED turns on when controller connects
5. Controller input is forwarded to Switch as POKKEN controller

Debug output available via UART (stdio_uart enabled).

## Macro Feature

The adapter supports recording and playing back controller input macros. Macros persist across power cycles (stored in flash).

### GPIO Button Wiring

| GPIO | Function                    |
|------|-----------------------------|
| 10   | Record modifier (hold)      |
| 11   | Macro slot 0                |
| 12   | Macro slot 1                |
| 13   | Macro slot 2                |
| 14   | Macro slot 3                |
| 15   | Macro slot 4                |
| 16   | Single mode modifier (hold) |
| 17   | Rapid mode modifier (hold)  |

All buttons use internal pull-up; connect to GND to activate.

### Recording a Macro

1. Hold GPIO10 (record button)
2. Press a slot button (GPIO11-15)
3. LED blinks fast (2.5Hz) during recording
4. Perform controller inputs (max 10 seconds, 100 frames)
5. Release GPIO10 to stop and save
6. LED flashes 3x on successful save

### Playing a Macro

1. Press a slot button (GPIO11-15) without GPIO10
2. LED blinks medium speed (1Hz) during playback
3. Macro input is merged with live controller input
4. Playback stops automatically when complete

### Rapid Fire Mode

Each slot can be set to rapid fire mode, which continuously repeats playback while the slot button is held.

1. Hold GPIO17 + press a slot button to enable rapid fire mode for that slot
2. Hold GPIO16 + press a slot button to return to single (normal) mode
3. Mode settings are stored in RAM (reset on power cycle)

In rapid fire mode:
- Press and hold the slot button to continuously repeat the macro
- Release the button to stop immediately

### LED Patterns

- **Solid on**: Controller connected, idle
- **Fast blink** (200ms): Recording in progress
- **Medium blink** (500ms): Playback in progress
- **3 quick flashes**: Save successful
- **1 second solid**: Save error

## Setup Notes

- Run `git submodule update --init --recursive` after cloning to fetch bluepad32
- The `build/` directory is git-ignored; regenerate with cmake/ninja after checkout
- VSCode workspace includes Pico extension configuration with debug support
