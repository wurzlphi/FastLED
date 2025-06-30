/// @file    noisepalette.h
/// @brief   Demonstrates how to mix noise generation with color palettes on a
/// 2D LED matrix

#pragma once

#include "fl/stdint.h"

#include "FastLED.h"
#include "fl/ptr.h"
#include "fl/xymap.h"
#include "fx/fx2d.h"
#include "fx/time.h"
#include "lib8tion/random8.h"
#include "noise.h"
#include "fl/int.h"

namespace fl {

FASTLED_SMART_PTR(NoisePalette);

class NoisePalette : public Fx2d {
  public:
    // Fps is used by the fx_engine to maintain a fixed frame rate, ignored
    // otherwise.
    NoisePalette(XYMap xyMap, float fps = 60.f);

    bool hasFixedFrameRate(float *fps) const override {
        *fps = mFps;
        return true;
    }

    // No need for a destructor, scoped_ptr will handle memory deallocation

    void draw(DrawContext context) override {
        fillnoise8();
        mapNoiseToLEDsUsingPalette(context.leds);
    }

    Str fxName() const override { return "NoisePalette"; }
    void mapNoiseToLEDsUsingPalette(CRGB *leds);

    uint8_t changeToRandomPalette();

    // There are 12 palette indexes but they don't have names. Use this to set
    // which one you want.
    uint8_t getPalettePresetCount() const { return 12; }
    uint8_t getPalettePreset() const { return currentPaletteIndex; }
    void setPalettePreset(int paletteIndex);
    void setPalette(const CRGBPalette16 &palette, fl::u16 speed,
                    fl::u16 scale, bool colorLoop) {
        currentPalette = palette;
        this->speed = speed;
        this->scale = scale;
        this->colorLoop = colorLoop;
    }
    void setSpeed(fl::u16 speed) { this->speed = speed; }
    void setScale(fl::u16 scale) { this->scale = scale; }

  private:
    fl::u16 mX, mY, mZ;
    fl::u16 width, height;
    fl::u16 speed = 0;
    fl::u16 scale = 0;
    fl::vector<uint8_t, fl::allocator_psram<uint8_t>> noise;
    CRGBPalette16 currentPalette;
    bool colorLoop = 0;
    int currentPaletteIndex = 0;
    float mFps = 60.f;

    void fillnoise8();

    fl::u16 XY(uint8_t x, uint8_t y) const { return mXyMap.mapToIndex(x, y); }

    void SetupRandomPalette() {
        CRGBPalette16 newPalette;
        do {
            newPalette = CRGBPalette16(
                CHSV(random8(), 255, 32), CHSV(random8(), 255, 255),
                CHSV(random8(), 128, 255), CHSV(random8(), 255, 255));
        } while (newPalette == currentPalette);
        currentPalette = newPalette;
    }

    void SetupBlackAndWhiteStripedPalette() {
        fill_solid(currentPalette, 16, CRGB::Black);
        currentPalette[0] = CRGB::White;
        currentPalette[4] = CRGB::White;
        currentPalette[8] = CRGB::White;
        currentPalette[12] = CRGB::White;
    }

    void SetupPurpleAndGreenPalette() {
        CRGB purple = CHSV(HUE_PURPLE, 255, 255);
        CRGB green = CHSV(HUE_GREEN, 255, 255);
        CRGB black = CRGB::Black;

        currentPalette = CRGBPalette16(
            green, green, black, black, purple, purple, black, black, green,
            green, black, black, purple, purple, black, black);
    }
};

} // namespace fl
