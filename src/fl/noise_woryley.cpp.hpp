#include "noise_woryley.h"
#include "fl/int.h"

namespace fl {
namespace {

constexpr fl::i32 Q15_ONE = 32768; // 1.0 in Q15
// constexpr fl::i32 Q15_HALF = Q15_ONE / 2;

// Helper: multiply two Q15 numbers (result in Q15)
// fl::i32 q15_mul(fl::i32 a, fl::i32 b) {
//     return (fl::i32)(((int64_t)a * b) >> 15);
// }

// Helper: absolute difference
fl::i32 q15_abs(fl::i32 a) { return a < 0 ? -a : a; }

// Pseudo-random hash based on grid coordinates
fl::u16 hash(fl::i32 x, fl::i32 y) {
    fl::u32 n = (fl::u32)(x * 374761393 + y * 668265263);
    n = (n ^ (n >> 13)) * 1274126177;
    return (fl::u16)((n ^ (n >> 16)) & 0xFFFF);
}

// Get fractional feature point inside a grid cell
void feature_point(fl::i32 gx, fl::i32 gy, fl::i32 &fx, fl::i32 &fy) {
    fl::u16 h = hash(gx, gy);
    fx = (h & 0xFF) * 128; // scale to Q15 (0–32767)
    fy = ((h >> 8) & 0xFF) * 128;
}
} // namespace

// Compute 2D Worley noise at (x, y) in Q15
fl::i32 worley_noise_2d_q15(fl::i32 x, fl::i32 y) {
    fl::i32 cell_x = x >> 15;
    fl::i32 cell_y = y >> 15;

    fl::i32 min_dist = INT32_MAX;

    // Check surrounding 9 cells
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            fl::i32 gx = cell_x + dx;
            fl::i32 gy = cell_y + dy;

            fl::i32 fx, fy;
            feature_point(gx, gy, fx, fy);

            fl::i32 feature_x = (gx << 15) + fx;
            fl::i32 feature_y = (gy << 15) + fy;

            fl::i32 dx_q15 = x - feature_x;
            fl::i32 dy_q15 = y - feature_y;

            // Approximate distance using Manhattan (faster) or Euclidean
            // (costlier)
            fl::i32 dist =
                q15_abs(dx_q15) + q15_abs(dy_q15); // Manhattan distance

            if (dist < min_dist)
                min_dist = dist;
        }
    }

    // Normalize: maximum possible distance is roughly 2*Q15_ONE
    return (min_dist << 15) / (2 * Q15_ONE);
}

} // namespace fl
