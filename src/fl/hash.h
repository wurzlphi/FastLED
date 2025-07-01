#pragma once

#include "fl/str.h"
#include "fl/template_magic.h"
#include "fl/int.h"
#include "fl/stdint.h"
#include "fl/force_inline.h"
#include <string.h>

namespace fl {

template <typename T> struct vec2;

//-----------------------------------------------------------------------------
// MurmurHash3 x86 32-bit
//-----------------------------------------------------------------------------
// Based on the public‐domain implementation by Austin Appleby:
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
static inline fl::u32 MurmurHash3_x86_32(const void *key, size_t len,
                                          fl::u32 seed = 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"

    const fl::u8 *data = static_cast<const fl::u8 *>(key);
    const int nblocks = int(len / 4);

    fl::u32 h1 = seed;
    const fl::u32 c1 = 0xcc9e2d51;
    const fl::u32 c2 = 0x1b873593;

    // body
    const fl::u32 *blocks = reinterpret_cast<const fl::u32 *>(data);
    for (int i = 0; i < nblocks; ++i) {
        fl::u32 k1 = blocks[i];
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + 0xe6546b64;
    }

    // tail
    const fl::u8 *tail = data + (nblocks * 4);
    fl::u32 k1 = 0;
    switch (len & 3) {
    case 3:
        k1 ^= fl::u32(tail[2]) << 16;
    case 2:
        k1 ^= fl::u32(tail[1]) << 8;
    case 1:
        k1 ^= fl::u32(tail[0]);
        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= c2;
        h1 ^= k1;
    }

    // finalization
    h1 ^= fl::u32(len);
    // fmix32
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;

#pragma GCC diagnostic pop
}

//-----------------------------------------------------------------------------
// Fast, cheap 32-bit integer hash (Thomas Wang)
//-----------------------------------------------------------------------------
static inline fl::u32 fast_hash32(fl::u32 x) noexcept {
    x = (x ^ 61u) ^ (x >> 16);
    x = x + (x << 3);
    x = x ^ (x >> 4);
    x = x * 0x27d4eb2dU;
    x = x ^ (x >> 15);
    return x;
}

// 3) Handy two-word hasher
static inline fl::u32 hash_pair(fl::u32 a, fl::u32 b,
                                 fl::u32 seed = 0) noexcept {
    // mix in 'a', then mix in 'b'
    fl::u32 h = fast_hash32(seed ^ a);
    return fast_hash32(h ^ b);
}

//-----------------------------------------------------------------------------
// Functor for hashing arbitrary byte‐ranges to a 32‐bit value
//-----------------------------------------------------------------------------
// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
template <typename T> struct Hash {
    static_assert(fl::is_pod<T>::value,
                  "fl::Hash<T> only supports POD types (integrals, floats, "
                  "etc.), you need to define your own hash.");
    fl::u32 operator()(const T &key) const noexcept {
        return MurmurHash3_x86_32(&key, sizeof(T));
    }
};

template <typename T> struct FastHash {
    static_assert(fl::is_pod<T>::value,
                  "fl::FastHash<T> only supports POD types (integrals, floats, "
                  "etc.), you need to define your own hash.");
    fl::u32 operator()(const T &key) const noexcept {
        return fast_hash32(key);
    }
};

template <typename T> struct FastHash<vec2<T>> {
    fl::u32 operator()(const vec2<T> &key) const noexcept {
        if (sizeof(T) == sizeof(fl::u8)) {
            fl::u32 x = static_cast<fl::u32>(key.x) +
                         (static_cast<fl::u32>(key.y) << 8);
            return fast_hash32(x);
        }
        if (sizeof(T) == sizeof(u16)) {
            fl::u32 x = static_cast<fl::u32>(key.x) +
                         (static_cast<fl::u32>(key.y) << 16);
            return fast_hash32(x);
        }
        if (sizeof(T) == sizeof(fl::u32)) {
            return hash_pair(key.x, key.y);
        }
        return MurmurHash3_x86_32(&key, sizeof(T));
    }
};

template <typename T> struct Hash<T *> {
    fl::u32 operator()(T *key) const noexcept {
        if (sizeof(T *) == sizeof(fl::u32)) {
            fl::u32 key_u = reinterpret_cast<uintptr_t>(key);
            return fast_hash32(key_u);
        } else {
            return MurmurHash3_x86_32(key, sizeof(T *));
        }
    }
};

template <typename T> struct Hash<vec2<T>> {
    fl::u32 operator()(const vec2<T> &key) const noexcept {
#ifndef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        T packed[2];
        memset(packed, 0, sizeof(packed));
        packed[0] = key.x;
        packed[1] = key.y; // Protect against alignment issues
        const void *p = &packed[0];
        return MurmurHash3_x86_32(p, sizeof(packed));
#ifndef __clang__
#pragma GCC diagnostic pop
#endif
    }
};

template <typename T> struct Hash<Ptr<T>> {
    fl::u32 operator()(const T &key) const noexcept {
        auto hasher = Hash<T *>();
        return hasher(key.get());
    }
};

#define FASTLED_DEFINE_FAST_HASH(T)                                            \
    template <> struct Hash<T> {                                               \
        fl::u32 operator()(const int &key) const noexcept {                   \
            return fast_hash32(key);                                           \
        }                                                                      \
    };

FASTLED_DEFINE_FAST_HASH(fl::u8)
FASTLED_DEFINE_FAST_HASH(fl::u16)
FASTLED_DEFINE_FAST_HASH(fl::u32)
FASTLED_DEFINE_FAST_HASH(fl::i8)
FASTLED_DEFINE_FAST_HASH(fl::i16)
FASTLED_DEFINE_FAST_HASH(fl::i32)
FASTLED_DEFINE_FAST_HASH(float)
FASTLED_DEFINE_FAST_HASH(double)
FASTLED_DEFINE_FAST_HASH(bool)

// FASTLED_DEFINE_FAST_HASH(int)

//-----------------------------------------------------------------------------
// Convenience for std::string → fl::u32
//----------------------------------------------------------------------------
template <> struct Hash<fl::string> {
    fl::u32 operator()(const fl::string &key) const noexcept {
        return MurmurHash3_x86_32(key.data(), key.size());
    }
};



} // namespace fl
