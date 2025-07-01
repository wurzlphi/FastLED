
#pragma once

#include "fl/stdint.h"

#include "fl/int.h"
#include "fl/map.h"
#include "fl/namespace.h"
#include "fl/scoped_ptr.h"
#include "fl/span.h"
#include "fl/vector.h"

namespace fl {

struct DrawItem {
    DrawItem() = default;
    DrawItem(fl::u8 pin, u16 numLeds, bool is_rgbw);
    
    // Rule of 5 for POD data
    DrawItem(const DrawItem &other) = default;
    DrawItem &operator=(const DrawItem &other) = default;
    DrawItem(DrawItem &&other) noexcept = default;
    DrawItem &operator=(DrawItem &&other) noexcept = default;
    
    fl::u8 mPin = 0;
    fl::u32 mNumBytes = 0;
    bool mIsRgbw = false;
    bool operator!=(const DrawItem &other) const {
        return mPin != other.mPin || mNumBytes != other.mNumBytes ||
               mIsRgbw != other.mIsRgbw;
    }
};

// Needed by controllers that require a compact, rectangular buffer of pixel
// data. Namely, ObjectFLED and the I2S controllers. This class handles using
// multiple independent strips of LEDs, each with their own buffer of pixel
// data. The strips are not necessarily contiguous in memory. One or more
// DrawItems containing the pin number and number are queued up. When the
// queue-ing is done, the buffers are compacted into the rectangular buffer.
// Data access is achieved through a span<fl::u8> representing the pixel data
// for that pin.
class RectangularDrawBuffer {
  public:
    typedef fl::HeapVector<DrawItem> DrawList;
    // We manually manage the memory for the buffer of all LEDs so that it can
    // go into psram on ESP32S3, which is managed by fl::PSRamAllocator.
    scoped_array<fl::u8> mAllLedsBufferUint8;
    fl::u32 mAllLedsBufferUint8Size = 0;
    fl::FixedMap<fl::u8, fl::span<fl::u8>, 50> mPinToLedSegment;
    DrawList mDrawList;
    DrawList mPrevDrawList;
    bool mDrawListChangedThisFrame = false;

    enum QueueState { IDLE, QUEUEING, QUEUE_DONE };
    QueueState mQueueState = IDLE;

    RectangularDrawBuffer() = default;
    ~RectangularDrawBuffer() = default;

    fl::span<fl::u8> getLedsBufferBytesForPin(fl::u8 pin,
                                               bool clear_first = true);

    // Safe to call multiple times before calling queue() once. Returns true on
    // the first call, false after.
    bool onQueuingStart();
    void queue(const DrawItem &item);

    // Safe to call multiple times before calling onQueueingStart() again.
    // Returns true on the first call, false after.
    bool onQueuingDone();
    fl::u32 getMaxBytesInStrip() const;
    fl::u32 getTotalBytes() const;
    void getBlockInfo(fl::u32 *num_strips, fl::u32 *bytes_per_strip,
                      fl::u32 *total_bytes) const;
};

} // namespace fl
