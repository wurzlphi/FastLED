
#include "fl/stdint.h"
#include <string.h>

#include "fl/clamp.h"
#include "fl/force_inline.h"
#include "fl/namespace.h"
#include "fl/screenmap.h"
#include "fl/xymap.h"
#include "fl/int.h"

namespace fl {

ScreenMap XYMap::toScreenMap() const {
    const fl::u16 length = width * height;
    ScreenMap out(length);
    for (fl::u16 w = 0; w < width; w++) {
        for (fl::u16 h = 0; h < height; h++) {
            fl::u16 index = mapToIndex(w, h);
            vec2f p = {static_cast<float>(w), static_cast<float>(h)};
            out.set(index, p);
        }
    }
    return out;
}

XYMap XYMap::constructWithUserFunction(fl::u16 width, fl::u16 height,
                                       XYFunction xyFunction, fl::u16 offset) {
    XYMap out(width, height, kFunction);
    out.xyFunction = xyFunction;
    out.mOffset = offset;
    return out;
}

XYMap XYMap::constructRectangularGrid(fl::u16 width, fl::u16 height,
                                      fl::u16 offset) {
    XYMap out(width, height, kLineByLine);
    out.mOffset = offset;
    return out;
}

XYMap XYMap::constructWithLookUpTable(fl::u16 width, fl::u16 height,
                                      const fl::u16 *lookUpTable,
                                      fl::u16 offset) {
    XYMap out(width, height, kLookUpTable);
    out.mLookUpTable = LUT16Ptr::New(width * height);
    memcpy(out.mLookUpTable->getDataMutable(), lookUpTable,
           width * height * sizeof(fl::u16));
    out.mOffset = offset;
    return out;
}

XYMap XYMap::constructSerpentine(fl::u16 width, fl::u16 height,
                                 fl::u16 offset) {
    XYMap out(width, height, true);
    out.mOffset = offset;
    return out;
}

XYMap::XYMap(fl::u16 width, fl::u16 height, bool is_serpentine,
             fl::u16 offset)
    : type(is_serpentine ? kSerpentine : kLineByLine), width(width),
      height(height), mOffset(offset) {}

void XYMap::mapPixels(const CRGB *input, CRGB *output) const {
    fl::u16 pos = 0;
    for (fl::u16 y = 0; y < height; y++) {
        for (fl::u16 x = 0; x < width; x++) {
            fl::u16 i = pos++;
            output[i] = input[mapToIndex(x, y)];
        }
    }
}

void XYMap::convertToLookUpTable() {
    if (type == kLookUpTable) {
        return;
    }
    mLookUpTable = LUT16Ptr::New(width * height);
    fl::u16 *data = mLookUpTable->getDataMutable();
    for (fl::u16 y = 0; y < height; y++) {
        for (fl::u16 x = 0; x < width; x++) {
            data[y * width + x] = mapToIndex(x, y);
        }
    }
    type = kLookUpTable;
    xyFunction = nullptr;
}

void XYMap::setRectangularGrid() {
    type = kLineByLine;
    xyFunction = nullptr;
    mLookUpTable.reset();
}

fl::u16 XYMap::mapToIndex(const fl::u16 &x, const fl::u16 &y) const {
    fl::u16 index;
    switch (type) {
    case kSerpentine: {
        fl::u16 xx = x % width;
        fl::u16 yy = y % height;
        index = xy_serpentine(xx, yy, width, height);
        break;
    }
    case kLineByLine: {
        fl::u16 xx = x % width;
        fl::u16 yy = y % height;
        index = xy_line_by_line(xx, yy, width, height);
        break;
    }
    case kFunction:
        index = xyFunction(x, y, width, height);
        break;
    case kLookUpTable:
        index = mLookUpTable->getData()[y * width + x];
        break;
    default:
        return 0;
    }
    return index + mOffset;
}

fl::u16 XYMap::getWidth() const { return width; }

fl::u16 XYMap::getHeight() const { return height; }

fl::u16 XYMap::getTotal() const { return width * height; }

XYMap::XyMapType XYMap::getType() const { return type; }

XYMap::XYMap(fl::u16 width, fl::u16 height, XyMapType type)
    : type(type), width(width), height(height), mOffset(0) {}

} // namespace fl
