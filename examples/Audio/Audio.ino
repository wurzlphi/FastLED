/// @file    Audio.ino
/// @brief   Audio visualization example with XY mapping
/// @example Audio.ino
///
/// This sketch is fully compatible with the FastLED web compiler. To use it do the following:
/// 1. Install Fastled: `pip install fastled`
/// 2. cd into this examples page.  
/// 3. Run the FastLED web compiler at root: `fastled`
/// 4. When the compiler is done a web page will open.

/*
This demo is best viewed using the FastLED compiler.

Windows/MacOS binaries: https://github.com/FastLED/FastLED/releases

Python

Install: pip install fastled
Run: fastled <this sketch directory>
This will compile and preview the sketch in the browser, and enable
all the UI elements you see below.
*/

#include <Arduino.h>
#include <FastLED.h>

#if !SKETCH_HAS_LOTS_OF_MEMORY
// Platform does not have enough memory
void setup() {}
void loop() {}
#else

#include "fl/audio.h"
#include "fl/downscale.h"
#include "fl/draw_visitor.h"
#include "fl/fft.h"
#include "fl/math.h"
#include "fl/math_macros.h"
#include "fl/raster.h"
#include "fl/time_alpha.h"
#include "fl/ui.h"
#include "fl/xypath.h"
#include "fl/unused.h"
#include "fx/time.h"
#include "fl/function.h"

// Sketch.
#include "fx_audio.h"

using namespace fl;

#define HEIGHT 128
#define WIDTH 128
#define NUM_LEDS ((WIDTH) * (HEIGHT))
#define IS_SERPINTINE false
#define TIME_ANIMATION 1000 // ms
#define PIN_DATA 3

// Main UI Controls
UITitle title("Advanced Audio Reactive Visualization");
UIDescription description("Multiple audio visualization modes with advanced controls");

// Master enable for audio reactive mode
UICheckbox enableAudioReactive("Enable Audio Reactive Mode", true);

// Visualization Mode Selection
UIDropdown visualizationMode("Visualization Mode", 
    {"Spectrum Analyzer", "Waveform", "VU Meter", "Spectrogram", "Combined", "Reactive Patterns"});

// Audio Processing Controls
UISlider decayTimeSeconds("Fade time (seconds)", .1, 0, 4, .02);
UISlider attackTimeSeconds("Attack time (seconds)", .1, 0, 4, .02);
UISlider outputTimeSec("Output smoothing (seconds)", .17, 0, 2, .01);
UISlider audioGain("Audio Gain", 1.0, 0.1, 5.0, 0.1);
UISlider noiseFloor("Noise Floor (dB)", -60, -80, -20, 1);

// Visual Controls
UISlider fadeToBlack("Fade to black", 5, 0, 50, 1);
UISlider brightness("Brightness", 128, 0, 255, 1);
UISlider colorSpeed("Color Speed", 1.0, 0.1, 5.0, 0.1);
UIDropdown colorPalette("Color Palette", 
    {"Heat", "Rainbow", "Ocean", "Forest", "Lava", "Cloud", "Party"});
UICheckbox mirrorMode("Mirror Mode", false);
UICheckbox smoothing("Smoothing", true);

// FFT Specific Controls
UISlider fftMinFreq("Min Frequency (Hz)", 20, 20, 1000, 10);
UISlider fftMaxFreq("Max Frequency (Hz)", 10000, 1000, 20000, 100);
UICheckbox logScale("Logarithmic Scale", true);
UISlider fftSmoothing("FFT Smoothing", 0.8, 0.0, 0.95, 0.05);

// Advanced Controls
UICheckbox freeze("Freeze frame", false);
UIButton advanceFrame("Advance frame");
UICheckbox beatDetection("Beat Detection", true);
UISlider beatSensitivity("Beat Sensitivity", 1.5, 0.5, 3.0, 0.1);
UICheckbox autoGainControl("Auto Gain Control", true);

UIAudio audio("Audio");

MaxFadeTracker audioFadeTracker(attackTimeSeconds.value(),
                                decayTimeSeconds.value(), outputTimeSec.value(),
                                44100);

CRGB framebuffer[NUM_LEDS];
XYMap frameBufferXY(WIDTH, HEIGHT, IS_SERPINTINE);

CRGB leds[NUM_LEDS / 4]; // Downscaled buffer
XYMap ledsXY(WIDTH / 2, HEIGHT / 2, IS_SERPINTINE);

FFTBins fftOut(WIDTH);
SoundLevelMeter soundLevelMeter(.0, 0.0);

// Audio reactive state variables
float fftHistory[WIDTH] = {0};
float beatThreshold = 0.0;
float beatAverage = 0.0;
uint32_t lastBeatTime = 0;
bool beatDetected = false;
float autoGainValue = 1.0;
uint8_t colorOffset = 0;

// Function to get the current color palette
CRGBPalette16 getCurrentPalette() {
    switch(colorPalette.as_int()) {
        case 0: return HeatColors_p;
        case 1: return RainbowColors_p;
        case 2: return OceanColors_p;
        case 3: return ForestColors_p;
        case 4: return LavaColors_p;
        case 5: return CloudColors_p;
        case 6: return PartyColors_p;
        default: return HeatColors_p;
    }
}

void setup() {
    Serial.begin(115200);
    
    auto screenmap = ledsXY.toScreenMap();
    screenmap.setDiameter(.2);
    
    // Setup UI callbacks
    decayTimeSeconds.onChanged([](float value) {
        audioFadeTracker.setDecayTime(value);
        FASTLED_WARN("Fade time seconds: " << value);
    });
    
    attackTimeSeconds.onChanged([](float value) {
        audioFadeTracker.setAttackTime(value);
        FASTLED_WARN("Attack time seconds: " << value);
    });
    
    outputTimeSec.onChanged([](float value) {
        audioFadeTracker.setOutputTime(value);
        FASTLED_WARN("Output time seconds: " << value);
    });
    
    FastLED.addLeds<NEOPIXEL, PIN_DATA>(leds, ledsXY.getTotal())
        .setScreenMap(screenmap);
    
    FastLED.setBrightness(brightness.as_int());
}

void clearDisplay() {
    memset(framebuffer, 0, sizeof(framebuffer));
}

void drawSpectrumAnalyzer(const FFTBins& fft, float fade) {
    CRGBPalette16 palette = getCurrentPalette();
    
    for (size_t i = 0; i < fft.bins_raw.size() && i < WIDTH; ++i) {
        float v = fft.bins_db[i];
        
        // Apply noise floor
        v = v - noiseFloor.value();
        if (v < 0) v = 0;
        
        // Map to 0-1 range
        v = fl::map_range<float, float>(v, 0, 80 + noiseFloor.value(), 0, 1.0f);
        v = fl::clamp(v, 0.0f, 1.0f);
        
        // Apply FFT smoothing
        if (smoothing) {
            fftHistory[i] = fftHistory[i] * fftSmoothing.value() + v * (1.0 - fftSmoothing.value());
            v = fftHistory[i];
        }
        
        // Apply gain
        v *= audioGain.value() * autoGainValue;
        v = fl::clamp(v, 0.0f, 1.0f);
        
        // Draw the bar
        int barHeight = v * HEIGHT;
        for (int y = 0; y < barHeight; ++y) {
            uint8_t colorIndex = fl::map_range<float, uint8_t>(
                float(y) / HEIGHT, 0, 1, 0, 255
            ) + colorOffset;
            CRGB color = ColorFromPalette(palette, colorIndex);
            
            framebuffer[frameBufferXY(i, y)] = color;
            
            if (mirrorMode && i < WIDTH/2) {
                framebuffer[frameBufferXY(WIDTH - 1 - i, y)] = color;
            }
        }
    }
}

void drawWaveform(const Slice<const int16_t>& pcm, float fade) {
    CRGBPalette16 palette = getCurrentPalette();
    
    // Clear middle section
    for (int x = 0; x < WIDTH; ++x) {
        for (int y = HEIGHT/4; y < 3*HEIGHT/4; ++y) {
            framebuffer[frameBufferXY(x, y)] = CRGB::Black;
        }
    }
    
    // Draw waveform
    int samplesPerPixel = pcm.size() / WIDTH;
    for (int x = 0; x < WIDTH; ++x) {
        int sampleIndex = x * samplesPerPixel;
        if (sampleIndex < pcm.size()) {
            float sample = float(pcm[sampleIndex]) / 32768.0f;
            sample *= audioGain.value() * autoGainValue;
            
            int y = HEIGHT/2 + (sample * HEIGHT/4);
            y = fl::clamp(y, 0, HEIGHT - 1);
            
            uint8_t colorIndex = fl::map_range<float, uint8_t>(
                ABS(sample), 0, 1, 0, 255
            ) + colorOffset;
            
            framebuffer[frameBufferXY(x, y)] = ColorFromPalette(palette, colorIndex);
            
            // Draw a vertical line for better visibility
            if (ABS(sample) > 0.1) {
                int yStart = HEIGHT/2;
                int yEnd = y;
                if (yStart > yEnd) { int temp = yStart; yStart = yEnd; yEnd = temp; }
                
                for (int yi = yStart; yi <= yEnd; ++yi) {
                    framebuffer[frameBufferXY(x, yi)] = ColorFromPalette(palette, colorIndex);
                }
            }
        }
    }
}

void drawVUMeter(float rms, float peak, float fade) {
    CRGBPalette16 palette = getCurrentPalette();
    
    // Clear VU meter area
    clearDisplay();
    
    // Draw RMS level
    int rmsWidth = rms * WIDTH * audioGain.value() * autoGainValue;
    rmsWidth = fl::clamp(rmsWidth, 0, WIDTH);
    
    for (int x = 0; x < rmsWidth; ++x) {
        for (int y = HEIGHT/3; y < 2*HEIGHT/3; ++y) {
            uint8_t colorIndex = fl::map_range<int, uint8_t>(x, 0, WIDTH, 0, 255) + colorOffset;
            framebuffer[frameBufferXY(x, y)] = ColorFromPalette(palette, colorIndex);
        }
    }
    
    // Draw peak indicator
    int peakX = peak * WIDTH * audioGain.value() * autoGainValue;
    peakX = fl::clamp(peakX, 0, WIDTH - 1);
    
    for (int y = HEIGHT/4; y < 3*HEIGHT/4; ++y) {
        framebuffer[frameBufferXY(peakX, y)] = CRGB::White;
    }
    
    // Draw fade indicator
    int fadeWidth = fade * WIDTH;
    for (int x = 0; x < fadeWidth; ++x) {
        framebuffer[frameBufferXY(x, HEIGHT - 1)] = CRGB::Yellow;
    }
}

void drawSpectrogram() {
    // Shift existing content up
    for (int y = HEIGHT - 1; y > 0; --y) {
        CRGB* row1 = &framebuffer[frameBufferXY(0, y)];
        CRGB* row2 = &framebuffer[frameBufferXY(0, y - 1)];
        memcpy(row1, row2, WIDTH * sizeof(CRGB));
    }
    
    // Clear bottom row
    CRGB* row = &framebuffer[frameBufferXY(0, 0)];
    memset(row, 0, sizeof(CRGB) * WIDTH);
}

void drawReactivePatterns(float fade, bool beat) {
    CRGBPalette16 palette = getCurrentPalette();
    
    if (beat && beatDetection) {
        // Flash effect on beat
        for (int i = 0; i < NUM_LEDS; ++i) {
            framebuffer[i].fadeLightBy(-128); // Make brighter
        }
    }
    
    // Radial pattern based on audio level
    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;
    int radius = fade * MIN(WIDTH, HEIGHT) / 2;
    
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            int dx = x - centerX;
            int dy = y - centerY;
            int dist = sqrt(dx*dx + dy*dy);
            
            if (dist < radius) {
                uint8_t colorIndex = fl::map_range<int, uint8_t>(
                    dist, 0, radius, 255, 0
                ) + colorOffset;
                framebuffer[frameBufferXY(x, y)] = ColorFromPalette(palette, colorIndex);
            }
        }
    }
}

bool detectBeat(float currentLevel) {
    // Simple beat detection algorithm
    beatAverage = beatAverage * 0.95f + currentLevel * 0.05f;
    beatThreshold = beatAverage * beatSensitivity.value();
    
    uint32_t currentTime = millis();
    if (currentLevel > beatThreshold && (currentTime - lastBeatTime) > 100) {
        lastBeatTime = currentTime;
        return true;
    }
    return false;
}

void updateAutoGain(float currentLevel) {
    if (!autoGainControl) {
        autoGainValue = 1.0;
        return;
    }
    
    // Simple AGC algorithm
    static float targetLevel = 0.7f;
    static float avgLevel = 0.0f;
    
    avgLevel = avgLevel * 0.99f + currentLevel * 0.01f;
    
    if (avgLevel > 0.01f) {
        float gainAdjust = targetLevel / avgLevel;
        gainAdjust = fl::clamp(gainAdjust, 0.5f, 2.0f);
        autoGainValue = autoGainValue * 0.95f + gainAdjust * 0.05f;
    }
}

bool doFrame() {
    if (!freeze) {
        return true;
    }
    if (advanceFrame.isPressed()) {
        return true;
    }
    return false;
}

void loop() {
    // Update color animation
    colorOffset += colorSpeed.value();
    
    // Update brightness
    FastLED.setBrightness(brightness.as_int());
    
    // Check if audio reactive mode is enabled
    if (!enableAudioReactive) {
        // Clear display and show a simple pattern
        clearDisplay();
        // You could add a non-audio pattern here
        FastLED.show();
        return;
    }
    
    bool do_frame = doFrame();
    
    while (AudioSample sample = audio.next()) {
        if (!do_frame) {
            continue;
        }
        
        // Process audio
        float fade = audioFadeTracker(sample.pcm().data(), sample.pcm().size());
        soundLevelMeter.processBlock(sample.pcm());
        
        // Get FFT data
        sample.fft(&fftOut);
        
        // Calculate RMS
        float rms = sample.rms();
        rms = fl::map_range<float, float>(rms, 0.0f, 32768.0f, 0.0f, 1.0f);
        rms = fl::clamp(rms, 0.0f, 1.0f);
        
        // Calculate peak
        int32_t max = 0;
        for (size_t i = 0; i < sample.pcm().size(); ++i) {
            int32_t x = ABS(sample.pcm()[i]);
            if (x > max) {
                max = x;
            }
        }
        float peak = fl::map_range<float, float>(max, 0.0f, 32768.0f, 0.0f, 1.0f);
        peak = fl::clamp(peak, 0.0f, 1.0f);
        
        // Beat detection
        beatDetected = detectBeat(peak);
        
        // Auto gain control
        updateAutoGain(rms);
        
        // Apply fade effect
        if (fadeToBlack.as_int() > 0) {
            for (int i = 0; i < NUM_LEDS; ++i) {
                framebuffer[i].fadeToBlackBy(fadeToBlack.as_int());
            }
        }
        
        // Draw based on selected visualization mode
        switch(visualizationMode.as_int()) {
            case 0: // Spectrum Analyzer
                drawSpectrumAnalyzer(fftOut, fade);
                break;
                
            case 1: // Waveform
                drawWaveform(sample.pcm(), fade);
                break;
                
            case 2: // VU Meter
                drawVUMeter(rms, peak, fade);
                break;
                
            case 3: // Spectrogram
                drawSpectrogram();
                drawSpectrumAnalyzer(fftOut, fade);
                break;
                
            case 4: // Combined
                // Draw spectrum in top half
                for (size_t i = 0; i < fftOut.bins_raw.size() && i < WIDTH; ++i) {
                    float v = fftOut.bins_db[i];
                    v = fl::map_range<float, float>(v, 45, 70, 0, 1.f);
                    v = fl::clamp(v, 0.0f, 1.0f) * audioGain.value() * autoGainValue;
                    int barHeight = v * HEIGHT/2;
                    
                    for (int y = HEIGHT/2; y < HEIGHT/2 + barHeight && y < HEIGHT; ++y) {
                        uint8_t colorIndex = fl::map_range<int, uint8_t>(y - HEIGHT/2, 0, HEIGHT/2, 0, 255) + colorOffset;
                        framebuffer[frameBufferXY(i, y)] = ColorFromPalette(getCurrentPalette(), colorIndex);
                    }
                }
                
                // Draw waveform in bottom half
                drawWaveform(sample.pcm(), fade);
                break;
                
            case 5: // Reactive Patterns
                drawReactivePatterns(fade, beatDetected);
                break;
        }
    }
    
    // Downscale framebuffer to LED matrix
    downscale(framebuffer, frameBufferXY, leds, ledsXY);
    
    FastLED.show();
}

#endif  // __AVR__
