
#include "fl/time_alpha.h"
#include "fl/warn.h"
#include "fl/int.h"
#include "math_macros.h"

namespace fl {

fl::u8 time_alpha8(fl::u32 now, fl::u32 start, fl::u32 end) {
    if (now < start) {
        return 0;
    }
    if (now > end) {
        return 255;
    }
    fl::u32 elapsed = now - start;
    fl::u32 total = end - start;
    fl::u32 out = (elapsed * 255) / total;
    if (out > 255) {
        out = 255;
    }
    return static_cast<fl::u8>(out);
}

fl::u16 time_alpha16(fl::u32 now, fl::u32 start, fl::u32 end) {
    if (now < start) {
        return 0;
    }
    if (now > end) {
        return 65535;
    }
    fl::u32 elapsed = now - start;
    fl::u32 total = end - start;
    fl::u32 out = (elapsed * 65535) / total;
    if (out > 65535) {
        out = 65535;
    }
    return static_cast<fl::u16>(out);
}

TimeRamp::TimeRamp(fl::u32 risingTime, fl::u32 latchMs, fl::u32 fallingTime)
    : mLatchMs(latchMs), mRisingTime(risingTime), mFallingTime(fallingTime) {}

void TimeRamp::trigger(fl::u32 now) {
    mStart = now;
    // mLastValue = 0;

    mFinishedRisingTime = mStart + mRisingTime;
    mFinishedPlateauTime = mFinishedRisingTime + mLatchMs;
    mFinishedFallingTime = mFinishedPlateauTime + mFallingTime;
}

void TimeRamp::trigger(fl::u32 now, fl::u32 risingTime, fl::u32 latchMs,
                       fl::u32 fallingTime) {
    mRisingTime = risingTime;
    mLatchMs = latchMs;
    mFallingTime = fallingTime;
    trigger(now);
}

bool TimeRamp::isActive(fl::u32 now) const {

    bool not_started = (mFinishedRisingTime == 0) &&
                       (mFinishedPlateauTime == 0) &&
                       (mFinishedFallingTime == 0);
    if (not_started) {
        // if we have not started, we are not active
        return false;
    }

    if (now < mStart) {
        // if the time is before the start, we are not active
        return false;
    }

    if (now > mFinishedFallingTime) {
        // if the time is after the finished rising, we are not active
        return false;
    }

    return true;
}

fl::u8 TimeRamp::update8(fl::u32 now) {
    if (!isActive(now)) {
        return 0;
    }
    // fl::u32 elapsed = now - mStart;
    fl::u8 out = 0;
    if (now < mFinishedRisingTime) {
        out = time_alpha8(now, mStart, mFinishedRisingTime);
    } else if (now < mFinishedPlateauTime) {
        // plateau
        out = 255;
    } else if (now < mFinishedFallingTime) {
        // ramp down
        fl::u8 alpha =
            time_alpha8(now, mFinishedPlateauTime, mFinishedFallingTime);
        out = 255 - alpha;
    } else {
        // finished
        out = 0;
    }

    mLastValue = out;
    return out;
}

} // namespace fl