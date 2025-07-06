#pragma once

// Platform-specific integer type definitions
// This file dispatches to the appropriate platform-specific int.h file

#if defined(ESP32)
    #include "platforms/esp/int.h"
#elif defined(__AVR__)
    #include "platforms/avr/int.h"
#elif defined(__SAM3X8E__)
    // Arduino Due (SAM3X8E Cortex-M3)
    #include "platforms/arm/sam/int.h"
#elif defined(__MK20DX128__) || defined(__MK20DX256__)
    // Teensy 3.0 / 3.1 (MK20DX128 / MK20DX256)
    #include "platforms/arm/k20/int.h"
#elif defined(__MKL26Z64__)
    // Teensy LC (MKL26Z64 Cortex-M0+)
    #include "platforms/arm/kl26/int.h"
#elif defined(__IMXRT1062__)
    // Teensy 4.0 / 4.1 (iMXRT1062 Cortex-M7)
    #include "platforms/arm/mxrt1062/int.h"
#elif defined(ARDUINO_ARCH_RENESAS_UNO)
    // Arduino UNO R4 WiFi (Renesas RA4M1)
    #include "platforms/arm/renesas/int.h"
#elif defined(STM32F1)
    // STM32F1 (Maple Mini and similar)
    #include "platforms/arm/stm32/int.h"
#elif defined(ARDUINO_GIGA) || defined(ARDUINO_GIGA_M7)
    // Arduino GIGA R1 (STM32H747)
    #include "platforms/arm/giga/int.h"
#elif defined(NRF52_SERIES) || defined(ARDUINO_ARCH_NRF52) || defined(NRF52840_XXAA)
    // Nordic nRF52 family - shared implementation
    #include "platforms/arm/nrf52/int.h"
#elif defined(__EMSCRIPTEN__)
    // WebAssembly / Emscripten
    #include "platforms/wasm/int.h"
#else
    // Default platform (desktop/generic)
    #include "platforms/shared/int.h"
#endif
