#pragma once

/// @file lib8tion_base.h
/// Base definitions for lib8tion that avoid circular dependencies with FastLED.h

// Only include what we absolutely need
#include "fl/stdint.h"
#include "fl/namespace.h"
#include "lib8tion/types.h"
#include "lib8tion/config.h"
#include "fl/deprecated.h"

// Platform detection for timer functions
#if defined(ARDUINO) || defined(SPARK) || defined(FASTLED_HAS_MILLIS)
  #define FASTLED_LIB8TION_USE_MILLIS 1
  #if defined(ARDUINO) && !defined(ARDUINO_ARCH_RP2040) && !defined(ESP32) && !defined(ESP8266) && !defined(__AVR__) && !defined(FASTLED_STUB_IMPL)
    // For Arduino platforms that need Arduino.h for millis()
    #include <Arduino.h>
  #endif
#endif

// Forward declaration for platforms without millis()
#ifndef FASTLED_LIB8TION_USE_MILLIS
extern "C" {
  uint32_t get_millisecond_timer();
}
#endif

// Define GET_MILLIS without needing FastLED.h
#ifdef FASTLED_LIB8TION_USE_MILLIS
  #ifndef GET_MILLIS
    #define GET_MILLIS millis
  #endif
#else
  #ifndef GET_MILLIS
    #define GET_MILLIS get_millisecond_timer
  #endif
#endif

// Note: LIB8STATIC and LIB8STATIC_ALWAYS_INLINE are defined in lib8tion/lib8static.h
// which is included by the lib8tion sub-headers. We don't need to define them here.
