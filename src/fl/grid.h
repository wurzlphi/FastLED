
#pragma once

#include "fl/span.h"
#include "fl/vector.h"
#include "fl/int.h"

namespace fl {


template <typename T> class Grid {
  public:
    Grid() = default;

    Grid(fl::u32 width, fl::u32 height) { reset(width, height); }

    void reset(fl::u32 width, fl::u32 height) {
        clear();
        if (width != mWidth || height != mHeight) {
            mWidth = width;
            mHeight = height;
            mData.resize(width * height);

        }
        mSlice = fl::MatrixSlice<T>(mData.data(), width, height, 0, 0,
                                    width, height);
    }


    void clear() {
        for (fl::u32 i = 0; i < mWidth * mHeight; ++i) {
            mData[i] = T();
        }
    }

    vec2<T> minMax() const {
        T minValue = mData[0];
        T maxValue = mData[0];
        for (fl::u32 i = 1; i < mWidth * mHeight; ++i) {
            if (mData[i] < minValue) {
                minValue = mData[i];
            }
            if (mData[i] > maxValue) {
                maxValue = mData[i];
            }
        }
        // *min = minValue;
        // *max = maxValue;
        vec2<T> out(minValue, maxValue);
        return out;
    }

    T &at(fl::u32 x, fl::u32 y) { return access(x, y); }
    const T &at(fl::u32 x, fl::u32 y) const { return access(x, y); }

    T &operator()(fl::u32 x, fl::u32 y) { return at(x, y); }
    const T &operator()(fl::u32 x, fl::u32 y) const { return at(x, y); }

    fl::u32 width() const { return mWidth; }
    fl::u32 height() const { return mHeight; }

    T* data() { return mData.data(); }
    const T* data() const { return mData.data(); }

    size_t size() const { return mData.size(); }

  private:
    static T &NullValue() {
        static T gNull;
        return gNull;
    }
    T &access(fl::u32 x, fl::u32 y) {
        if (x < mWidth && y < mHeight) {
            return mSlice.at(x, y);
        } else {
            return NullValue(); // safe.
        }
    }
    const T &access(fl::u32 x, fl::u32 y) const {
        if (x < mWidth && y < mHeight) {
            return mSlice.at(x, y);
        } else {
            return NullValue(); // safe.
        }
    }
    fl::vector<T> mData;
    fl::u32 mWidth = 0;
    fl::u32 mHeight = 0;
    fl::MatrixSlice<T> mSlice;
};

} // namespace fl
