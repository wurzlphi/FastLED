# FastLED SD Card FileSystem Example

This example demonstrates how to use SD card storage with FastLED to read configuration files, LED patterns, and other data.

## Hardware Requirements

- Arduino board (Uno, Mega, ESP32, etc.)
- SD card module or shield
- LED strip (WS2812B or similar)
- SD card (formatted as FAT32)

## Wiring

### SD Card Module
- **CS** → Pin 4 (or change `SD_CS_PIN` in the code)
- **MOSI** → Pin 11 (Arduino Uno/Nano) or MOSI pin
- **MISO** → Pin 12 (Arduino Uno/Nano) or MISO pin  
- **SCK** → Pin 13 (Arduino Uno/Nano) or SCK pin
- **VCC** → 5V (or 3.3V depending on module)
- **GND** → GND

### LED Strip
- **Data** → Pin 3 (or change `DATA_PIN` in the code)
- **VCC** → 5V
- **GND** → GND

## SD Card Preparation

Create the following files on your SD card:

### 1. `hello.txt`
A simple text file with any content:
```
Hello from SD card!
FastLED is awesome!
```

### 2. `pattern.rgb`
A binary file containing RGB values for the LEDs. Each LED uses 3 bytes (R, G, B).

You can create this file using a hex editor or with this Python script:
```python
# create_pattern.py
import struct

# Create a gradient pattern
with open('pattern.rgb', 'wb') as f:
    for i in range(60):  # 60 LEDs
        r = int(255 * (i / 59.0))  # Red gradient
        g = 0
        b = int(255 * (1 - i / 59.0))  # Blue inverse gradient
        f.write(struct.pack('BBB', r, g, b))
```

### 3. `config.json`
A JSON configuration file:
```json
{
  "brightness": 128,
  "color": 16711680,
  "speed": 30,
  "pattern": "rainbow"
}
```

Note: The color value is in hex format (0xFF0000 = 16711680 = red).

## Code Features

The example demonstrates:

1. **Text File Reading** - Read and display text files from SD card
2. **Binary Pattern Loading** - Load LED color data directly from binary files
3. **JSON Configuration** - Parse JSON files for dynamic configuration
4. **Error Handling** - Proper error messages when files are not found

## Usage

1. Format your SD card as FAT32
2. Copy the example files to the root of the SD card
3. Wire up your hardware according to the pinout above
4. Upload the sketch to your Arduino
5. Open Serial Monitor at 115200 baud
6. Watch the LEDs and monitor output

## Expected Behavior

1. **Green LEDs** - SD card initialized successfully
2. **White Flash** - Text file read successfully
3. **Pattern Display** - Shows the pattern from `pattern.rgb`
4. **Color from JSON** - Sets color based on `config.json`
5. **Rainbow Effect** - Continuous rainbow in the loop

## Troubleshooting

### SD Card Not Initializing
- Check wiring connections
- Ensure SD card is formatted as FAT32
- Try a different SD card
- Verify CS pin matches your wiring

### Files Not Found
- Ensure files are in the root directory
- Check file names match exactly (case sensitive)
- Verify files were properly copied to SD card

### LED Issues
- Check DATA_PIN matches your wiring
- Ensure power supply is adequate for your LED strip
- Verify NUM_LEDS matches your strip length

## Advanced Usage

### Using SdFat Library

For better performance and features, you can use the SdFat library:

1. Install SdFat library from Library Manager
2. Add before including FastLED:
```cpp
#define USE_SDFAT
#include <FastLED.h>
```

### Custom File Formats

You can create your own file formats for animations:

```cpp
// Animation frame format (example)
struct AnimationFrame {
    uint16_t duration_ms;
    CRGB colors[NUM_LEDS];
};

// Read animation
fl::FileHandlePtr file = filesystem.openRead("animation.dat");
AnimationFrame frame;
file->read((uint8_t*)&frame, sizeof(AnimationFrame));
```

## Memory Considerations

- File operations use dynamic memory
- Large files should be read in chunks
- Close files when done to free resources

## Platform Notes

### ESP32
- Can use built-in SD card slot on some boards
- Supports SDMMC for faster speeds

### Arduino Uno
- Limited RAM, keep files small
- Consider reading data in chunks

### Teensy
- Supports high-speed SPI
- Can use built-in SD socket on some models
