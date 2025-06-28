/// @file    AudioReactive.ino
/// @brief   Audio reactive visualization with multiple modes
/// @example AudioReactive.ino

#include <Arduino.h>
#include <FastLED.h>

// Check if platform has enough memory for audio processing
#if !SKETCH_HAS_LOTS_OF_MEMORY
void setup() {
    Serial.begin(115200);
    Serial.println("This example requires a platform with more memory (ESP32, Teensy, etc.)");
}
void loop() {}
#else

#include "fl/ui.h"
#include "fl/audio.h"
#include "fl/fft.h"
#include "fl/xymap.h"
#include "fl/math.h"

using namespace fl;

// Display configuration
#define WIDTH 100
#define HEIGHT 100
#define NUM_LEDS (WIDTH * HEIGHT)
#define LED_PIN 3
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// Audio configuration
#define SAMPLE_RATE 44100
#define FFT_SIZE 512

// UI Elements
UITitle title("Audio Reactive Visualizations");
UIDescription description("Real-time audio visualizations with beat detection and multiple modes");

// Master controls
UICheckbox enableAudio("Enable Audio", true);
UIDropdown visualMode("Visualization Mode", 
    {"Spectrum Bars", "Radial Spectrum", "Waveform", "VU Meter", "Matrix Rain", "Fire Effect", "Plasma Wave"});

// Audio controls
UISlider audioGain("Audio Gain", 1.0f, 0.1f, 5.0f, 0.1f);
UISlider noiseFloor("Noise Floor", 0.1f, 0.0f, 1.0f, 0.01f);
UICheckbox autoGain("Auto Gain", true);

// Visual controls
UISlider brightness("Brightness", 128, 0, 255, 1);
UISlider fadeSpeed("Fade Speed", 20, 0, 255, 1);
UIDropdown colorPalette("Color Palette", 
    {"Rainbow", "Heat", "Ocean", "Forest", "Party", "Lava", "Cloud"});
UICheckbox mirrorMode("Mirror Mode", false);

// Beat detection
UICheckbox beatDetect("Beat Detection", true);
UISlider beatSensitivity("Beat Sensitivity", 1.5f, 0.5f, 3.0f, 0.1f);
UICheckbox beatFlash("Beat Flash", true);

// Audio input
UIAudio audio("Audio Input");

// Global variables
CRGB leds[NUM_LEDS];
XYMap xyMap(WIDTH, HEIGHT, false);
FFTBins fftBins(FFT_SIZE / 2);
SoundLevelMeter soundMeter(0.0f, 0.0f);

// Audio processing variables
float fftSmooth[FFT_SIZE / 2] = {0};
float beatHistory[43] = {0};
int beatHistoryIndex = 0;
float beatAverage = 0;
float beatVariance = 0;
uint32_t lastBeatTime = 0;
bool isBeat = false;
float autoGainValue = 1.0f;
float peakLevel = 0;

// Visual effect variables
uint8_t hue = 0;
float plasma[WIDTH][HEIGHT];
uint8_t fireBuffer[WIDTH][HEIGHT] = {0};

// Get current color palette
CRGBPalette16 getCurrentPalette() {
    switch(colorPalette.as_int()) {
        case 0: return RainbowColors_p;
        case 1: return HeatColors_p;
        case 2: return OceanColors_p;
        case 3: return ForestColors_p;
        case 4: return PartyColors_p;
        case 5: return LavaColors_p;
        case 6: return CloudColors_p;
        default: return RainbowColors_p;
    }
}

// Beat detection algorithm
bool detectBeat(float energy) {
    beatHistory[beatHistoryIndex] = energy;
    beatHistoryIndex = (beatHistoryIndex + 1) % 43;
    
    // Calculate average
    beatAverage = 0;
    for (int i = 0; i < 43; i++) {
        beatAverage += beatHistory[i];
    }
    beatAverage /= 43.0f;
    
    // Calculate variance
    beatVariance = 0;
    for (int i = 0; i < 43; i++) {
        float diff = beatHistory[i] - beatAverage;
        beatVariance += diff * diff;
    }
    beatVariance /= 43.0f;
    
    // Detect beat
    float threshold = beatAverage + (beatSensitivity.value() * sqrt(beatVariance));
    uint32_t currentTime = millis();
    
    if (energy > threshold && (currentTime - lastBeatTime) > 80) {
        lastBeatTime = currentTime;
        return true;
    }
    return false;
}

// Update auto gain
void updateAutoGain(float level) {
    if (!autoGain) {
        autoGainValue = 1.0f;
        return;
    }
    
    static float targetLevel = 0.7f;
    static float avgLevel = 0.0f;
    
    avgLevel = avgLevel * 0.95f + level * 0.05f;
    
    if (avgLevel > 0.01f) {
        float gainAdjust = targetLevel / avgLevel;
        gainAdjust = fl::clamp(gainAdjust, 0.5f, 2.0f);
        autoGainValue = autoGainValue * 0.9f + gainAdjust * 0.1f;
    }
}

// Clear display
void clearDisplay() {
    if (fadeSpeed.as_int() == 0) {
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    } else {
        fadeToBlackBy(leds, NUM_LEDS, fadeSpeed.as_int());
    }
}

// Visualization: Spectrum Bars
void drawSpectrumBars(const FFTBins& fft, float peak) {
    clearDisplay();
    CRGBPalette16 palette = getCurrentPalette();
    
    int barWidth = WIDTH / 32;  // 32 frequency bands
    
    for (int band = 0; band < 32 && band < fft.bins_db.size(); band++) {
        float magnitude = fft.bins_db[band];
        
        // Apply noise floor
        magnitude = magnitude / 100.0f;  // Normalize from dB
        magnitude = max(0.0f, magnitude - noiseFloor.value());
        
        // Smooth the FFT
        int fftIndex = band * (fft.bins_db.size() / 32);
        fftSmooth[fftIndex] = fftSmooth[fftIndex] * 0.8f + magnitude * 0.2f;
        magnitude = fftSmooth[fftIndex];
        
        // Apply gain
        magnitude *= audioGain.value() * autoGainValue;
        magnitude = fl::clamp(magnitude, 0.0f, 1.0f);
        
        int barHeight = magnitude * HEIGHT;
        int xStart = band * barWidth;
        
        for (int x = 0; x < barWidth - 1; x++) {
            for (int y = 0; y < barHeight; y++) {
                uint8_t colorIndex = fl::map_range<float, uint8_t>(
                    float(y) / HEIGHT, 0, 1, 0, 255
                );
                CRGB color = ColorFromPalette(palette, colorIndex + hue);
                
                int ledIndex = xyMap(xStart + x, y);
                if (ledIndex >= 0) {
                    leds[ledIndex] = color;
                }
                
                if (mirrorMode) {
                    int mirrorIndex = xyMap(WIDTH - 1 - (xStart + x), y);
                    if (mirrorIndex >= 0) {
                        leds[mirrorIndex] = color;
                    }
                }
            }
        }
    }
}

// Visualization: Radial Spectrum
void drawRadialSpectrum(const FFTBins& fft, float peak) {
    clearDisplay();
    CRGBPalette16 palette = getCurrentPalette();
    
    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;
    
    for (int angle = 0; angle < 360; angle += 3) {
        int band = (angle / 3) % 32;
        if (band >= fft.bins_db.size()) continue;
        
        float magnitude = fft.bins_db[band] / 100.0f;
        magnitude = max(0.0f, magnitude - noiseFloor.value());
        magnitude *= audioGain.value() * autoGainValue;
        magnitude = fl::clamp(magnitude, 0.0f, 1.0f);
        
        int radius = magnitude * (MIN(WIDTH, HEIGHT) / 2);
        
        for (int r = 0; r < radius; r++) {
            int x = centerX + (r * cos(angle * PI / 180.0f));
            int y = centerY + (r * sin(angle * PI / 180.0f));
            
            if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                uint8_t colorIndex = fl::map_range<int, uint8_t>(r, 0, radius, 255, 0);
                int ledIndex = xyMap(x, y);
                if (ledIndex >= 0) {
                    leds[ledIndex] = ColorFromPalette(palette, colorIndex + hue);
                }
            }
        }
    }
}

// Visualization: Waveform
void drawWaveform(const Slice<const int16_t>& pcm, float peak) {
    clearDisplay();
    CRGBPalette16 palette = getCurrentPalette();
    
    int samplesPerPixel = pcm.size() / WIDTH;
    int centerY = HEIGHT / 2;
    
    for (int x = 0; x < WIDTH; x++) {
        int sampleIndex = x * samplesPerPixel;
        if (sampleIndex >= pcm.size()) break;
        
        float sample = float(pcm[sampleIndex]) / 32768.0f;
        sample *= audioGain.value() * autoGainValue * 2.0f;
        
        int amplitude = abs(sample * HEIGHT / 2);
        amplitude = MIN(amplitude, HEIGHT / 2);
        
        uint8_t colorIndex = fl::map_range<int, uint8_t>(amplitude, 0, HEIGHT/2, 0, 255);
        CRGB color = ColorFromPalette(palette, colorIndex + hue);
        
        // Draw vertical line
        for (int y = -amplitude; y <= amplitude; y++) {
            int plotY = centerY + y;
            if (plotY >= 0 && plotY < HEIGHT) {
                int ledIndex = xyMap(x, plotY);
                if (ledIndex >= 0) {
                    leds[ledIndex] = color;
                }
            }
        }
    }
}

// Visualization: VU Meter
void drawVUMeter(float rms, float peak) {
    clearDisplay();
    CRGBPalette16 palette = getCurrentPalette();
    
    // RMS level bar
    int rmsWidth = rms * WIDTH * audioGain.value() * autoGainValue;
    rmsWidth = MIN(rmsWidth, WIDTH);
    
    for (int x = 0; x < rmsWidth; x++) {
        for (int y = HEIGHT/3; y < 2*HEIGHT/3; y++) {
            uint8_t colorIndex = fl::map_range<int, uint8_t>(x, 0, WIDTH, 0, 255);
            int ledIndex = xyMap(x, y);
            if (ledIndex >= 0) {
                leds[ledIndex] = ColorFromPalette(palette, colorIndex);
            }
        }
    }
    
    // Peak indicator
    int peakX = peak * WIDTH * audioGain.value() * autoGainValue;
    peakX = MIN(peakX, WIDTH - 1);
    
    for (int y = HEIGHT/4; y < 3*HEIGHT/4; y++) {
        int ledIndex = xyMap(peakX, y);
        if (ledIndex >= 0) {
            leds[ledIndex] = CRGB::White;
        }
    }
    
    // Beat indicator
    if (isBeat && beatFlash) {
        for (int x = 0; x < WIDTH; x++) {
            int ledIndex1 = xyMap(x, 0);
            int ledIndex2 = xyMap(x, HEIGHT - 1);
            if (ledIndex1 >= 0) leds[ledIndex1] = CRGB::White;
            if (ledIndex2 >= 0) leds[ledIndex2] = CRGB::White;
        }
    }
}

// Visualization: Matrix Rain
void drawMatrixRain(float peak) {
    // Shift everything down
    for (int x = 0; x < WIDTH; x++) {
        for (int y = HEIGHT - 1; y > 0; y--) {
            int currentIndex = xyMap(x, y);
            int aboveIndex = xyMap(x, y - 1);
            if (currentIndex >= 0 && aboveIndex >= 0) {
                leds[currentIndex] = leds[aboveIndex];
                leds[currentIndex].fadeToBlackBy(40);
            }
        }
    }
    
    // Add new drops based on audio
    int numDrops = peak * WIDTH * audioGain.value() * autoGainValue;
    for (int i = 0; i < numDrops; i++) {
        int x = random(WIDTH);
        int ledIndex = xyMap(x, 0);
        if (ledIndex >= 0) {
            leds[ledIndex] = CHSV(96, 255, 255);  // Green
        }
    }
}

// Visualization: Fire Effect
void drawFireEffect(float peak) {
    // Update fire buffer
    for (int x = 0; x < WIDTH; x++) {
        // Add heat at bottom based on audio
        int heat = 100 + (peak * 155 * audioGain.value() * autoGainValue);
        fireBuffer[x][HEIGHT - 1] = MIN(heat, 255);
    }
    
    // Propagate fire upwards
    for (int y = 0; y < HEIGHT - 1; y++) {
        for (int x = 0; x < WIDTH; x++) {
            int heat = fireBuffer[x][y + 1];
            
            // Add some randomness
            heat = heat - random(0, 20);
            
            // Spread to neighbors
            if (x > 0) heat = (heat + fireBuffer[x - 1][y + 1]) / 2;
            if (x < WIDTH - 1) heat = (heat + fireBuffer[x + 1][y + 1]) / 2;
            
            fireBuffer[x][y] = MAX(0, heat);
        }
    }
    
    // Convert to colors
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            int ledIndex = xyMap(x, y);
            if (ledIndex >= 0) {
                uint8_t heat = fireBuffer[x][y];
                leds[ledIndex] = HeatColor(heat);
            }
        }
    }
}

// Visualization: Plasma Wave
void drawPlasmaWave(float peak) {
    static float time = 0;
    time += 0.05f + (peak * 0.2f);
    
    CRGBPalette16 palette = getCurrentPalette();
    
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            float value = sin(x * 0.1f + time) + 
                         sin(y * 0.1f - time) +
                         sin((x + y) * 0.1f + time) +
                         sin(sqrt(x * x + y * y) * 0.1f - time);
            
            value = (value + 4) / 8;  // Normalize to 0-1
            value *= audioGain.value() * autoGainValue;
            
            uint8_t colorIndex = value * 255;
            int ledIndex = xyMap(x, y);
            if (ledIndex >= 0) {
                leds[ledIndex] = ColorFromPalette(palette, colorIndex + hue);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Audio Reactive Visualizations");
    Serial.println("Initializing...");
    
    // Initialize LEDs
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(brightness.as_int());
    FastLED.clear();
    FastLED.show();
    
    // Set up UI callbacks
    brightness.onChanged([](float value) {
        FastLED.setBrightness(value);
    });
    
    // Initialize plasma buffer
    for (int x = 0; x < WIDTH; x++) {
        for (int y = 0; y < HEIGHT; y++) {
            plasma[x][y] = 0;
        }
    }
    
    Serial.println("Setup complete!");
}

void loop() {
    // Check if audio is enabled
    if (!enableAudio) {
        // Show a simple test pattern
        fill_rainbow(leds, NUM_LEDS, hue++, 7);
        FastLED.show();
        delay(20);
        return;
    }
    
    // Process audio samples
    while (AudioSample sample = audio.next()) {
        // Update sound meter
        soundMeter.processBlock(sample.pcm());
        
        // Get audio levels
        float rms = sample.rms() / 32768.0f;
        
        // Calculate peak
        int32_t maxSample = 0;
        for (size_t i = 0; i < sample.pcm().size(); i++) {
            int32_t absSample = abs(sample.pcm()[i]);
            if (absSample > maxSample) {
                maxSample = absSample;
            }
        }
        float peak = float(maxSample) / 32768.0f;
        peakLevel = peakLevel * 0.9f + peak * 0.1f;  // Smooth peak
        
        // Update auto gain
        updateAutoGain(rms);
        
        // Beat detection
        if (beatDetect) {
            isBeat = detectBeat(peak);
        }
        
        // Get FFT data
        sample.fft(&fftBins);
        
        // Update color animation
        hue += 1;
        
        // Apply beat flash
        if (isBeat && beatFlash) {
            for (int i = 0; i < NUM_LEDS; i++) {
                leds[i].fadeLightBy(-50);  // Make brighter
            }
        }
        
        // Draw selected visualization
        switch (visualMode.as_int()) {
            case 0:  // Spectrum Bars
                drawSpectrumBars(fftBins, peakLevel);
                break;
                
            case 1:  // Radial Spectrum
                drawRadialSpectrum(fftBins, peakLevel);
                break;
                
            case 2:  // Waveform
                drawWaveform(sample.pcm(), peakLevel);
                break;
                
            case 3:  // VU Meter
                drawVUMeter(rms, peakLevel);
                break;
                
            case 4:  // Matrix Rain
                drawMatrixRain(peakLevel);
                break;
                
            case 5:  // Fire Effect
                drawFireEffect(peakLevel);
                break;
                
            case 6:  // Plasma Wave
                drawPlasmaWave(peakLevel);
                break;
        }
    }
    
    FastLED.show();
}

#endif  // SKETCH_HAS_LOTS_OF_MEMORY
