#pragma once

#include "fl/stdint.h"

namespace fl {
    // 8-bit types - char is reliably 8 bits on all supported platforms
    typedef signed char i8;
    typedef unsigned char u8;
    
    // 16-bit and 32-bit types - platform-specific to match stdint.h exactly
    #ifdef __AVR__
        // On AVR: int is 16-bit, long is 32-bit
        // This matches how stdint.h defines these types on AVR
        typedef int i16;
        typedef unsigned int u16;
        typedef long i32;
        typedef unsigned long u32;
        typedef long long i64;
        typedef unsigned long long u64;
    #elif defined(ESP32) || defined(ESP_PLATFORM)
        // On ESP32: short is 16-bit, long is 32-bit (to match uint32_t)
        // This ensures fl::u32 matches uint32_t exactly for function pointer compatibility
        typedef short i16;
        typedef unsigned short u16;
        typedef long i32;
        typedef unsigned long u32;
        typedef long long i64;
        typedef unsigned long long u64;
    #else
        // On most other platforms: short is 16-bit, int is 32-bit
        typedef short i16;
        typedef unsigned short u16;
        typedef int i32;
        typedef unsigned int u32;
        typedef long long i64;
        typedef unsigned long long u64;
    #endif
    
    // Pointer and size types - universal across platforms
    typedef uintptr_t uptr;  ///< Pointer-sized unsigned integer
    typedef size_t sz;       ///< Size type for containers and memory
    
    ///////////////////////////////////////////////////////////////////////
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

    /// ANSI: unsigned short _Fract. 
    /// Range is 0 to 0.99609375 in steps of 0.00390625.  
    /// Should be interpreted as unsigned 256ths.
    typedef uint8_t   fract8;

    /// ANSI: signed short _Fract. 
    /// Range is -0.9921875 to 0.9921875 in steps of 0.0078125.  
    /// Should be interpreted as signed 128ths.
    typedef int8_t    sfract7;

    /// ANSI: unsigned _Fract.
    /// Range is 0 to 0.99998474121 in steps of 0.00001525878.  
    /// Should be interpreted as unsigned 65536ths.
    typedef uint16_t  fract16;

    typedef int32_t   sfract31; ///< ANSI: signed long _Fract. 31 bits int, 1 bit fraction

    typedef uint32_t  fract32;   ///< ANSI: unsigned long _Fract. 32 bits int, 32 bits fraction

    /// ANSI: signed _Fract.
    /// Range is -0.99996948242 to 0.99996948242 in steps of 0.00003051757.  
    /// Should be interpreted as signed 32768ths.
    typedef int16_t   sfract15;

    typedef uint16_t  accum88;    ///< ANSI: unsigned short _Accum. 8 bits int, 8 bits fraction
    typedef int16_t   saccum78;   ///< ANSI: signed   short _Accum. 7 bits int, 8 bits fraction
    typedef uint32_t  accum1616;  ///< ANSI: signed         _Accum. 16 bits int, 16 bits fraction
    typedef int32_t   saccum1516; ///< ANSI: signed         _Accum. 15 bits int, 16 bits fraction
    typedef uint16_t  accum124;   ///< no direct ANSI counterpart. 12 bits int, 4 bits fraction
    typedef int32_t   saccum114;  ///< no direct ANSI counterpart. 1 bit int, 14 bits fraction

    /// @} FractionalTypes
    
    // Compile-time verification that types are exactly the expected size
    static_assert(sizeof(i8) == 1, "i8 must be exactly 1 byte");
    static_assert(sizeof(i16) == 2, "i16 must be exactly 2 bytes");
    static_assert(sizeof(i32) == 4, "i32 must be exactly 4 bytes");
    static_assert(sizeof(i64) == 8, "i64 must be exactly 8 bytes");
    static_assert(sizeof(u8) == 1, "u8 must be exactly 1 byte");
    static_assert(sizeof(u16) == 2, "u16 must be exactly 2 bytes");
    static_assert(sizeof(u32) == 4, "u32 must be exactly 4 bytes");
    static_assert(sizeof(u64) == 8, "u64 must be exactly 8 bytes");
    static_assert(sizeof(uptr) >= sizeof(void*), "uptr must be at least pointer size");
    static_assert(sizeof(sz) >= sizeof(void*), "sz must be at least pointer size for large memory operations");
    
    // Compile-time verification for fractional types
    static_assert(sizeof(fract8) == 1, "fract8 must be exactly 1 byte");
    static_assert(sizeof(sfract7) == 1, "sfract7 must be exactly 1 byte");
    static_assert(sizeof(fract16) == 2, "fract16 must be exactly 2 bytes");
    static_assert(sizeof(sfract15) == 2, "sfract15 must be exactly 2 bytes");
    static_assert(sizeof(fract32) == 4, "fract32 must be exactly 4 bytes");
    static_assert(sizeof(sfract31) == 4, "sfract31 must be exactly 4 bytes");
    static_assert(sizeof(accum88) == 2, "accum88 must be exactly 2 bytes");
    static_assert(sizeof(saccum78) == 2, "saccum78 must be exactly 2 bytes");
    static_assert(sizeof(accum1616) == 4, "accum1616 must be exactly 4 bytes");
    static_assert(sizeof(saccum1516) == 4, "saccum1516 must be exactly 4 bytes");
    static_assert(sizeof(accum124) == 2, "accum124 must be exactly 2 bytes");
    static_assert(sizeof(saccum114) == 4, "saccum114 must be exactly 4 bytes");
}
