#pragma once

#include "fl/stdint.h"
#include "fl/int.h"

namespace fl {

// Compute 2D Worley noise at (x, y) in Q15
fl::i32 worley_noise_2d_q15(fl::i32 x, fl::i32 y);

} // namespace fl
