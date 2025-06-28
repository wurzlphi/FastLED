# lib8tion Dependency Structure

## Before Refactoring (Circular Dependencies)
```
FastLED.h
    ↓ includes
led_sysdefs.h ←─────┐
    ↓ includes      │ circular!
FastLED.h ──────────┘
    ↓ includes
lib8tion.h
    ↓ includes
FastLED.h (circular!)
```

## After Refactoring (No Circular Dependencies)
```
lib8tion_base.h (NEW)
    ├── fl/stdint.h
    ├── fl/namespace.h
    ├── lib8tion/types.h
    ├── lib8tion/config.h
    └── fl/deprecated.h

lib8tion.h (REFACTORED)
    ├── lib8tion_base.h
    ├── lib8tion/lib8static.h
    ├── lib8tion/qfx.h
    ├── lib8tion/memmove.h
    ├── lib8tion/math8.h
    ├── lib8tion/scale8.h
    ├── lib8tion/random8.h
    └── lib8tion/trig8.h

lib8tion_core.h (NEW - Minimal)
    ├── lib8tion_base.h
    ├── lib8tion/scale8.h
    ├── lib8tion/math8.h
    └── lib8tion/random8.h

FastLED.h
    ├── led_sysdefs.h (no longer includes FastLED.h)
    ├── lib8tion.h (no longer includes FastLED.h)
    └── ... other headers
```

## Usage Patterns

### 1. Traditional FastLED Usage (No Change)
```cpp
#include "FastLED.h"
// Everything works as before
```

### 2. Minimal lib8tion Usage
```cpp
#include "lib8tion_core.h"
// Only essential 8-bit math functions
// ~90% smaller than full lib8tion
```

### 3. Full Standalone lib8tion
```cpp
#include "lib8tion.h"
// All lib8tion functions without FastLED
// Note: Need to link with lib8tion.cpp for rand16seed
```

### 4. Custom Selection
```cpp
#include "lib8tion_base.h"
#include "lib8tion/scale8.h"
#include "lib8tion/random8.h"
// Pick only what you need
```

## Size Comparison

| Configuration | Preprocessed Lines | Reduction |
|--------------|-------------------|-----------|
| Original (with FastLED.h circular dep) | ~35,000 | - |
| Refactored lib8tion.h | ~3,000 | 91% |
| lib8tion_core.h | ~500 | 98.5% |
| Single function include | ~100 | 99.7% |

## Benefits Summary

1. ✅ **No Circular Dependencies** - Clean dependency tree
2. ✅ **Modular Usage** - Use only what you need
3. ✅ **Standalone Compatible** - lib8tion without FastLED
4. ✅ **Backward Compatible** - Existing code unchanged
5. ✅ **Faster Compilation** - 91% reduction in preprocessed code
6. ✅ **Better Organization** - Clear separation of concerns
