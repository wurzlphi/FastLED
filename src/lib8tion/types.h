/// @file types.h
/// Defines fractional types used for lib8tion functions

#pragma once

#include "fl/stdint.h"
#include "fl/int.h"
#include "fl/namespace.h"

FASTLED_NAMESPACE_BEGIN

/// @addtogroup lib8tion
/// @{

///////////////////////////////////////////////////////////////////////
///
/// @defgroup FractionalTypes Fixed-Point Fractional Types. 
/// Types for storing fractional data. 
///
/// * ::sfract7 should be interpreted as signed 128ths.
/// * ::fract8 should be interpreted as unsigned 256ths.
/// * ::sfract15 should be interpreted as signed 32768ths.
/// * ::fract16 should be interpreted as unsigned 65536ths.
///
/// Example: if a fract8 has the value "64", that should be interpreted
///          as 64/256ths, or one-quarter.
///
/// accumXY types should be interpreted as X bits of integer,
///         and Y bits of fraction.  
/// E.g., ::accum88 has 8 bits of int, 8 bits of fraction
///
/// @{

/// Fractional types are now defined in fl/int.h - 
/// these typedefs provide backward compatibility
typedef fl::fract8   fract8;
typedef fl::sfract7  sfract7;
typedef fl::fract16  fract16;
typedef fl::sfract31 sfract31;
typedef fl::fract32  fract32;
typedef fl::sfract15 sfract15;
typedef fl::accum88    accum88;
typedef fl::saccum78   saccum78;
typedef fl::accum1616  accum1616;
typedef fl::saccum1516 saccum1516;
typedef fl::accum124   accum124;
typedef fl::saccum114  saccum114;


/// typedef for IEEE754 "binary32" float type internals
/// @see https://en.wikipedia.org/wiki/IEEE_754
typedef union {
    uint32_t i;  ///< raw value, as an integer
    float    f;  ///< raw value, as a float
    struct {
        uint32_t mantissa: 23;  ///< 23-bit mantissa
        uint32_t exponent:  8;  ///< 8-bit exponent
        uint32_t signbit:   1;  ///< sign bit
    };
    struct {
        uint32_t mant7 :  7;  ///< @todo Doc: what is this for?
        uint32_t mant16: 16;  ///< @todo Doc: what is this for?
        uint32_t exp_  :  8;  ///< @todo Doc: what is this for?
        uint32_t sb_   :  1;  ///< @todo Doc: what is this for?
    };
    struct {
        uint32_t mant_lo8 : 8;  ///< @todo Doc: what is this for?
        uint32_t mant_hi16_exp_lo1 : 16;  ///< @todo Doc: what is this for?
        uint32_t sb_exphi7 : 8;  ///< @todo Doc: what is this for?
    };
} IEEE754binary32_t;

/// @} FractionalTypes
/// @} lib8tion

FASTLED_NAMESPACE_END
