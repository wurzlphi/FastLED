#pragma once

#ifndef __INC_LIB8TION_H
#define __INC_LIB8TION_H

// Remove circular dependency on FastLED.h
// #include "FastLED.h"
#include "lib8tion_base.h"  // New minimal base header

// Note: These were already included by lib8tion_base.h but we'll keep them 
// explicit for clarity and backward compatibility
#include "lib8tion/types.h"
#include "fl/deprecated.h"

// Remove the LED_SYSDEFS check that creates circular dependency
// The platform detection is now handled in lib8tion_base.h
// #ifndef __INC_LED_SYSDEFS_H
// #error WTH?  led_sysdefs needs to be included first
// #endif

/// @file lib8tion.h
/// Fast, efficient 8-bit math functions specifically
/// designed for high-performance LED programming. 

#include "fl/stdint.h"
#include "lib8tion/lib8static.h"
#include "lib8tion/qfx.h"
#include "lib8tion/memmove.h"
#include "lib8tion/config.h"
#include "fl/ease.h"


#if !defined(__AVR__)
#include <string.h>
// for memmove, memcpy, and memset if not defined here
#endif // end of !defined(__AVR__)


/// @defgroup lib8tion Fast Math Functions
/// Fast, efficient 8-bit math functions specifically
/// designed for high-performance LED programming. 
///
/// Because of the AVR (Arduino) and ARM assembly language
/// implementations provided, using these functions often
/// results in smaller and faster code than the equivalent
/// program using plain "C" arithmetic and logic.
///
/// Included are:
///
///  - Saturating unsigned 8-bit add and subtract.
///    Instead of wrapping around if an overflow occurs,
///    these routines just 'clamp' the output at a maxumum
///    of 255, or a minimum of 0.  Useful for adding pixel
///    values.  E.g., qadd8( 200, 100) = 255.
///      @code
///      qadd8( i, j) == MIN( (i + j), 0xFF )
///      qsub8( i, j) == MAX( (i - j), 0 )
///      @endcode
///
///  - Saturating signed 8-bit ("7-bit") add.
///      @code
///      qadd7( i, j) == MIN( (i + j), 0x7F)
///      @endcode
///
///  - Scaling (down) of unsigned 8- and 16- bit values.
///    Scaledown value is specified in 1/256ths.
///      @code
///      scale8( i, sc) == (i * sc) / 256
///      scale16by8( i, sc) == (i * sc) / 256
///      @endcode
///
///    Example: scaling a 0-255 value down into a
///    range from 0-99:
///      @code
///      downscaled = scale8( originalnumber, 100);
///      @endcode
///
///    A special version of scale8 is provided for scaling
///    LED brightness values, to make sure that they don't
///    accidentally scale down to total black at low
///    dimming levels, since that would look wrong:
///      @code
///      scale8_video( i, sc) = ((i * sc) / 256) +? 1
///      @endcode
///
///    Example: reducing an LED brightness by a
///    dimming factor:
///      @code
///      new_bright = scale8_video( orig_bright, dimming);
///      @endcode
///
///  - Fast 8- and 16- bit unsigned random numbers.
///    Significantly faster than Arduino random(), but
///    also somewhat less random.  You can add entropy.
///      @code
///      random8()       == random from 0..255
///      random8( n)     == random from 0..(N-1)
///      random8( n, m)  == random from N..(M-1)
///
///      random16()      == random from 0..65535
///      random16( n)    == random from 0..(N-1)
///      random16( n, m) == random from N..(M-1)
///
///      random16_set_seed( k)    ==  seed = k
///      random16_add_entropy( k) ==  seed += k
///      @endcode
///
///  - Absolute value of a signed 8-bit value.
///      @code
///      abs8( i)     == abs( i)
///      @endcode
///
///  - 8-bit math operations which return 8-bit values.
///    These are provided mostly for completeness,
///    not particularly for performance.
///      @code
///      mul8( i, j)  == (i * j) & 0xFF
///      add8( i, j)  == (i + j) & 0xFF
///      sub8( i, j)  == (i - j) & 0xFF
///      @endcode
///
///  - Fast 16-bit approximations of sin and cos.
///    Input angle is a uint16_t from 0-65535.
///    Output is a signed int16_t from -32767 to 32767.
///      @code
///      sin16( x)  == sin( (x/32768.0) * pi) * 32767
///      cos16( x)  == cos( (x/32768.0) * pi) * 32767
///      @endcode
///
///    Accurate to more than 99% in all cases.
///
///  - Fast 8-bit approximations of sin and cos.
///    Input angle is a uint8_t from 0-255.
///    Output is an UNsigned uint8_t from 0 to 255.
///      @code
///      sin8( x)  == (sin( (x/128.0) * pi) * 128) + 128
///      cos8( x)  == (cos( (x/128.0) * pi) * 128) + 128
///      @endcode
///
///    Accurate to within about 2%.
///
///  - Fast 8-bit "easing in/out" function.
///      @code
///      ease8InOutCubic(x) == 3(x^2) - 2(x^3)
///      ease8InOutApprox(x) ==
///        faster, rougher, approximation of cubic easing
///      ease8InOutQuad(x) == quadratic (vs cubic) easing
///      @endcode
///
///  - Cubic, Quadratic, and Triangle wave functions.
///    Input is a uint8_t representing phase withing the wave,
///      similar to how sin8 takes an angle 'theta'.
///    Output is a uint8_t representing the amplitude of
///    the wave at that point.
///      @code
///      cubicwave8( x)
///      quadwave8( x)
///      triwave8( x)
///      @endcode
///
///  - Square root for 16-bit integers.  About three times
///    faster and five times smaller than Arduino's built-in
///    generic 32-bit sqrt routine.
///      @code
///      sqrt16( uint16_t x ) == sqrt( x)
///      @endcode
///
///  - Dimming and brightening functions for 8-bit
///    light values.
///      @code
///      dim8_video( x)  == scale8_video( x, x)
///      dim8_raw( x)    == scale8( x, x)
///      dim8_lin( x)    == (x<128) ? ((x+1)/2) : scale8(x,x)
///      brighten8_video( x) == 255 - dim8_video( 255 - x)
///      brighten8_raw( x) == 255 - dim8_raw( 255 - x)
///      brighten8_lin( x) == 255 - dim8_lin( 255 - x)
///      @endcode
///
///    The dimming functions in particular are suitable
///    for making LED light output appear more 'linear'.
///
///  - Linear interpolation between two values, with the
///    fraction between them expressed as an 8- or 16-bit
///    fixed point fraction (fract8 or fract16).
///      @code
///      lerp8by8(   fromU8, toU8, fract8 )
///      lerp16by8(  fromU16, toU16, fract8 )
///      lerp15by8(  fromS16, toS16, fract8 )
///        == from + (( to - from ) * fract8) / 256)
///      lerp16by16( fromU16, toU16, fract16 )
///        == from + (( to - from ) * fract16) / 65536)
///      map8( in, rangeStart, rangeEnd)
///        == map( in, 0, 255, rangeStart, rangeEnd);
///      @endcode
///
///  - Optimized memmove, memcpy, and memset, that are
///    faster than standard avr-libc 1.8.
///      @code
///      memmove8( dest, src,  bytecount)
///      memcpy8(  dest, src,  bytecount)
///      memset8(  buf, value, bytecount)
///      @endcode
///
///  - Beat generators which return sine or sawtooth
///    waves in a specified number of Beats Per Minute.
///    Sine wave beat generators can specify a low and
///    high range for the output.  Sawtooth wave beat
///    generators always range 0-255 or 0-65535.
///      @code
///      beatsin8( BPM, low8, high8)
///          = (sine(beatphase) * (high8-low8)) + low8
///      beatsin16( BPM, low16, high16)
///          = (sine(beatphase) * (high16-low16)) + low16
///      beatsin88( BPM88, low16, high16)
///          = (sine(beatphase) * (high16-low16)) + low16
///      beat8( BPM)  = 8-bit repeating sawtooth wave
///      beat16( BPM) = 16-bit repeating sawtooth wave
///      beat88( BPM88) = 16-bit repeating sawtooth wave
///      @endcode
///
///    BPM is beats per minute in either simple form
///    e.g. 120, or Q8.8 fixed-point form.
///    BPM88 is beats per minute in ONLY Q8.8 fixed-point
///    form.
///
/// Lib8tion is pronounced like 'libation': lie-BAY-shun
///
/// @{




#include "lib8tion/math8.h"
#include "lib8tion/scale8.h"
#include "lib8tion/random8.h"
#include "lib8tion/trig8.h"

///////////////////////////////////////////////////////////////////////




FASTLED_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////
///
/// @defgroup FloatConversions Float-to-Fixed and Fixed-to-Float Conversions
/// Functions to convert between floating point and fixed point types. 
/// @note Anything involving a "float" on AVR will be slower.
/// @{

/// Conversion from 16-bit fixed point (::sfract15) to IEEE754 32-bit float.
LIB8STATIC float sfract15ToFloat( sfract15 y)
{
    return y / 32768.0;
}

/// Conversion from IEEE754 float in the range (-1,1) to 16-bit fixed point (::sfract15).
/// @note The extremes of one and negative one are NOT representable! The
/// representable range is 0.99996948242 to -0.99996948242, in steps of 0.00003051757.
LIB8STATIC sfract15 floatToSfract15( float f)
{
    return f * 32768.0;
}

/// @} FloatConversions

// Note: The rest of lib8tion.h continues unchanged from here...
// Including all the interpolation, easing, waveform, and timing functions
// This comment represents lines 259-1249 of the original file
