/// @file fl/crgb.h
/// Defines the red, green, and blue (RGB) pixel struct in the fl namespace

#pragma once

#include "fl/stdint.h"

namespace fl {

/// Representation of an RGB pixel (Red, Green, Blue)
struct CRGB {
    union {
        struct {
            union {
                uint8_t r;    ///< Red channel value
                uint8_t red;  ///< @copydoc r
            };
            union {
                uint8_t g;      ///< Green channel value
                uint8_t green;  ///< @copydoc g
            };
            union {
                uint8_t b;     ///< Blue channel value
                uint8_t blue;  ///< @copydoc b
            };
        };
        /// Access the red, green, and blue data as an array.
        uint8_t raw[3];
    };

    /// Array access operator to index into the CRGB object
    inline uint8_t& operator[](uint8_t x) { return raw[x]; }
    
    /// Array access operator to index into the CRGB object (const)
    inline const uint8_t& operator[](uint8_t x) const { return raw[x]; }

    /// Default constructor - uninitialized 
    inline CRGB() = default;

    /// Allow construction from R, G, B
    constexpr CRGB(uint8_t ir, uint8_t ig, uint8_t ib) noexcept : r(ir), g(ig), b(ib) {}

    /// Allow construction from 32-bit color code
    constexpr CRGB(uint32_t colorcode) noexcept
    : r((colorcode >> 16) & 0xFF), g((colorcode >> 8) & 0xFF), b((colorcode >> 0) & 0xFF) {}

    /// Allow copy construction
    inline CRGB(const CRGB& rhs) = default;

    /// Assignment from RGB values
    inline CRGB& setRGB(uint8_t nr, uint8_t ng, uint8_t nb) {
        r = nr; g = ng; b = nb;
        return *this;
    }

    /// Assignment from 32-bit color code
    inline CRGB& operator=(const uint32_t colorcode) {
        r = (colorcode >> 16) & 0xFF;
        g = (colorcode >>  8) & 0xFF;
        b = (colorcode >>  0) & 0xFF;
        return *this;
    }

    /// Copy assignment
    inline CRGB& operator=(const CRGB& rhs) = default;
};

/// Check if two CRGB objects have the same color data
inline bool operator==(const CRGB& lhs, const CRGB& rhs) {
    return (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b);
}

/// Check if two CRGB objects do not have the same color data
inline bool operator!=(const CRGB& lhs, const CRGB& rhs) {
    return !(lhs == rhs);
}

} // namespace fl
