
// g++ --std=c++11 test.cpp

#include "test.h"

#include "test.h"
#include "lib8tion/intmap.h"
#include "fl/transform.h"
#include "fl/vector.h"
#include "fl/unused.h"
#include <string>
#include "fl/int.h"

using namespace fl;


TEST_CASE("Transform16::ToBounds(max_value)") {
    // Transform16 tx = Transform16::ToBounds(255);

    SUBCASE("Check bounds at 128") {
        // known bad at i == 128
        Transform16 tx = Transform16::ToBounds(255);
        fl::u16 i16 = map8_to_16(128);
        vec2<fl::u16> xy_input = vec2<fl::u16>(i16, i16);
        vec2<fl::u16> xy = tx.transform(xy_input);
        INFO("i = " << 128);
        REQUIRE_EQ(128, xy.x);
        REQUIRE_EQ(128, xy.y);
    }

    SUBCASE("Check identity from 8 -> 16") {
        Transform16 tx = Transform16::ToBounds(255);
        for (fl::u16 i = 0; i < 256; i++) {
            fl::u16 i16 = map8_to_16(i);
            vec2<fl::u16> xy_input = vec2<fl::u16>(i16, i16);
            vec2<fl::u16> xy = tx.transform(xy_input);
            INFO("i = " << i);
            REQUIRE_EQ(i, xy.x);
            REQUIRE_EQ(i, xy.y);
        }
    }

    SUBCASE("Check all bounds are in 255") {
        Transform16 tx = Transform16::ToBounds(255);
        uint32_t smallest = ~0;
        uint32_t largest = 0;
        for (fl::u16 i = 0; i < 256; i++) {
            fl::u16 i16 = map8_to_16(i);
            vec2<fl::u16> xy_input = vec2<fl::u16>(i16, i16);
            vec2<fl::u16> xy = tx.transform(xy_input);
            INFO("i = " << i);
            REQUIRE_LE(xy.x, 255);
            REQUIRE_LE(xy.y, 255);
            smallest = MIN(smallest, xy.x);
            largest = MAX(largest, xy.x);
        }

        REQUIRE_EQ(0, smallest);
        REQUIRE_EQ(255, largest);
    }
}

TEST_CASE("Transform16::ToBounds(min, max)") {
    SUBCASE("Check bounds at 128") {
        fl::u16 low = 127;
        fl::u16 high = 255 + 127;
        vec2<fl::u16> min = vec2<fl::u16>(low, low);
        vec2<fl::u16> max = vec2<fl::u16>(high, high);
        Transform16 tx = Transform16::ToBounds(min, max);
        auto t1 = tx.transform(vec2<fl::u16>(0, 0));
        auto t2 = tx.transform(vec2<fl::u16>(0xffff, 0xffff));
        REQUIRE_EQ(vec2<fl::u16>(low, low), t1);
        REQUIRE_EQ(vec2<fl::u16>(high, high), t2);
    }
}
