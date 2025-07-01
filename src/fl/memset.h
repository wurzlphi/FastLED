#pragma once

/// @file memset.h
/// @brief FastLED memset implementation for embedded systems
/// 
/// Provides fl::memset as an alternative to standard library memset,
/// following FastLED's pattern of providing embedded-friendly implementations
/// of standard library functions.

#include "fl/stdint.h"
#include "fl/namespace.h"

// Include the optimized memset8 for AVR platforms when available
#include "lib8tion/memmove.h"

namespace fl {

/// @brief Set memory to a specified value
/// @param ptr Pointer to the memory to set
/// @param value Value to set each byte to (cast to unsigned char)
/// @param num Number of bytes to set
/// @return Pointer to the memory area ptr
inline void* memset(void* ptr, int value, size_t num) {
    if (!ptr || num == 0) {
        return ptr;
    }
    
#if defined(__AVR__)
    // Use optimized memset8 for AVR platforms when the value fits in uint8_t
    // and the count fits in uint16_t
    if (num <= UINT16_MAX) {
        return memset8(ptr, static_cast<uint8_t>(value), static_cast<uint16_t>(num));
    }
#endif
    
    // Fallback to standard library memset for other platforms or large sizes
    return ::memset(ptr, value, num);
}

/// @brief Convenience overload for setting memory to zero
/// @param ptr Pointer to the memory to zero
/// @param num Number of bytes to zero
/// @return Pointer to the memory area ptr
inline void* zero(void* ptr, size_t num) {
    return memset(ptr, 0, num);
}

/// @brief Template version for type-safe memory setting
/// @tparam T Type of objects to set
/// @param ptr Pointer to the array of T objects
/// @param value Value to set each byte to
/// @param count Number of T objects to set
/// @return Pointer to the memory area ptr as T*
template<typename T>
inline T* memset(T* ptr, int value, size_t count) {
    return static_cast<T*>(memset(static_cast<void*>(ptr), value, count * sizeof(T)));
}

/// @brief Template version for zeroing arrays of objects
/// @tparam T Type of objects to zero
/// @param ptr Pointer to the array of T objects
/// @param count Number of T objects to zero
/// @return Pointer to the memory area ptr as T*
template<typename T>
inline T* zero(T* ptr, size_t count) {
    return static_cast<T*>(zero(static_cast<void*>(ptr), count * sizeof(T)));
}

} // namespace fl
