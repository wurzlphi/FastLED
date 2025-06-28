# FestivalStick Preprocessor Analysis Report

## Summary

The GCC preprocessor successfully processed the FestivalStick.ino file with the following results:
- Total preprocessed output: 47,102 lines (~1.2 MB)
- Total unique files included: 259 files
- FastLED library files: 139 files
- System/Standard library files: 120 files

## Include File Categories

### 1. FastLED Core Files (../../src/*.h)
The main FastLED library headers that provide core functionality:
- ../../src/bitswap.h
- ../../src/chipsets.h
- ../../src/chsv.h
- ../../src/cled_controller.h
- ../../src/color.h
- ../../src/colorpalettes.h
- ../../src/colorutils.h
- ../../src/controller.h
- ../../src/cpixel_ledcontroller.h
- ../../src/cpp_compat.h
- ../../src/crgb.h
- ../../src/dither_mode.h
- ../../src/dmx.h
- ../../src/eorder.h
- ../../src/fastled_config.h
- ../../src/fastled_delay.h
- ../../src/FastLED.h
- ../../src/fastled_progmem.h
- ../../src/fastpin.h
- ../../src/fastspi_bitbang.h
- ../../src/fastspi.h
- ../../src/fastspi_types.h
- ../../src/hsv2rgb.h
- ../../src/led_sysdefs.h
- ../../src/lib8tion.h
- ../../src/noise.h
- ../../src/pixel_controller.h
- ../../src/pixel_iterator.h
- ../../src/pixelset.h
- ../../src/pixeltypes.h
- ../../src/platforms.h
- ../../src/power_mgt.h
- ../../src/rgbw.h

### 2. FastLED fl/ Namespace Files (../../src/fl/*.h)
The new fl:: namespace utilities and abstractions:
- ../../src/fl/algorithm.h
- ../../src/fl/allocator.h
- ../../src/fl/array.h
- ../../src/fl/assert.h
- ../../src/fl/audio.h
- ../../src/fl/blur.h
- ../../src/fl/clamp.h
- ../../src/fl/clear.h
- ../../src/fl/colorutils.h
- ../../src/fl/colorutils_misc.h
- ../../src/fl/comparators.h
- ../../src/fl/compiler_control.h
- ../../src/fl/corkscrew.h
- ../../src/fl/dbg.h
- ../../src/fl/deprecated.h
- ../../src/fl/draw_mode.h
- ../../src/fl/ease.h
- ../../src/fl/engine_events.h
- ../../src/fl/fft.h
- ../../src/fl/fill.h
- ../../src/fl/five_bit_hd_gamma.h
- ../../src/fl/force_inline.h
- ../../src/fl/functional.h
- ../../src/fl/function.h
- ../../src/fl/function_list.h
- ../../src/fl/gamma.h
- ../../src/fl/geometry.h
- ../../src/fl/gradient.h
- ../../src/fl/grid.h
- ../../src/fl/initializer_list.h
- ../../src/fl/inplacenew.h
- ../../src/fl/insert_result.h
- ../../src/fl/leds.h
- ../../src/fl/lut.h
- ../../src/fl/map.h
- ../../src/fl/map_range.h
- ../../src/fl/math.h
- ../../src/fl/math_macros.h
- ../../src/fl/move.h
- ../../src/fl/namespace.h
- ../../src/fl/pair.h
- ../../src/fl/ptr.h
- ../../src/fl/random.h
- ../../src/fl/rbtree.h
- ../../src/fl/register.h
- ../../src/fl/scoped_ptr.h
- ../../src/fl/screenmap.h
- ../../src/fl/singleton.h
- ../../src/fl/sketch_macros.h
- ../../src/fl/slab_allocator.h
- ../../src/fl/slice.h
- ../../src/fl/sstream.h
- ../../src/fl/stdint.h
- ../../src/fl/str.h
- ../../src/fl/string.h
- ../../src/fl/strstream.h
- ../../src/fl/supersample.h
- ../../src/fl/template_magic.h
- ../../src/fl/tile2x2.h
- ../../src/fl/types.h
- ../../src/fl/type_traits.h
- ../../src/fl/ui.h
- ../../src/fl/ui_impl.h
- ../../src/fl/unused.h
- ../../src/fl/variant.h
- ../../src/fl/vector.h
- ../../src/fl/virtual_if_not_avr.h
- ../../src/fl/warn.h
- ../../src/fl/wave_simulation.h
- ../../src/fl/wave_simulation_real.h
- ../../src/fl/xmap.h
- ../../src/fl/xymap.h

### 3. Effects and Animation Files (../../src/fx/*)
Animation and effect-related headers:
- ../../src/fx/2d/animartrix_detail.hpp
- ../../src/fx/2d/animartrix.hpp
- ../../src/fx/2d/blend.h
- ../../src/fx/2d/wave.h
- ../../src/fx/detail/draw_context.h
- ../../src/fx/detail/fx_compositor.h
- ../../src/fx/detail/fx_layer.h
- ../../src/fx/detail/transition.h
- ../../src/fx/frame.h
- ../../src/fx/fx1d.h
- ../../src/fx/fx2d.h
- ../../src/fx/fx_engine.h
- ../../src/fx/fx.h
- ../../src/fx/time.h
- ../../src/fx/video.h

### 4. Platform and Hardware Support Files
Platform-specific implementations and hardware abstractions:
- ../../src/platforms/assert_defs.h
- ../../src/platforms/null_progmem.h
- ../../src/platforms/stub/clockless_stub_generic.h
- ../../src/platforms/stub/clockless_stub.h
- ../../src/platforms/stub/fastled_stub.h
- ../../src/platforms/stub/fastspi_stub_generic.h
- ../../src/platforms/stub/fastspi_stub.h
- ../../src/platforms/stub/led_sysdefs_stub_generic.h
- ../../src/platforms/stub/led_sysdefs_stub.h
- ../../src/platforms/ui_defs.h

- ../../src/lib8tion/brightness_bitshifter.h
- ../../src/lib8tion/config.h
- ../../src/lib8tion/intmap.h
- ../../src/lib8tion/lib8static.h
- ../../src/lib8tion/math8.h
- ../../src/lib8tion/memmove.h
- ../../src/lib8tion/qfx.h
- ../../src/lib8tion/random8.h
- ../../src/lib8tion/scale8.h
- ../../src/lib8tion/trig8.h
- ../../src/lib8tion/types.h

### 5. System and Standard Library Files
System headers automatically included by the preprocessor:
- /usr/include/c++/14/* - C++ standard library headers
- /usr/include/* - C system headers
- /usr/lib/gcc/* - GCC built-in headers

Total system files: 120 files

### 6. Local Example Files
Files from the FestivalStick example directory:
- FestivalStick.cpp - The main sketch file
- curr.h - Current implementation of the festival stick demo

## Key Observations

1. **No std:: usage**: The code correctly avoids using std:: namespace functions, instead using fl:: alternatives as per project guidelines.

2. **Heavy use of fl:: namespace**: The example extensively uses the new fl:: namespace features including:
   - fl::corkscrew - For LED mapping
   - fl::grid - For 2D grid operations
   - fl::screenmap - For web interface visualization
   - fl::ui - For web UI controls
   - fl::vector, fl::string - STL alternatives

3. **Effects framework**: The example uses multiple effects systems:
   - Wave effects (fx/2d/wave.h)
   - Animartrix animations (fx/2d/animartrix.hpp)
   - Blend operations (fx/2d/blend.h)
   - FX Engine (fx/fx_engine.h)

4. **Platform abstraction**: The code includes platform stubs for generic compilation, allowing it to be preprocessed on any system.

## Include Dependency Tree (Simplified)

```
FestivalStick.cpp
├── FastLED.h
│   ├── led_sysdefs.h
│   ├── fastled_config.h
│   ├── controller.h
│   ├── pixeltypes.h
│   ├── lib8tion.h
│   └── ... (many more core files)
└── curr.h
    ├── fl/corkscrew.h
    ├── fl/grid.h
    ├── fl/leds.h
    ├── fl/screenmap.h
    ├── fl/ui.h
    ├── fx/2d/wave.h
    ├── fx/2d/blend.h
    ├── fx/fx_engine.h
    └── fx/2d/animartrix.hpp
```

