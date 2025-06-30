
// g++ --std=c++11 test.cpp

#include "test.h"

#include "test.h"
#include "lib8tion/intmap.h"
#include "fl/lut.h"
#include "fl/int.h"


using namespace fl;

TEST_CASE("LUT interp8") {
    LUT<fl::u16> lut(2);
    fl::u16* data = lut.getDataMutable();
    data[0] = 0;
    data[1] = 255;
    CHECK_EQ(lut.interp8(0), 0);
    CHECK_EQ(lut.interp8(255), 255);
    CHECK_EQ(lut.interp8(128), 128);

    // Check the LUT interpolation for all values.
    for (fl::u16 i = 0; i < 256; i++) {
        fl::u16 expected = (i * 255) / 255;
        CHECK_EQ(lut.interp8(i), expected);
    }
}

TEST_CASE("LUT interp16") {
    LUT<fl::u16> lut(2);
    fl::u16* data = lut.getDataMutable();
    data[0] = 0;
    data[1] = 255;
    CHECK_EQ(lut.interp16(0), 0);
    CHECK_EQ(lut.interp16(0xffff), 255);
    CHECK_EQ(lut.interp16(0xffff / 2), 127);

    // Check the LUT interpolation for all values.
    for (int i = 0; i < 256; i++) {
        fl::u16 alpha16 = map8_to_16(i);
        CHECK_EQ(i, lut.interp16(alpha16));
    }
}