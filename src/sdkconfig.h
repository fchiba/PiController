// Bluepad32 SDK configuration for picontroller2
// Emulates ESP-IDF menuconfig

// Single controller support (simplified from 4)
#define CONFIG_BLUEPAD32_MAX_DEVICES 1
#define CONFIG_BLUEPAD32_MAX_ALLOWLIST 1

// Security and BLE settings
#define CONFIG_BLUEPAD32_GAP_SECURITY 1
#define CONFIG_BLUEPAD32_ENABLE_BLE_BY_DEFAULT 1

// Custom platform for Switch controller output
#define CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#define CONFIG_TARGET_PICO_W

// Log level: 2 = Info
#define CONFIG_BLUEPAD32_LOG_LEVEL 2
