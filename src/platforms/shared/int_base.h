#pragma once

// Eventually these will go away.
#include <stdint.h>  // ok include
#include <stddef.h>  // ok include

namespace fl {
    // 8-bit types - char is reliably 8 bits on all supported platforms
    typedef signed char i8;
    typedef unsigned char u8;
    
    // Platform-specific 16/32/64-bit types are defined in platform-specific files
    // that include this file
    
    // Pointer and size types - universal across platforms
    typedef uintptr_t uptr;  ///< Pointer-sized unsigned integer
    typedef size_t sz;       ///< Size type for containers and memory
    
    // Compile-time verification that types are exactly the expected size
    static_assert(sizeof(i8) == 1, "i8 must be exactly 1 byte");
    static_assert(sizeof(u8) == 1, "u8 must be exactly 1 byte");
    static_assert(sizeof(uptr) >= sizeof(void*), "uptr must be at least pointer size");
    static_assert(sizeof(sz) >= sizeof(void*), "sz must be at least pointer size for large memory operations");
}

// Platform-specific size assertions for i16, i32, i64, u16, u32, u64 
// are included after the platform-specific typedefs
