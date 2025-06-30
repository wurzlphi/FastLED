#pragma once

#include "FastLED.h"
#include "fl/namespace.h"
#include "fx/fx1d.h"
#include "fl/int.h"

namespace fl {

/// @file    pride2015.hpp
/// @brief   Animated, ever-changing rainbows (Pride2015 effect)
/// @example Pride2015.ino

// Pride2015
// Animated, ever-changing rainbows.
// by Mark Kriegsman

FASTLED_SMART_PTR(Pride2015);

class Pride2015 : public Fx1d {
  public:
    Pride2015(fl::u16 num_leds) : Fx1d(num_leds) {}

    void draw(Fx::DrawContext context) override;
    fl::string fxName() const override { return "Pride2015"; }

  private:
    fl::u16 mPseudotime = 0;
    fl::u16 mLastMillis = 0;
    fl::u16 mHue16 = 0;
};

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void Pride2015::draw(Fx::DrawContext ctx) {
    if (ctx.leds == nullptr || mNumLeds == 0) {
        return;
    }

    uint8_t sat8 = beatsin88(87, 220, 250);
    uint8_t brightdepth = beatsin88(341, 96, 224);
    fl::u16 brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
    uint8_t msmultiplier = beatsin88(147, 23, 60);

    fl::u16 hue16 = mHue16;
    fl::u16 hueinc16 = beatsin88(113, 1, 3000);

    fl::u16 ms = millis();
    fl::u16 deltams = ms - mLastMillis;
    mLastMillis = ms;
    mPseudotime += deltams * msmultiplier;
    mHue16 += deltams * beatsin88(400, 5, 9);
    fl::u16 brightnesstheta16 = mPseudotime;

    // set master brightness control
    for (fl::u16 i = 0; i < mNumLeds; i++) {
        hue16 += hueinc16;
        uint8_t hue8 = hue16 / 256;

        brightnesstheta16 += brightnessthetainc16;
        fl::u16 b16 = sin16(brightnesstheta16) + 32768;

        fl::u16 bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
        uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
        bri8 += (255 - brightdepth);

        CRGB newcolor = CHSV(hue8, sat8, bri8);

        fl::u16 pixelnumber = (mNumLeds - 1) - i;

        nblend(ctx.leds[pixelnumber], newcolor, 64);
    }
}

} // namespace fl
