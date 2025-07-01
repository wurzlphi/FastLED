/// @file noisegen.h
/// Noise generation classes

#pragma once

#include "fl/stdint.h"
#include "fl/int.h"
#include "noise.h"

#include "fl/math_macros.h"
#include "fl/namespace.h"

FASTLED_NAMESPACE_BEGIN


// Simple noise generator for 1-d waves. Default values will give good results
// for most cases.
struct NoiseGenerator {
    int32_t iteration_scale;
    unsigned long time_multiplier;

    NoiseGenerator() : iteration_scale(10), time_multiplier(10) {}
    NoiseGenerator(int32_t itScale, int32_t timeMul) : iteration_scale(itScale), time_multiplier(timeMul) {}

    fl::u8 Value(int32_t i, unsigned long time_ms) const {
        uint32_t input = iteration_scale * i + time_ms * time_multiplier;
        uint16_t v1 = inoise16(input);
        return fl::u8(v1 >> 8);
    }

    int LedValue(int32_t i, unsigned long time_ms) const {
        int val = Value(i, time_ms);
        return MAX(0, val - 128) * 2;
    }
};

FASTLED_NAMESPACE_END
