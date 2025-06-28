/// @file    SDCardFileSystem.ino
/// @brief   Example of using SD card filesystem with FastLED
/// @example SDCardFileSystem.ino

#include <FastLED.h>
#include <SPI.h>
#include <SD.h>

// How many leds in your strip?
#define NUM_LEDS 60

// SD card chip select pin
#define SD_CS_PIN 4

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI
#define DATA_PIN 3
#define CLOCK_PIN 13

// Define the array of leds
CRGB leds[NUM_LEDS];

// FileSystem instance
fl::FileSystem filesystem;

void setup() { 
    Serial.begin(115200);
    delay(2000); // Safety delay
    
    Serial.println("FastLED SD Card FileSystem Example");
    
    // Initialize FastLED
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(96);
    
    // Initialize SD card filesystem
    Serial.print("Initializing SD card...");
    if (!filesystem.beginSd(SD_CS_PIN)) {
        Serial.println("SD card initialization failed!");
        Serial.println("Things to check:");
        Serial.println("* Is a card inserted?");
        Serial.println("* Is your wiring correct?");
        Serial.println("* Did you change the CS pin to match your shield or module?");
        
        // Show red on failure
        fill_solid(leds, NUM_LEDS, CRGB::Red);
        FastLED.show();
        while (1); // Stop here
    }
    Serial.println("SD card initialized.");
    
    // Show green on success
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(1000);
    
    // Example 1: Read a text file
    readTextExample();
    
    // Example 2: Read LED pattern data
    readLedPatternExample();
    
    // Example 3: Read JSON configuration
    readJsonExample();
}

void readTextExample() {
    Serial.println("\n--- Reading Text File Example ---");
    
    fl::string content;
    if (filesystem.readText("hello.txt", &content)) {
        Serial.print("File content: ");
        Serial.println(content.c_str());
        
        // Flash white to indicate success
        fill_solid(leds, NUM_LEDS, CRGB::White);
        FastLED.show();
        delay(500);
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        FastLED.show();
    } else {
        Serial.println("Failed to read hello.txt");
    }
}

void readLedPatternExample() {
    Serial.println("\n--- Reading LED Pattern Example ---");
    
    // Try to open a file containing RGB values
    fl::FileHandlePtr file = filesystem.openRead("pattern.rgb");
    if (!file || !file->valid()) {
        Serial.println("Failed to open pattern.rgb");
        return;
    }
    
    Serial.print("File size: ");
    Serial.print(file->size());
    Serial.println(" bytes");
    
    // Read LED colors from file
    size_t ledsToRead = min(NUM_LEDS, file->size() / 3);
    size_t ledsRead = file->readCRGB(leds, ledsToRead);
    
    Serial.print("Read ");
    Serial.print(ledsRead);
    Serial.println(" LED values");
    
    // Close the file
    filesystem.close(file);
    
    // Show the pattern
    FastLED.show();
    delay(2000);
}

void readJsonExample() {
    Serial.println("\n--- Reading JSON Configuration Example ---");
    
    fl::JsonDocument doc;
    if (filesystem.readJson("config.json", &doc)) {
        // Example: reading brightness from JSON
        if (doc.containsKey("brightness")) {
            int brightness = doc["brightness"];
            Serial.print("Setting brightness to: ");
            Serial.println(brightness);
            FastLED.setBrightness(brightness);
        }
        
        // Example: reading a color from JSON
        if (doc.containsKey("color")) {
            uint32_t color = doc["color"];
            Serial.print("Setting color to: 0x");
            Serial.println(color, HEX);
            fill_solid(leds, NUM_LEDS, CRGB(color));
            FastLED.show();
        }
    } else {
        Serial.println("Failed to read config.json");
    }
}

void loop() {
    // Rainbow effect
    static uint8_t hue = 0;
    fill_rainbow(leds, NUM_LEDS, hue, 7);
    FastLED.show();
    FastLED.delay(20);
    hue++;
}
