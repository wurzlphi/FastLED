# FastLED Build System Refactor

## Overview

The FastLED build system has been refactored to use the root `platformio.ini` file with `pio ci` for better symlink handling, build control, and automated build arguments support.

## Key Changes

### 1. Enhanced Root `platformio.ini`

The root `platformio.ini` file now includes:

- **All board configurations** from `ci/ci/boards.py` 
- **Environment variable support** for build arguments
- **Global symlink configuration** to avoid late binding issues
- **Support for build directory customization**
- **Optimization report generation**
- **build_info.json generation**
- **Custom defines injection**

### 2. New Build Scripts

#### `ci/ci-compile-platformio.py`
- **Replacement for `ci-compile.py`** using the new system
- **Uses `pio ci`** with the root `platformio.ini` file
- **Proper symlink handling** by keeping configuration at project root
- **Environment variable setup** for build arguments

#### `ci-compile-new` (Bash Wrapper)
- **Drop-in replacement** for current build commands
- **Simple interface** to the new Python script
- **Backward compatibility** with existing command-line arguments

#### `ci/ci/fastled_build_support.py`
- **Environment variable processing** for `platformio.ini`
- **build_info.json generation** using PlatformIO metadata
- **Tool alias injection** for symbol analysis compatibility

## Benefits

### 1. Symlink Handling
- **Fixed symlink issues** - `platformio.ini` stays at project root
- **No more late binding** - symlinks resolve correctly during automated builds
- **Consistent library access** across all build environments

### 2. Build Customization
- **Custom build directories** via `--build-dir` or `FASTLED_BUILD_DIR`
- **Optimization reports** via `--optimization-report` or `FASTLED_OPTIMIZATION_REPORT`
- **build_info.json generation** via `--build-info` or `FASTLED_GENERATE_BUILD_INFO`
- **Custom defines** via `--defines` or `FASTLED_CUSTOM_DEFINES`

### 3. Improved CI/CD
- **Consistent behavior** between local and automated builds
- **Environment variable control** for automated systems
- **Better error handling** and build isolation
- **Comprehensive logging** and analysis support

## Usage Examples

### Basic Compilation
```bash
# Using the new bash wrapper
./ci-compile-new uno Blink
./ci-compile-new esp32dev Audio

# Using the Python script directly  
uv run ci/ci-compile-platformio.py uno Blink
uv run ci/ci-compile-platformio.py esp32dev --examples Audio,Blink
```

### Advanced Features
```bash
# With optimization report and build info
./ci-compile-new --optimization-report --build-info uno Blink

# Custom build directory
./ci-compile-new --build-dir /tmp/fastled-build uno Blink

# Custom defines
./ci-compile-new --defines "DEBUG=1,CUSTOM_FLAG=value" uno Blink

# Multiple examples and boards
./ci-compile-new esp32dev,uno --examples Blink,Audio,Fire2012
```

### Environment Variable Control
```bash
# Set environment variables for automated builds
export FASTLED_BUILD_DIR="/tmp/fastled-build"
export FASTLED_OPTIMIZATION_REPORT="1"
export FASTLED_GENERATE_BUILD_INFO="1" 
export FASTLED_CUSTOM_DEFINES="DEBUG=1,FASTLED_ALL_SRC=1"

# Run compilation (environment variables will be used)
./ci-compile-new uno Blink
```

## Supported Environment Variables

| Variable | Description | Values |
|----------|-------------|---------|
| `FASTLED_BUILD_DIR` | Custom build directory | Any path |
| `FASTLED_OPTIMIZATION_REPORT` | Enable optimization reports | `1`, `true`, `yes`, `on` |
| `FASTLED_GENERATE_BUILD_INFO` | Generate build_info.json | `1`, `true`, `yes`, `on` |
| `FASTLED_CUSTOM_DEFINES` | Additional defines | Comma-separated list |

## Supported Board Environments

All boards from `ci/ci/boards.py` are now available in `platformio.ini`:

### ESP32 Family
- `esp32dev` - ESP32 development board
- `esp32s3` - ESP32-S3 with PSRAM 
- `esp32c3` - ESP32-C3 RISC-V
- `esp32c6` - ESP32-C6 with WiFi 6
- `esp32c2` - ESP32-C2 low-cost
- `esp32p4` - ESP32-P4 high-performance
- `esp32rmt_51` - ESP32 with RMT5 support

### Arduino AVR
- `uno` - Arduino UNO
- `yun` - Arduino Yún
- `attiny85` - ATtiny85 microcontroller
- `ATtiny1616` - ATtiny1616 microcontroller

### Teensy
- `teensylc` - Teensy LC
- `teensy30` - Teensy 3.0
- `teensy31` - Teensy 3.1/3.2
- `teensy40` - Teensy 4.0
- `teensy41` - Teensy 4.1

### ARM Cortex-M
- `giga_r1` - Arduino GIGA R1
- `bluepill` - STM32F103 "Blue Pill"
- `digix` - Arduino Due compatible
- `maple_mini` - STM32 Maple Mini

### Nordic nRF52
- `adafruit_feather_nrf52840_sense` - Adafruit Feather nRF52840
- `xiaoblesense_adafruit` - Seeed XIAO BLE Sense

### Raspberry Pi Pico
- `rpipico` - Raspberry Pi Pico
- `rpipico2` - Raspberry Pi Pico 2

### Other Platforms
- `apollo3_red` - SparkFun Apollo3 Red
- `uno_r4_wifi` - Arduino UNO R4 WiFi
- `sparkfun_xrp_controller` - SparkFun XRP Controller
- `esp01` - ESP8266 ESP-01

## Migration Guide

### For Existing Scripts

Replace `ci-compile.py` calls with the new system:

```bash
# Old way
uv run ci-compile.py uno --examples Blink

# New way (using wrapper)
./ci-compile-new uno Blink

# New way (direct)
uv run ci/ci-compile-platformio.py uno Blink
```

### For CI/CD Systems

Update CI/CD scripts to use environment variables:

```yaml
# GitHub Actions example
- name: Compile FastLED Examples
  env:
    FASTLED_BUILD_DIR: ${{ github.workspace }}/.build
    FASTLED_OPTIMIZATION_REPORT: "1"
    FASTLED_GENERATE_BUILD_INFO: "1"
  run: ./ci-compile-new uno,esp32dev Blink,Audio
```

### For Manual Builds

The new system is backward compatible:

```bash
# These work the same as before
./ci-compile-new uno Blink
./ci-compile-new esp32dev --examples Audio,Fire2012
./ci-compile-new --symbols uno Blink  # With symbol analysis
```

## Technical Details

### PlatformIO.ini Structure

The enhanced `platformio.ini` includes:

```ini
[platformio]
src_dir = dev
default_envs = dev
build_cache_dir = .pio_cache

[env]
; Global settings with environment variable support
build_flags = 
    -DUSE_CCACHE=1
    ${sysenv.FASTLED_OPTIMIZATION_REPORT_FLAG}
    ${sysenv.FASTLED_CUSTOM_DEFINES_FLAGS}
build_dir = ${sysenv.FASTLED_BUILD_DIR}
lib_deps = FastLED=symlink://./

[env:uno]
platform = atmelavr
board = uno
framework = arduino
build_flags = ${env.build_flags}
```

### Environment Variable Processing

The `fastled_build_support.py` script:

1. **Processes environment variables** into PlatformIO format
2. **Sets up optimization flags** based on `FASTLED_OPTIMIZATION_REPORT`
3. **Configures build directories** from `FASTLED_BUILD_DIR`
4. **Handles custom defines** from `FASTLED_CUSTOM_DEFINES`
5. **Generates build_info.json** when `FASTLED_GENERATE_BUILD_INFO` is set

### Symlink Resolution

- **platformio.ini at root** - ensures symlinks resolve correctly
- **Global lib_deps** - FastLED library linked for all environments
- **No late binding** - symlinks work in automated builds
- **Consistent paths** - same library path for all boards

## Troubleshooting

### Common Issues

1. **Symlink problems** - Make sure you're using the root `platformio.ini`
2. **Environment variables not applied** - Check `fastled_build_support.py` is imported
3. **Board not found** - Verify board name matches `platformio.ini` environments
4. **Build directory issues** - Ensure build directory has write permissions

### Debug Mode

Enable verbose output for troubleshooting:

```bash
./ci-compile-new -v uno Blink  # Verbose compilation
uv run ci/ci-compile-platformio.py --verbose uno Blink  # Direct verbose
```

### Verification

Test the new system with a simple build:

```bash
# Test basic compilation
./ci-compile-new uno Blink

# Test with all features
./ci-compile-new --optimization-report --build-info --symbols uno Blink

# Verify build output
ls -la .build/uno/Blink/
```

## Future Enhancements

- **Platform-specific optimization** flags in `platformio.ini`
- **Advanced build caching** with ccache integration
- **Build metrics collection** and analysis
- **Integration with symbol analysis** tools
- **Automated build artifact** management
