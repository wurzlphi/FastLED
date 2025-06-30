#pragma once

#include "crgb.h"
#include "fl/stdint.h"
#include "fl/ease.h"

namespace fl {

inline void gamma16(const CRGB &rgb, uint16_t* r16, uint16_t* g16, uint16_t* b16) {
    // Use the gamma 2.8 lookup function from ease.h which stores the table in PROGMEM
    *r16 = gamma2_8_lookup(rgb.r);
    *g16 = gamma2_8_lookup(rgb.g);
    *b16 = gamma2_8_lookup(rgb.b);
}

} // namespace fl
