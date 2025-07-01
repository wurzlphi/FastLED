#include "fl/tile2x2.h"
#include "fl/int.h"
#include "crgb.h"
#include "fl/draw_visitor.h"
#include "fl/math_macros.h"
#include "fl/raster.h"
#include "fl/raster_sparse.h"
#include "fl/unused.h"
#include "fl/warn.h"
#include "fl/xymap.h"
#include "fl/vector.h"

namespace fl {

namespace {
static vec2i16 wrap(const vec2i16 &v, const vec2i16 &size) {
    // Wrap the vector v around the size
    return vec2i16(v.x % size.x, v.y % size.y);
}

static vec2i16 wrap_x(const vec2i16 &v, const fl::u16 width) {
    // Wrap the x component of the vector v around the size
    return vec2i16(v.x % width, v.y);
}
} // namespace

Tile2x2_u8_wrap::Tile2x2_u8_wrap() {
    mData[0][0] = {vec2i16(0, 0), 0};
    mData[0][1] = {vec2i16(0, 1), 0};
    mData[1][0] = {vec2i16(1, 0), 0};
    mData[1][1] = {vec2i16(1, 1), 0};
}

void Tile2x2_u8::Rasterize(const span<const Tile2x2_u8> &tiles,
                           XYRasterU8Sparse *out_raster) {
    out_raster->rasterize(tiles);
}

void Tile2x2_u8::draw(const CRGB &color, const XYMap &xymap, CRGB *out) const {
    XYDrawComposited visitor(color, xymap, out);
    draw(xymap, visitor);
}

void Tile2x2_u8::scale(fl::u8 scale) {
    // scale the tile
    if (scale == 255) {
        return;
    }
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            fl::u16 value = at(x, y);
            at(x, y) = (value * scale) >> 8;
        }
    }
}

Tile2x2_u8_wrap::Tile2x2_u8_wrap(const Tile2x2_u8 &from, fl::u16 width) {
    const vec2i16 origin = from.origin();
    at(0, 0) = {wrap_x(vec2i16(origin.x, origin.y), width), from.at(0, 0)};
    at(0, 1) = {wrap_x(vec2i16(origin.x, origin.y + 1), width), from.at(0, 1)};
    at(1, 0) = {wrap_x(vec2i16(origin.x + 1, origin.y), width), from.at(1, 0)};
    at(1, 1) = {wrap_x(vec2i16(origin.x + 1, origin.y + 1), width),
                from.at(1, 1)};
}

Tile2x2_u8_wrap::Entry &Tile2x2_u8_wrap::at(fl::u16 x, fl::u16 y) {
    // Wrap around the edges
    x = (x + 2) % 2;
    y = (y + 2) % 2;
    return mData[y][x];
}


Tile2x2_u8_wrap::Tile2x2_u8_wrap(const Data& data) {
    mData[0][0] = data[0][0];
    mData[0][1] = data[0][1];
    mData[1][0] = data[1][0];
    mData[1][1] = data[1][1];
}


const Tile2x2_u8_wrap::Entry &Tile2x2_u8_wrap::at(fl::u16 x, fl::u16 y) const {
    // Wrap around the edges
    x = (x + 2) % 2;
    y = (y + 2) % 2;
    return mData[y][x];
}

Tile2x2_u8_wrap::Tile2x2_u8_wrap(const Tile2x2_u8 &from, fl::u16 width,
                                 fl::u16 height) {
    const vec2i16 origin = from.origin();
    at(0, 0) = {wrap(vec2i16(origin.x, origin.y), vec2i16(width, height)),
                from.at(0, 0)};
    at(0, 1) = {wrap(vec2i16(origin.x, origin.y + 1), vec2i16(width, height)),
                from.at(0, 1)};
    at(1, 0) = {wrap(vec2i16(origin.x + 1, origin.y), vec2i16(width, height)),
                from.at(1, 0)};
    at(1,
       1) = {wrap(vec2i16(origin.x + 1, origin.y + 1), vec2i16(width, height)),
             from.at(1, 1)};
}

fl::u8 Tile2x2_u8::maxValue() const {
    fl::u8 max = 0;
    max = MAX(max, at(0, 0));
    max = MAX(max, at(0, 1));
    max = MAX(max, at(1, 0));
    max = MAX(max, at(1, 1));
    return max;
}

Tile2x2_u8 Tile2x2_u8::MaxTile(const Tile2x2_u8 &a, const Tile2x2_u8 &b) {
    Tile2x2_u8 result;
    for (int x = 0; x < 2; ++x) {
        for (int y = 0; y < 2; ++y) {
            result.at(x, y) = MAX(a.at(x, y), b.at(x, y));
        }
    }
    return result;
}

rect<fl::i16> Tile2x2_u8::bounds() const {
    vec2<fl::i16> min = mOrigin;
    vec2<fl::i16> max = mOrigin + vec2<fl::i16>(2, 2);
    return rect<fl::i16>(min, max);
}

fl::vector_fixed<Tile2x2_u8_wrap, 2> Tile2x2_u8_wrap::Interpolate(const Tile2x2_u8_wrap& a, const Tile2x2_u8_wrap& b, float t) {
    fl::vector_fixed<Tile2x2_u8_wrap, 2> result;
    
    // Clamp t to [0, 1]
    if (t <= 0.0f) {
        result.push_back(a);
        return result;
    }
    if (t >= 1.0f) {
        result.push_back(b);
        return result;
    }
    
    // Create interpolated tile
    Tile2x2_u8_wrap interpolated;
    
    // Interpolate each of the 4 positions
    for (fl::u16 x = 0; x < 2; ++x) {
        for (fl::u16 y = 0; y < 2; ++y) {
            const auto& data_a = a.at(x, y);
            const auto& data_b = b.at(x, y);
            
            // For now, assume positions are the same or close enough
            // Use position from 'a' as the base
            vec2i16 pos = data_a.first;
            
            // Simple linear interpolation for alpha values
            fl::u8 alpha_a = data_a.second;
            fl::u8 alpha_b = data_b.second;
            
            // Linear interpolation: a + t * (b - a)
            float alpha_float = alpha_a + t * (alpha_b - alpha_a);
            fl::u8 interpolated_alpha = static_cast<fl::u8>(alpha_float + 0.5f); // Round to nearest
            
            interpolated.mData[y][x] = {pos, interpolated_alpha};
        }
    }
    
    result.push_back(interpolated);
    return result;
}

} // namespace fl
