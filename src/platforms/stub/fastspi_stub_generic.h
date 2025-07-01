#pragma once

#ifndef FASTLED_STUB_IMPL
#error "why is this being included?"
#endif

#include "fl/stdint.h"
#include "fl/int.h"
#include "fl/namespace.h"
#include "fl/unused.h"

// Signal to the engine that all pins are hardware SPI
#define FASTLED_ALL_PINS_HARDWARE_SPI

FASTLED_NAMESPACE_BEGIN

class StubSPIOutput {
public:
    StubSPIOutput() { }
    void select() { }
    void init() {}
    void waitFully() {}
    void release() {}
    void writeByte(fl::u8 byte) { FASTLED_UNUSED(byte); }
    void writeWord(uint16_t word) { FASTLED_UNUSED(word); }
};


FASTLED_NAMESPACE_END
