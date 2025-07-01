
#pragma once

#include "fl/stdint.h"

#include "fl/clamp.h"
#include "fl/force_inline.h"
#include "fl/math_macros.h"
#include "fl/compiler_control.h"
#include "fl/int.h"

FL_DISABLE_WARNING_PUSH
FL_DISABLE_WARNING(float-equal)

namespace fl {

template <typename T> struct vec2;

namespace map_range_detail {

// primary template: unchanged
template <typename T, typename U> struct map_range_math;
template <typename T> bool equals(T a, T b) { return a == b; }

} // namespace map_range_detail

template <typename T, typename U>
FASTLED_FORCE_INLINE U map_range(T value, T in_min, T in_max, U out_min,
                                 U out_max) {
    // Not fully tested with all unsigned types, so watch out if you use this
    // with fl::u16 and you value < in_min.
    using namespace map_range_detail;
    if (equals(value, in_min)) {
        return out_min;
    }
    if (equals(value, in_max)) {
        return out_max;
    }
    return map_range_math<T, U>::map(value, in_min, in_max, out_min, out_max);
}

template <typename T, typename U>
FASTLED_FORCE_INLINE U map_range_clamped(T value, T in_min, T in_max, U out_min,
                                         U out_max) {
    // Not fully tested with all unsigned types, so watch out if you use this
    // with fl::u16 and you value < in_min.
    using namespace map_range_detail;
    value = clamp(value, in_min, in_max);
    return map_range<T, U>(value, in_min, in_max, out_min, out_max);
}

//////////////////////////////////// IMPLEMENTATION
//////////////////////////////////////////

namespace map_range_detail {

// primary template: unchanged
template <typename T, typename U> struct map_range_math {
    static U map(T value, T in_min, T in_max, U out_min, U out_max) {
        if (in_min == in_max)
            return out_min;
        return out_min +
               (value - in_min) * (out_max - out_min) / (in_max - in_min);
    }
};

template <> struct map_range_math<fl::u8, fl::u8> {
    static fl::u8 map(fl::u8 value, fl::u8 in_min, fl::u8 in_max,
                       fl::u8 out_min, fl::u8 out_max) {
        if (value == in_min) {
            return out_min;
        }
        if (value == in_max) {
            return out_max;
        }
        // Promote fl::u8 to fl::i16 for mapping.
        fl::i16 v16 = value;
        fl::i16 in_min16 = in_min;
        fl::i16 in_max16 = in_max;
        fl::i16 out_min16 = out_min;
        fl::i16 out_max16 = out_max;
        fl::i16 out16 = map_range<fl::u16, fl::u16>(v16, in_min16, in_max16,
                                                      out_min16, out_max16);
        if (out16 < 0) {
            out16 = 0;
        } else if (out16 > 255) {
            out16 = 255;
        }
        return static_cast<fl::u8>(out16);
    }
};

// partial specialization for U = vec2<V>
template <typename T, typename V> struct map_range_math<T, vec2<V>> {
    static vec2<V> map(T value, T in_min, T in_max, vec2<V> out_min,
                       vec2<V> out_max) {
        if (in_min == in_max) {
            return out_min;
        }
        // normalized [0..1]
        T scale = (value - in_min) / T(in_max - in_min);
        // deltas
        V dx = out_max.x - out_min.x;
        V dy = out_max.y - out_min.y;
        // lerp each component
        V x = out_min.x + V(dx * scale);
        V y = out_min.y + V(dy * scale);
        return {x, y};
    }
};

inline bool equals(float a, float b) { return ALMOST_EQUAL_FLOAT(a, b); }
inline bool equals(double d, double d2) { return ALMOST_EQUAL_DOUBLE(d, d2); }

} // namespace map_range_detail

} // namespace fl

FL_DISABLE_WARNING_POP
