#pragma once

// Include shared base definitions
#include "platforms/shared/int_base.h"

namespace fl {
    // ARM platforms (32-bit): short is 16-bit, long is 32-bit
    // uint32_t resolves to 'unsigned long' on most ARM toolchains
    typedef short i16;
    typedef unsigned short u16;
    typedef long i32;
    typedef unsigned long u32;
    typedef long long i64;
    typedef unsigned long long u64;
}

// Include size assertions after platform-specific typedefs
#include "platforms/shared/int_size_assertions.h"

// Include fractional types after platform-specific typedefs
#include "platforms/shared/int_fractional_types.h"
