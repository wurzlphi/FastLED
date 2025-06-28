#pragma once

/// @file lib8tion_core.h
/// Minimal lib8tion include with only the most essential math functions.
/// Use this for projects that need basic 8-bit math without the full lib8tion overhead.

#include "lib8tion_base.h"

FASTLED_NAMESPACE_BEGIN

// Include only the most commonly used lib8tion functions
#include "lib8tion/scale8.h"
#include "lib8tion/math8.h"
#include "lib8tion/random8.h"

// Note: The basic functions (abs8, mul8, add8, sub8) are already 
// provided by math8.h, so we don't need to redefine them here.

FASTLED_NAMESPACE_END
