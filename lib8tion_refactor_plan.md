# lib8tion.h Refactoring Plan: Breaking FastLED.h Dependency

## Problem Analysis

lib8tion.h currently has a circular dependency with FastLED.h:
- lib8tion.h → includes → FastLED.h
- FastLED.h → includes → lib8tion.h
- Additionally: led_sysdefs.h → includes → FastLED.h

This creates compilation bloat and maintenance issues.

## Dependencies lib8tion.h Actually Needs

1. **Namespace macros**: `FASTLED_NAMESPACE_BEGIN/END` (from fl/namespace.h)
2. **Platform detection**: To ensure led_sysdefs.h was included (but not the file itself)
3. **Basic types**: uint8_t, uint16_t, etc. (from fl/stdint.h)
4. **Library configuration**: From lib8tion/config.h
5. **Platform-specific timer function**: millis() or get_millisecond_timer()

## Refactoring Steps

### Step 1: Create lib8tion_base.h
```cpp
// src/lib8tion_base.h
#pragma once

// Only include what we absolutely need
#include "fl/stdint.h"
#include "fl/namespace.h"
#include "lib8tion/types.h"
#include "lib8tion/config.h"
#include "fl/deprecated.h"

// Platform detection without including led_sysdefs.h
#if defined(ARDUINO) || defined(SPARK) || defined(FASTLED_HAS_MILLIS)
  #define FASTLED_LIB8TION_USE_MILLIS
#endif

// Define GET_MILLIS without needing FastLED.h
#ifdef FASTLED_LIB8TION_USE_MILLIS
  #define GET_MILLIS millis
#else
  uint32_t get_millisecond_timer();
  #define GET_MILLIS get_millisecond_timer
#endif
```

### Step 2: Modify lib8tion.h Header
```cpp
// src/lib8tion.h
#pragma once
#ifndef __INC_LIB8TION_H
#define __INC_LIB8TION_H

// Remove: #include "FastLED.h"
// Add instead:
#include "lib8tion_base.h"

// Remove the led_sysdefs check since it creates circular dependency
// #ifndef __INC_LED_SYSDEFS_H
// #error WTH?  led_sysdefs needs to be included first
// #endif

// Keep all the existing includes
#include "lib8tion/lib8static.h"
#include "lib8tion/qfx.h"
#include "lib8tion/memmove.h"
#include "fl/ease.h"
// ... rest of includes
```

### Step 3: Update led_sysdefs.h
```cpp
// src/led_sysdefs.h
#pragma once
#ifndef __INC_LED_SYSDEFS_H
#define __INC_LED_SYSDEFS_H

// Remove: #include "FastLED.h"
// Add only what's needed:
#include "fastled_config.h"
#include "fl/namespace.h"

// ... rest of platform detection code stays the same
```

### Step 4: Update FastLED.h
```cpp
// src/FastLED.h
// Ensure led_sysdefs.h is included before lib8tion.h
#include "led_sysdefs.h"
#include "lib8tion.h"
// ... rest of includes
```

### Step 5: Create lib8tion_all.h for Convenience
```cpp
// src/lib8tion_all.h
#pragma once
// For users who want all lib8tion features without FastLED
#include "lib8tion_base.h"
#include "lib8tion.h"
```

## Alternative Approach: Minimal lib8tion_core.h

Create a truly minimal lib8tion that only includes the most essential functions:

```cpp
// src/lib8tion_core.h
#pragma once

#include "fl/stdint.h"
#include "fl/namespace.h"
#include "lib8tion/types.h"

FASTLED_NAMESPACE_BEGIN

// Only include the most commonly used functions
#include "lib8tion/scale8.h"
#include "lib8tion/math8.h"

FASTLED_NAMESPACE_END
```

## Benefits

1. **Breaks circular dependency** between FastLED.h and lib8tion.h
2. **Reduces preprocessor expansion** by ~35,000 lines
3. **Faster compilation** for files that only need lib8tion
4. **Better modularity** - lib8tion can be used standalone
5. **Maintains backward compatibility** - existing code still works

## Implementation Order

1. Create lib8tion_base.h with minimal dependencies
2. Test that lib8tion.h works with the new base
3. Update led_sysdefs.h to remove FastLED.h dependency
4. Update inclusion order in FastLED.h
5. Create lib8tion_core.h for minimal builds
6. Test all examples to ensure compatibility

## Testing Strategy

1. Compile all examples to ensure no breakage
2. Create standalone test that uses only lib8tion without FastLED
3. Measure compilation time improvements
4. Test on multiple platforms (AVR, ESP32, Teensy)

## Potential Issues

1. Some platforms may depend on Arduino.h being included via FastLED.h
   - Solution: Add conditional Arduino.h include in lib8tion_base.h
   
2. GET_MILLIS may not work on all platforms without full led_sysdefs.h
   - Solution: Provide platform-specific timer implementations

3. Namespace macros might need additional configuration
   - Solution: Ensure fl/namespace.h is self-contained
