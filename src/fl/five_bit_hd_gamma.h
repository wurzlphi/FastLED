/// @file five_bit_hd_gamma.h
/// Declares functions for five-bit gamma correction

#pragma once

#include "fl/gamma.h"
#include "fl/stdint.h"
#include "fl/int.h"
#include "fl/bitcast.h"

#include "crgb.h"
#include "lib8tion/scale8.h"
#include "fl/force_inline.h"
#include "fl/math.h"
#include "fl/namespace.h"
#include "lib8tion/brightness_bitshifter.h"

namespace fl {

enum FiveBitGammaCorrectionMode {
    kFiveBitGammaCorrectionMode_Null = 0,
    kFiveBitGammaCorrectionMode_BitShift = 1
};

// Applies gamma correction for the RGBV(8, 8, 8, 5) color space, where
// the last byte is the brightness byte at 5 bits.
// To override this five_bit_hd_gamma_bitshift you'll need to define
// FASTLED_FIVE_BIT_HD_BITSHIFT_FUNCTION_OVERRIDE in your build settings
// then define the function anywhere in your project.
// Example:
//  FASTLED_NAMESPACE_BEGIN
//  void five_bit_hd_gamma_bitshift(
//      fl::u8 r8, fl::u8 g8, fl::u8 b8,
//      fl::u8 r8_scale, fl::u8 g8_scale, fl::u8 b8_scale,
//      fl::u8* out_r8,
//      fl::u8* out_g8,
//      fl::u8* out_b8,
//      fl::u8* out_power_5bit) {
//        cout << "hello world\n";
//  }
//  FASTLED_NAMESPACE_END

// Force push

void internal_builtin_five_bit_hd_gamma_bitshift(CRGB colors, CRGB colors_scale,
                                                 fl::u8 global_brightness,
                                                 CRGB *out_colors,
                                                 fl::u8 *out_power_5bit);

// Exposed for testing.
fl::u8 five_bit_bitshift(u16 r16, u16 g16, u16 b16,
                          fl::u8 brightness, CRGB *out,
                          fl::u8 *out_power_5bit);

#ifdef FASTLED_FIVE_BIT_HD_BITSHIFT_FUNCTION_OVERRIDE
// This function is located somewhere else in your project, so it's declared
// extern here.
extern void five_bit_hd_gamma_bitshift(CRGB colors, CRGB colors_scale,
                                       fl::u8 global_brightness,
                                       CRGB *out_colors,
                                       fl::u8 *out_power_5bit);
#else
inline void five_bit_hd_gamma_bitshift(CRGB colors, CRGB colors_scale,
                                       fl::u8 global_brightness,
                                       CRGB *out_colors,
                                       fl::u8 *out_power_5bit) {
    internal_builtin_five_bit_hd_gamma_bitshift(
        colors, colors_scale, global_brightness, out_colors, out_power_5bit);
}
#endif // FASTLED_FIVE_BIT_HD_BITSHIFT_FUNCTION_OVERRIDE

// Simple gamma correction function that converts from
// 8-bit color component and converts it to gamma corrected 16-bit
// color component. Fast and no memory overhead!
// To override this function you'll need to define
// FASTLED_FIVE_BIT_HD_GAMMA_BITSHIFT_FUNCTION_OVERRIDE in your build settings
// and then define your own version anywhere in your project. Example:
//  FASTLED_NAMESPACE_BEGIN
//  void five_bit_hd_gamma_function(
//    fl::u8 r8, fl::u8 g8, fl::u8 b8,
//    u16* r16, u16* g16, u16* b16) {
//      cout << "hello world\n";
//  }
//  FASTLED_NAMESPACE_END
#ifdef FASTLED_FIVE_BIT_HD_GAMMA_FUNCTION_OVERRIDE
// This function is located somewhere else in your project, so it's declared
// extern here.
extern void five_bit_hd_gamma_function(CRGB color, u16 *r16, u16 *g16,
                                       u16 *b16);
#else
inline void five_bit_hd_gamma_function(CRGB color, u16 *r16, u16 *g16,
                                       u16 *b16) {

    gamma16(color, r16, g16, b16);
}
#endif // FASTLED_FIVE_BIT_HD_GAMMA_FUNCTION_OVERRIDE

inline void internal_builtin_five_bit_hd_gamma_bitshift(
    CRGB colors, CRGB colors_scale, fl::u8 global_brightness, CRGB *out_colors,
    fl::u8 *out_power_5bit) {

    if (global_brightness == 0) {
        *out_colors = CRGB(0, 0, 0);
        *out_power_5bit = 0;
        return;
    }

    // Step 1: Gamma Correction
    u16 r16, g16, b16;
    five_bit_hd_gamma_function(colors, &r16, &g16, &b16);

    // Step 2: Color correction step comes after gamma correction. These values
    // are assumed to be be relatively close to 255.
    if (colors_scale.r != 0xff) {
        r16 = scale16by8(r16, colors_scale.r);
    }
    if (colors_scale.g != 0xff) {
        g16 = scale16by8(g16, colors_scale.g);
    }
    if (colors_scale.b != 0xff) {
        b16 = scale16by8(b16, colors_scale.b);
    }

    five_bit_bitshift(r16, g16, b16, global_brightness, out_colors,
                      out_power_5bit);
}

inline fl::u8 five_bit_bitshift(u16 r16, u16 g16, u16 b16,
                                 fl::u8 brightness, CRGB *out,
                                 fl::u8 *out_power_5bit) {

    auto max3 = [](u16 a, u16 b, u16 c) {
        return fl_max(fl_max(a, b), c);
    };

    if (brightness == 0) {
        *out = CRGB(0, 0, 0);
        *out_power_5bit = 0;
        return 0;
    }
    if (r16 == 0 && g16 == 0 && b16 == 0) {
        *out = CRGB(0, 0, 0);
        *out_power_5bit = (brightness <= 31) ? brightness : 31;
        return brightness;
    }

    // Note: One day someone smarter than me will come along and invent a closed
    // form solution for this. However, the numerical method works extremely
    // well and has been optimized to avoid division performance penalties as
    // much as possible.

    // Step 1: Initialize brightness
    static const fl::u8 kStartBrightness = 0b00010000;
    fl::u8 v5 = kStartBrightness;
    // Step 2: Boost brightness by swapping power with the driver brightness.
    brightness_bitshifter8(&v5, &brightness, 4);

    // Step 3: Boost brightness of the color channels by swapping power with the
    // driver brightness.
    u16 max_component = max3(r16, g16, b16);
    // five_bit_color_bitshift(&r16, &g16, &b16, &v5);
    fl::u8 shifts = brightness_bitshifter16(&v5, fl::bit_cast_ptr<fl::u16>(static_cast<void*>(&max_component)), 4, 2);
    if (shifts) {
        r16 = r16 << shifts;
        g16 = g16 << shifts;
        b16 = b16 << shifts;
    }

    // Step 4: scale by final brightness factor.
    if (brightness != 0xff) {
        r16 = scale16by8(r16, brightness);
        g16 = scale16by8(g16, brightness);
        b16 = scale16by8(b16, brightness);
    }

    // brighten hardware brightness by turning on low order bits
    if (v5 > 1) {
        // since v5 is a power of two, subtracting one will invert the leading
        // bit and invert all the bits below it. Example: 0b00010000 -1 =
        // 0b00001111 So 0b00010000 | 0b00001111 = 0b00011111
        v5 = v5 | (v5 - 1);
    }
    // Step 5: Convert back to 8-bit and output.
    *out = CRGB(map16_to_8(r16), map16_to_8(g16), map16_to_8(b16));
    *out_power_5bit = v5;
    return brightness;
}

} // namespace fl
