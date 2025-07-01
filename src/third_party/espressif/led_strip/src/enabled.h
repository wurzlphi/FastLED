#pragma once

#include "sdkconfig.h"
#include "platforms/esp/esp_version.h"

#if CONFIG_IDF_TARGET_ESP32C2
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 0
#define FASTLED_ESP32_HAS_RMT5 0
#elif CONFIG_IDF_TARGET_ESP32C3
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#elif CONFIG_IDF_TARGET_ESP32C6
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#elif CONFIG_IDF_TARGET_ESP32S2
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#elif CONFIG_IDF_TARGET_ESP32S3
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#elif CONFIG_IDF_TARGET_ESP32H2
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#elif CONFIG_IDF_TARGET_ESP32P4
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#elif CONFIG_IDF_TARGET_ESP8266
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 0
#define FASTLED_ESP32_HAS_RMT 0
#define FASTLED_ESP32_HAS_RMT5 0
#elif CONFIG_IDF_TARGET_ESP32 || defined(ARDUINO_ESP32_DEV)
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 1
#define FASTLED_ESP32_HAS_RMT 1
#define FASTLED_ESP32_HAS_RMT5 1
#else
#warning "Unknown board, assuming support for clockless RMT5 and SPI chipsets. Please file an bug report with FastLED and tell them about your board type."
#endif

// ESP32S3 and newer variants always use RMT5 - they are designed for it
// Only disable RMT5 for ESP32 original and pure ESP-IDF builds with version < 5.0.0
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2) || defined(CONFIG_IDF_TARGET_ESP32P4)
// Force RMT5 for these newer targets
#undef FASTLED_ESP32_HAS_RMT5
#define FASTLED_ESP32_HAS_RMT5 1
#elif ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0) && !defined(ARDUINO) && !defined(ARDUINO_ESP32_DEV) && !defined(PLATFORMIO)
// Only disable for pure ESP-IDF builds < 5.0.0 on older targets
#undef FASTLED_ESP32_HAS_RMT5
#undef FASTLED_ESP32_HAS_CLOCKLESS_SPI
#define FASTLED_ESP32_HAS_RMT5 0
#define FASTLED_ESP32_HAS_CLOCKLESS_SPI 0
#endif


// Note that FASTLED_RMT5 is a legacy name,
// so we keep it because "RMT" is specific to ESP32
#ifndef FASTLED_RMT5
#if FASTLED_ESP32_HAS_RMT5 && !defined(FASTLED_RMT5)
#define FASTLED_RMT5 1
#else
#define FASTLED_RMT5 0
#endif
#endif
