#pragma once

#ifndef __INC_LIB8TION_TRIG_H
#define __INC_LIB8TION_TRIG_H

#include "fl/stdint.h"
#include "fl/int.h"
#include "lib8tion/lib8static.h"

/// @file trig8.h
/// Fast, efficient 8-bit trigonometry functions specifically
/// designed for high-performance LED programming.

/// @ingroup lib8tion
/// @{

/// @defgroup Trig Fast Trigonometry Functions
/// Fast 8-bit and 16-bit approximations of sin(x) and cos(x).
///
/// Don't use these approximations for calculating the
/// trajectory of a rocket to Mars, but they're great
/// for art projects and LED displays.
///
/// On Arduino/AVR, the 16-bit approximation is more than
/// 10X faster than floating point sin(x) and cos(x), while
/// the 8-bit approximation is more than 20X faster.
/// @{

#if defined(USE_SIN_32)

#define sin16 fl::sin16lut
#define cos16 fl::cos16lut

#include "fl/sin32.h"

#elif defined(__AVR__)

/// Platform-independent alias of the fast sin implementation
#define sin16 sin16_avr

/// Fast 16-bit approximation of sin(x). This approximation never varies more
/// than 0.69% from the floating point value you'd get by doing
///    @code{.cpp}
///    float s = sin(x) * 32767.0;
///    @endcode
///
/// @param theta input angle from 0-65535
/// @returns sin of theta, value between -32767 to 32767.
LIB8STATIC int16_t sin16_avr(uint16_t theta) {
    static const fl::u8 data[] = {
        0,           0,           49, 0, 6393 % 256,  6393 / 256,  48, 0,
        12539 % 256, 12539 / 256, 44, 0, 18204 % 256, 18204 / 256, 38, 0,
        23170 % 256, 23170 / 256, 31, 0, 27245 % 256, 27245 / 256, 23, 0,
        30273 % 256, 30273 / 256, 14, 0, 32137 % 256, 32137 / 256, 4 /*,0*/};

    uint16_t offset = (theta & 0x3FFF);

    // AVR doesn't have a multi-bit shift instruction,
    // so if we say "offset >>= 3", gcc makes a tiny loop.
    // Inserting empty volatile statements between each
    // bit shift forces gcc to unroll the loop.
    offset >>= 1; // 0..8191
    asm volatile("");
    offset >>= 1; // 0..4095
    asm volatile("");
    offset >>= 1; // 0..2047

    if (theta & 0x4000)
        offset = 2047 - offset;

    fl::u8 sectionX4;
    sectionX4 = offset / 256;
    sectionX4 *= 4;

    fl::u8 m;

    union {
        uint16_t b;
        struct {
            fl::u8 blo;
            fl::u8 bhi;
        };
    } u;

    // in effect u.b = blo + (256 * bhi);
    u.blo = data[sectionX4];
    u.bhi = data[sectionX4 + 1];
    m = data[sectionX4 + 2];

    fl::u8 secoffset8 = (fl::u8)(offset) / 2;

    uint16_t mx = m * secoffset8;

    int16_t y = mx + u.b;
    if (theta & 0x8000)
        y = -y;

    return y;
}

#else

/// Platform-independent alias of the fast sin implementation
#define sin16 sin16_C

/// Fast 16-bit approximation of sin(x). This approximation never varies more
/// than 0.69% from the floating point value you'd get by doing
///    @code{.cpp}
///    float s = sin(x) * 32767.0;
///    @endcode
///
/// @param theta input angle from 0-65535
/// @returns sin of theta, value between -32767 to 32767.
LIB8STATIC int16_t sin16_C(uint16_t theta) {
    static const uint16_t base[] = {0,     6393,  12539, 18204,
                                    23170, 27245, 30273, 32137};
    static const fl::u8 slope[] = {49, 48, 44, 38, 31, 23, 14, 4};

    uint16_t offset = (theta & 0x3FFF) >> 3; // 0..2047
    if (theta & 0x4000)
        offset = 2047 - offset;

    fl::u8 section = offset / 256; // 0..7
    uint16_t b = base[section];
    fl::u8 m = slope[section];

    fl::u8 secoffset8 = (fl::u8)(offset) / 2;

    uint16_t mx = m * secoffset8;
    int16_t y = mx + b;

    if (theta & 0x8000)
        y = -y;

    return y;
}

#endif

/// Fast 16-bit approximation of cos(x). This approximation never varies more
/// than 0.69% from the floating point value you'd get by doing
///    @code{.cpp}
///    float s = cos(x) * 32767.0;
///    @endcode
///
/// @param theta input angle from 0-65535
/// @returns cos of theta, value between -32767 to 32767.
#ifndef USE_SIN_32
LIB8STATIC int16_t cos16(uint16_t theta) { return sin16(theta + 16384); }
#endif

///////////////////////////////////////////////////////////////////////
// sin8() and cos8()
// Fast 8-bit approximations of sin(x) & cos(x).

/// Pre-calculated lookup table used in sin8() and cos8() functions
const fl::u8 b_m16_interleave[] = {0, 49, 49, 41, 90, 27, 117, 10};

#if defined(__AVR__) && !defined(LIB8_ATTINY)
/// Platform-independent alias of the fast sin implementation
#define sin8 sin8_avr

/// Fast 8-bit approximation of sin(x). This approximation never varies more
/// than 2% from the floating point value you'd get by doing
///   @code{.cpp}
///   float s = (sin(x) * 128.0) + 128;
///   @endcode
///
/// @param theta input angle from 0-255
/// @returns sin of theta, value between 0 and 255
LIB8STATIC fl::u8 sin8_avr(fl::u8 theta) {
    fl::u8 offset = theta;

    asm volatile("sbrc %[theta],6         \n\t"
                 "com  %[offset]           \n\t"
                 : [theta] "+r"(theta), [offset] "+r"(offset));

    offset &= 0x3F; // 0..63

    fl::u8 secoffset = offset & 0x0F; // 0..15
    if (theta & 0x40)
        ++secoffset;

    fl::u8 m16;
    fl::u8 b;

    fl::u8 section = offset >> 4; // 0..3
    fl::u8 s2 = section * 2;

    const fl::u8 *p = b_m16_interleave;
    p += s2;
    b = *p;
    ++p;
    m16 = *p;

    fl::u8 mx;
    fl::u8 xr1;
    asm volatile("mul %[m16],%[secoffset]   \n\t"
                 "mov %[mx],r0              \n\t"
                 "mov %[xr1],r1             \n\t"
                 "eor  r1, r1               \n\t"
                 "swap %[mx]                \n\t"
                 "andi %[mx],0x0F           \n\t"
                 "swap %[xr1]               \n\t"
                 "andi %[xr1], 0xF0         \n\t"
                 "or   %[mx], %[xr1]        \n\t"
                 : [mx] "=d"(mx), [xr1] "=d"(xr1)
                 : [m16] "d"(m16), [secoffset] "d"(secoffset));

    int8_t y = mx + b;
    if (theta & 0x80)
        y = -y;

    y += 128;

    return y;
}

#else

/// Platform-independent alias of the fast sin implementation
#define sin8 sin8_C

/// Fast 8-bit approximation of sin(x). This approximation never varies more
/// than 2% from the floating point value you'd get by doing
///   @code{.cpp}
///   float s = (sin(x) * 128.0) + 128;
///   @endcode
///
/// @param theta input angle from 0-255
/// @returns sin of theta, value between 0 and 255
LIB8STATIC fl::u8 sin8_C(fl::u8 theta) {
    fl::u8 offset = theta;
    if (theta & 0x40) {
        offset = (fl::u8)255 - offset;
    }
    offset &= 0x3F; // 0..63

    fl::u8 secoffset = offset & 0x0F; // 0..15
    if (theta & 0x40)
        ++secoffset;

    fl::u8 section = offset >> 4; // 0..3
    fl::u8 s2 = section * 2;
    const fl::u8 *p = b_m16_interleave;
    p += s2;
    fl::u8 b = *p;
    ++p;
    fl::u8 m16 = *p;

    fl::u8 mx = (m16 * secoffset) >> 4;

    int8_t y = mx + b;
    if (theta & 0x80)
        y = -y;

    y += 128;

    return y;
}

#endif

/// Fast 8-bit approximation of cos(x). This approximation never varies more
/// than 2% from the floating point value you'd get by doing
///   @code{.cpp}
///   float s = (cos(x) * 128.0) + 128;
///   @endcode
///
/// @param theta input angle from 0-255
/// @returns cos of theta, value between 0 and 255
LIB8STATIC fl::u8 cos8(fl::u8 theta) { return sin8(theta + 64); }

/// @} Trig
/// @} lib8tion

#endif
