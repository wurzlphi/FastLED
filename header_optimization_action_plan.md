# FastLED Header Optimization Action Plan

## Quick Wins (1-2 days each)

### 1. Create forward declaration headers
```cpp
// src/fl/colorutils_fwd.h
#pragma once
struct CRGB;
struct CHSV;
class CRGBPalette16;
class CHSVPalette16;
// ... other forward declarations
```

### 2. Split lib8tion.h
```cpp
// src/lib8tion_core.h - Essential functions only
#include "lib8tion/types.h"
#include "lib8tion/scale8.h"  // Only the most used functions

// src/lib8tion_extended.h - Everything else
#include "lib8tion_core.h"
#include "lib8tion/math8.h"
#include "lib8tion/trig8.h"
// ... other includes
```

### 3. Reduce FastLED.h dependencies
- Move controller instantiation templates to separate header
- Include only essential headers by default
- Create FastLED_all.h for backward compatibility

## Medium-term Tasks (1 week each)

### 1. Modularize chipsets.h
```
src/chipsets/
├── apa102.h
├── ws2812.h
├── ws2801.h
├── lpd8806.h
└── chipsets_all.h  // Includes all chipsets
```

### 2. Refactor colorutils.h
```
src/fl/
├── colorutils_core.h      // Basic color functions
├── palette_types.h        // Palette class definitions
├── palette_functions.h    // ColorFromPalette, etc.
├── colorutils_blending.h  // Blending functions
└── colorutils.h          // Backward compatibility wrapper
```

## Long-term Improvements (2-4 weeks)

### 1. Template implementation separation
- Move all template implementations to .tpp files
- Include .tpp files at the end of headers
- Allows better organization and selective inclusion

### 2. Platform-specific optimizations
- Create platform-specific subdirectories
- Move inline assembly to platform-specific files
- Reduce conditional compilation in headers

### 3. Precompiled header support
- Create fastled_pch.h with common includes
- Add CMake/build system support for PCH
- Document usage for different platforms

## Measurement Metrics

Track improvements using:
1. Preprocessed line count
2. Compilation time (clean build)
3. Object file sizes
4. Include dependency depth

## Testing Strategy

1. Create compilation tests for each refactored header
2. Ensure examples still compile without changes
3. Benchmark compilation time improvements
4. Test on multiple platforms (AVR, ESP32, Teensy, etc.)
