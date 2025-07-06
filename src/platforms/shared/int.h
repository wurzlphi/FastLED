#pragma once

// Include shared base definitions
#include "platforms/shared/int_base.h"

namespace fl {
    // Default platform (desktop/generic): assume short 16-bit, int 32-bit (uint32_t is unsigned int)
    typedef short i16;
    typedef unsigned short u16;
    typedef int i32;             // 'int' is 32-bit on all desktop and most embedded targets
    typedef unsigned int u32;
    typedef long long i64;
    typedef unsigned long long u64;
}

// Include size assertions after platform-specific typedefs
#include "platforms/shared/int_size_assertions.h"

// Include fractional types after platform-specific typedefs
#include "platforms/shared/int_fractional_types.h"
