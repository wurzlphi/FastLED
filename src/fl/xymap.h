#pragma once

#include "fl/stdint.h"
#include <string.h>

#include "crgb.h"
#include "fl/clamp.h"
#include "fl/force_inline.h"
#include "fl/lut.h"
#include "fl/namespace.h"
#include "fl/ptr.h"
#include "fl/xmap.h" // Include xmap.h for LUT16
#include "fl/int.h"

namespace fl {
class ScreenMap;

FASTLED_FORCE_INLINE fl::u16 xy_serpentine(fl::u16 x, fl::u16 y,
                                            fl::u16 width, fl::u16 height) {
    (void)height;
    if (y & 1) // Even or odd row?
        // reverse every second line for a serpentine lled layout
        return (y + 1) * width - 1 - x;
    else
        return y * width + x;
}

FASTLED_FORCE_INLINE fl::u16 xy_line_by_line(fl::u16 x, fl::u16 y,
                                              fl::u16 width, fl::u16 height) {
    (void)height;
    return y * width + x;
}

// typedef for xyMap function type
typedef fl::u16 (*XYFunction)(fl::u16 x, fl::u16 y, fl::u16 width,
                               fl::u16 height);

// Maps x,y -> led index
//
// The common output led matrix you can buy on amazon is in a serpentine layout.
//
// XYMap allows you do to do graphic calculations on an LED layout as if it were
// a grid.
class XYMap {
  public:
    enum XyMapType { kSerpentine = 0, kLineByLine, kFunction, kLookUpTable };

    static XYMap constructWithUserFunction(fl::u16 width, fl::u16 height,
                                           XYFunction xyFunction,
                                           fl::u16 offset = 0);

    static XYMap constructRectangularGrid(fl::u16 width, fl::u16 height,
                                          fl::u16 offset = 0);

    static XYMap constructWithLookUpTable(fl::u16 width, fl::u16 height,
                                          const fl::u16 *lookUpTable,
                                          fl::u16 offset = 0);

    static XYMap constructSerpentine(fl::u16 width, fl::u16 height,
                                     fl::u16 offset = 0);

    static XYMap identity(fl::u16 width, fl::u16 height) {
        return constructRectangularGrid(width, height);
    }

    // is_serpentine is true by default. You probably want this unless you are
    // using a different layout
    XYMap(fl::u16 width, fl::u16 height, bool is_serpentine = true,
          fl::u16 offset = 0);

    XYMap(const XYMap &other) = default;
    XYMap &operator=(const XYMap &other) = default;

    fl::ScreenMap toScreenMap() const;

    void mapPixels(const CRGB *input, CRGB *output) const;

    void convertToLookUpTable();

    void setRectangularGrid();

    fl::u16 operator()(fl::u16 x, fl::u16 y) const {
        return mapToIndex(x, y);
    }

    fl::u16 mapToIndex(const fl::u16 &x, const fl::u16 &y) const;

    template <typename IntType,
              typename = fl::enable_if_t<!fl::is_integral<IntType>::value>>
    fl::u16 mapToIndex(IntType x, IntType y) const {
        x = fl::clamp<int>(x, 0, width - 1);
        y = fl::clamp<int>(y, 0, height - 1);
        return mapToIndex((fl::u16)x, (fl::u16)y);
    }

    bool has(fl::u16 x, fl::u16 y) const {
        return (x < width) && (y < height);
    }

    bool has(int x, int y) const {
        return (x >= 0) && (y >= 0) && has((fl::u16)x, (fl::u16)y);
    }

    bool isSerpentine() const { return type == kSerpentine; }
    bool isLineByLine() const { return type == kLineByLine; }
    bool isFunction() const { return type == kFunction; }
    bool isLUT() const { return type == kLookUpTable; }
    bool isRectangularGrid() const { return type == kLineByLine; }
    bool isSerpentineOrLineByLine() const {
        return type == kSerpentine || type == kLineByLine;
    }

    fl::u16 getWidth() const;
    fl::u16 getHeight() const;
    fl::u16 getTotal() const;
    XyMapType getType() const;

  private:
    XYMap(fl::u16 width, fl::u16 height, XyMapType type);

    XyMapType type;
    fl::u16 width;
    fl::u16 height;
    XYFunction xyFunction = nullptr;
    fl::LUT16Ptr mLookUpTable; // optional refptr to look up table.
    fl::u16 mOffset = 0;      // offset to be added to the output
};

} // namespace fl
