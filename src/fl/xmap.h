#pragma once

#include "fl/stdint.h"
#include "fl/int.h"
#include <string.h>

#include "fl/force_inline.h"
#include "fl/lut.h"
#include "fl/ptr.h"

#include "fl/namespace.h"

namespace fl {

FASTLED_FORCE_INLINE fl::u16 x_linear(fl::u16 x, fl::u16 length) {
    (void)length;
    return x;
}

FASTLED_FORCE_INLINE fl::u16 x_reverse(fl::u16 x, fl::u16 length) {
    return length - 1 - x;
}

// typedef for xMap function type
typedef fl::u16 (*XFunction)(fl::u16 x, fl::u16 length);

// XMap holds either a function or a look up table to map x coordinates to a 1D
// index.
class XMap {
  public:
    enum Type { kLinear = 0, kReverse, kFunction, kLookUpTable };

    static XMap constructWithUserFunction(fl::u16 length, XFunction xFunction,
                                          fl::u16 offset = 0);

    // When a pointer to a lookup table is passed in then we assume it's
    // owned by someone else and will not be deleted.
    static XMap constructWithLookUpTable(fl::u16 length,
                                         const fl::u16 *lookUpTable,
                                         fl::u16 offset = 0);

    // is_reverse is false by default for linear layout
    XMap(fl::u16 length, bool is_reverse = false, fl::u16 offset = 0);

    XMap(const XMap &other);

    // define the assignment operator
    XMap &operator=(const XMap &other);

    void convertToLookUpTable();

    fl::u16 mapToIndex(fl::u16 x) const;

    fl::u16 operator()(fl::u16 x) const { return mapToIndex(x); }

    fl::u16 getLength() const;

    Type getType() const;

  private:
    XMap(fl::u16 length, Type type);
    fl::u16 length = 0;
    Type type = kLinear;
    XFunction xFunction = nullptr;
    const fl::u16 *mData = nullptr;
    fl::LUT16Ptr mLookUpTable;
    fl::u16 mOffset = 0; // offset to be added to the output
};

} // namespace fl
