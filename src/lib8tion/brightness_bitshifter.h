/// @file brightness_bitshifter.h
/// Defines brightness bitshifting functions

#pragma once

#include "fl/stdint.h"
#include "fl/int.h"

/// @addtogroup lib8tion
/// @{

/// @addtogroup Dimming
/// @{

inline fl::u8 brightness_bitshifter8(fl::u8 *brightness_src, fl::u8 *brightness_dst, fl::u8 max_shifts) {
    fl::u8 src = *brightness_src;
    if (*brightness_dst == 0 || src == 0) {
        return 0;
    }
    // Steal brightness from brightness_src and give it to brightness_dst.
    // After this function concludes the multiplication of brightness_dst and brightness_src will remain
    // constant.
    // This algorithm is a little difficult to follow and I don't understand why it works that well,
    // however I did work it out manually and has something to do with how numbers respond to bitshifts.
    fl::u8 curr = *brightness_dst;
    fl::u8 shifts = 0;
    for (fl::u8 i = 0; i < max_shifts && src > 1; i++) {
        if (curr & 0b10000000) {
            // next shift will overflow
            break;
        }
        curr <<= 1;
        src >>= 1;
        shifts++;
    }
    // write out the output values.
    *brightness_dst = curr;
    *brightness_src = src;
    return shifts;
}

// Return value is the number of shifts on the src. Multiply this by the number of steps to get the
// the number of shifts on the dst.
inline fl::u8 brightness_bitshifter16(fl::u8 *brightness_src, uint16_t *brightness_dst, fl::u8 max_shifts, fl::u8 steps=2) {
    fl::u8 src = *brightness_src;
    if (*brightness_dst == 0 || src == 0) {
        return 0;
    }
    uint16_t overflow_mask = 0b1000000000000000;
    for (fl::u8 i = 1; i < steps; i++) {
        overflow_mask >>= 1;
        overflow_mask |= 0b1000000000000000;
    }
    const fl::u8 underflow_mask = 0x1;
    // Steal brightness from brightness_src and give it to brightness_dst.
    // After this function concludes the multiplication of brightness_dst and brightness_src will remain
    // constant.
    uint16_t curr = *brightness_dst;
    fl::u8 shifts = 0;
    for (fl::u8 i = 0; i < max_shifts; i++) {
        if (src & underflow_mask) {
            break;
        }
        if (curr & overflow_mask) {
            // next shift will overflow
            break;
        }
        curr <<= steps;
        src >>= 1;
        shifts++;
    }
    // write out the output values.
    *brightness_dst = curr;
    *brightness_src = src;
    return shifts;
}

/// @} Dimming
/// @} lib8tion
