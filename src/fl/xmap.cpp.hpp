

#include "fl/xmap.h"
#include "fl/int.h"

namespace fl {

XMap XMap::constructWithUserFunction(fl::u16 length, XFunction xFunction,
                                     fl::u16 offset) {
    XMap out = XMap(length, kFunction);
    out.xFunction = xFunction;
    out.mOffset = offset;
    return out;
}

XMap XMap::constructWithLookUpTable(fl::u16 length,
                                    const fl::u16 *lookUpTable,
                                    fl::u16 offset) {
    XMap out = XMap(length, kLookUpTable);
    out.mData = lookUpTable;
    out.mOffset = offset;
    return out;
}

XMap::XMap(fl::u16 length, bool is_reverse, fl::u16 offset) {
    type = is_reverse ? kReverse : kLinear;
    this->length = length;
    this->mOffset = offset;
}

XMap::XMap(const XMap &other) {
    type = other.type;
    length = other.length;
    xFunction = other.xFunction;
    mData = other.mData;
    mLookUpTable = other.mLookUpTable;
    mOffset = other.mOffset;
}

void XMap::convertToLookUpTable() {
    if (type == kLookUpTable) {
        return;
    }
    mLookUpTable.reset();
    mLookUpTable = LUT16Ptr::New(length);
    fl::u16 *dataMutable = mLookUpTable->getDataMutable();
    mData = mLookUpTable->getData();
    for (fl::u16 x = 0; x < length; x++) {
        dataMutable[x] = mapToIndex(x);
    }
    type = kLookUpTable;
    xFunction = nullptr;
}

fl::u16 XMap::mapToIndex(fl::u16 x) const {
    fl::u16 index;
    switch (type) {
    case kLinear:
        index = x_linear(x, length);
        break;
    case kReverse:
        index = x_reverse(x, length);
        break;
    case kFunction:
        x = x % length;
        index = xFunction(x, length);
        break;
    case kLookUpTable:
        index = mData[x];
        break;
    default:
        return 0;
    }
    return index + mOffset;
}

fl::u16 XMap::getLength() const { return length; }

XMap::Type XMap::getType() const { return type; }

XMap::XMap(fl::u16 length, Type type)
    : length(length), type(type), mOffset(0) {}

XMap &XMap::operator=(const XMap &other) {
    if (this != &other) {
        type = other.type;
        length = other.length;
        xFunction = other.xFunction;
        mData = other.mData;
        mLookUpTable = other.mLookUpTable;
        mOffset = other.mOffset;
    }
    return *this;
}

} // namespace fl
