#pragma once

#include "crgb.h"
#include "fl/namespace.h"
#include "fl/stdint.h"
#include "fl/int.h"

namespace fl {

// Abstract base class for effects on a strip/grid of LEDs.

struct _DrawContext {
    uint32_t now;
    CRGB *leds;
    fl::u16 frame_time = 0;
    float speed = 1.0f;
    _DrawContext(uint32_t now, CRGB *leds, fl::u16 frame_time = 0,
                 float speed = 1.0f)
        : now(now), leds(leds), frame_time(frame_time), speed(speed) {}
};

} // namespace fl
