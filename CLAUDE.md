# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**picontroller2** is a Raspberry Pi Pico W / Pico 2 W controller application written in C that integrates Bluetooth gamepad support using the bluepad32 library. The project creates a wireless game controller interface on Raspberry Pi Pico microcontroller hardware.

- **Language:** C (C11 standard)
- **Platform:** Raspberry Pi Pico W / Pico 2 W (ARM Cortex-M0+)
- **Build System:** CMake + Ninja

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

```
picontroller2/
├── picontroller2.c           # Main application entry point
├── CMakeLists.txt            # Build configuration
├── pico_sdk_import.cmake     # Pico SDK integration
├── bluepad32/                # Git submodule: Bluetooth gamepad library
└── build/                    # Build artifacts (generated)
```

### Dependencies

- **Pico SDK v2.2.0** - Hardware abstraction layer, expected at `~/.pico-sdk/sdk/2.2.0`
- **bluepad32** - Bluetooth controller host library (submodule), supports DualSense, Xbox, Joy-Con, and other Bluetooth controllers
- **ARM GCC toolchain** - Cross-compiler at `~/.pico-sdk/toolchain/14_2_Rel1/bin/`

### Build Outputs

- `build/picontroller2.uf2` - Drag-and-drop flash format
- `build/picontroller2.elf` - Debuggable executable
- `build/picontroller2.hex` / `.bin` - Alternative flash formats

## Setup Notes

- Run `git submodule update --init --recursive` after cloning to fetch bluepad32
- The `build/` directory is git-ignored; regenerate with cmake/ninja after checkout
- VSCode workspace includes Pico extension configuration with debug support
