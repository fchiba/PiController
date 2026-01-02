# picontroller2 - Bluetooth Gamepad to Nintendo Switch USB HID Adapter
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Bluepad32 configuration
set(BLUEPAD32_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/bluepad32)
set(BTSTACK_ROOT ${PICO_SDK_PATH}/lib/btstack)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(picontroller2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Source files
add_executable(picontroller2
    src/main.c
    src/switch_platform.c
    src/usb_task.c
    src/usb_descriptors.c
    src/report.c
)

# Include directories for this target
target_include_directories(picontroller2 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${BLUEPAD32_ROOT}/src/components/bluepad32/include
)

# Global includes for bluepad32 to find btstack_config.h / sdkconfig.h
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)

# BTstack includes (needed for some IDEs)
include_directories(${BTSTACK_ROOT}/src)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)

# Link libraries
target_link_libraries(picontroller2 PUBLIC
    pico_stdlib
    pico_cyw43_arch_none
    pico_btstack_classic
    pico_btstack_cyw43
    bluepad32
    tinyusb_device
    tinyusb_board
    pico_multicore
)

# Add bluepad32 as subdirectory
add_subdirectory(${BLUEPAD32_ROOT}/src/components/bluepad32 libbluepad32)

# Disable stdio over USB (USB is used for HID)
# Enable UART for debug output
pico_enable_stdio_usb(picontroller2 0)
pico_enable_stdio_uart(picontroller2 1)

pico_set_program_name(picontroller2 "picontroller2")
pico_set_program_version(picontroller2 "1.0")

# Create map/bin/hex/uf2 file in addition to ELF
pico_add_extra_outputs(picontroller2)
