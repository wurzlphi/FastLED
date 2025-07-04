#include "test.h"
#include "fx/fx_engine.h"
#include "crgb.h"

using namespace fl;

TEST_CASE("test_fx_engine_setfx") {
    constexpr uint16_t NUM_LEDS = 50;
    CRGB buffer[NUM_LEDS];

    SUBCASE("SetValidEffect") {
        FxEngine engine(NUM_LEDS);
        
        // Test setting a valid effect
        bool result = engine.setFx("cylon");
        CHECK(result);
        
        // Verify the effect is set by drawing a frame
        bool drawResult = engine.draw(0, buffer);
        CHECK(drawResult);
    }

    SUBCASE("SetInvalidEffect") {
        FxEngine engine(NUM_LEDS);
        
        // Test setting an invalid effect
        bool result = engine.setFx("nonexistent");
        CHECK_FALSE(result);
    }

    SUBCASE("SetNullEffect") {
        FxEngine engine(NUM_LEDS);
        
        // Test setting a null effect name
        bool result = engine.setFx(nullptr);
        CHECK_FALSE(result);
    }

    SUBCASE("SetEmptyEffect") {
        FxEngine engine(NUM_LEDS);
        
        // Test setting an empty effect name
        bool result = engine.setFx("");
        CHECK_FALSE(result);
    }

    SUBCASE("CaseInsensitiveMatching") {
        FxEngine engine(NUM_LEDS);
        
        // Test case insensitive matching
        CHECK(engine.setFx("CYLON"));
        CHECK(engine.setFx("Cylon"));
        CHECK(engine.setFx("cylon"));
        CHECK(engine.setFx("CyLoN"));
    }

    SUBCASE("AllAvailableEffects") {
        FxEngine engine(NUM_LEDS);
        
        // Test all available effects
        CHECK(engine.setFx("cylon"));
        CHECK(engine.setFx("fire2012"));
        CHECK(engine.setFx("pride2015"));
        CHECK(engine.setFx("demoreel100"));
        CHECK(engine.setFx("pacifica"));
        CHECK(engine.setFx("twinklefox"));
    }

    SUBCASE("EffectSwitching") {
        FxEngine engine(NUM_LEDS);
        
        // Test switching between effects
        CHECK(engine.setFx("cylon"));
        bool drawResult1 = engine.draw(0, buffer);
        CHECK(drawResult1);
        
        CHECK(engine.setFx("fire2012"));
        bool drawResult2 = engine.draw(100, buffer);
        CHECK(drawResult2);
        
        CHECK(engine.setFx("pride2015"));
        bool drawResult3 = engine.draw(200, buffer);
        CHECK(drawResult3);
    }

    SUBCASE("EffectPersistence") {
        FxEngine engine(NUM_LEDS);
        
        // Test that the effect persists after being set
        CHECK(engine.setFx("cylon"));
        
        // Draw multiple frames to ensure the effect is still active
        for (int i = 0; i < 10; i++) {
            bool drawResult = engine.draw(i * 100, buffer);
            CHECK(drawResult);
        }
    }
}
