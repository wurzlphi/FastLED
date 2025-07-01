
#pragma once

#include "fl/stdint.h"
#include "fl/int.h"
#include "fl/namespace.h"

namespace fl {

// NOTE: LED_STRIP_RMT_DEFAULT_MEM_BLOCK_SYMBOLS controls the memory block size.
// See codebase.
class IRmtStrip
{
public:
    enum DmaMode {
        DMA_AUTO,  // Use DMA if available, otherwise use RMT.
        DMA_ENABLED,
        DMA_DISABLED,
    };

    static IRmtStrip* Create(
        int pin, uint32_t led_count, bool is_rgbw,
        uint32_t th0, uint32_t tl0, uint32_t th1, uint32_t tl1, uint32_t reset,
        DmaMode dma_config = DMA_AUTO, fl::u8 interrupt_priority = 3);

    virtual ~IRmtStrip() {}
    virtual void setPixel(uint32_t index, fl::u8 red, fl::u8 green, fl::u8 blue) = 0;
    virtual void setPixelRGBW(uint32_t index, fl::u8 red, fl::u8 green, fl::u8 blue, fl::u8 white) = 0;
    virtual void drawSync()
    {
        drawAsync();
        waitDone();
    }
    virtual void drawAsync() = 0;
    virtual void waitDone() = 0;
    virtual bool isDrawing() = 0;
    virtual void fill(fl::u8 red, fl::u8 green, fl::u8 blue) = 0;
    virtual void fillRGBW(fl::u8 red, fl::u8 green, fl::u8 blue, fl::u8 white) = 0;
    virtual uint32_t numPixels() = 0;
};

} // namespace fl
