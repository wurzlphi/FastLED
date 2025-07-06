#pragma once

// Nordic nRF52 family (e.g., nRF52832, nRF52840) – ARM Cortex-M4 / M33.
// The GNU ARM tool-chain used by both the Adafruit and Nordic cores aliases
// uint32_t to 'unsigned long', so we follow that mapping.
// Uses standard ARM 32-bit type mappings
#include "platforms/arm/shared_arm_int.h"
