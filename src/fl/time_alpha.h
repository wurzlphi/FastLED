
#pragma once

#include "fl/stdint.h"

#include "fl/math_macros.h"
#include "fl/warn.h"
#include "fl/int.h"

namespace fl {

// Use this function to compute the alpha value based on the time elapsed
// 0 -> 255
fl::u8 time_alpha8(fl::u32 now, fl::u32 start, fl::u32 end);
// 0 -> 65535
fl::u16 time_alpha16(fl::u32 now, fl::u32 start, fl::u32 end);

inline float time_alphaf(fl::u32 now, fl::u32 start, fl::u32 end) {
    if (now < start) {
        return 0.0f;
    }
    fl::u32 elapsed = now - start;
    fl::u32 total = end - start;
    float out = static_cast<float>(elapsed) / static_cast<float>(total);
    return out;
}

class TimeAlpha {
  public:
    virtual ~TimeAlpha() = default;
    virtual void trigger(fl::u32 now) = 0;
    virtual fl::u8 update8(fl::u32 now) = 0;
    virtual fl::u16 update16(fl::u32 now) {
        return static_cast<fl::u16>(update8(now) << 8) + 0xFF;
    }
    virtual float updatef(fl::u32 now) {
        return static_cast<float>(update16(now)) / 65535.0f;
    }
    virtual bool isActive(fl::u32 now) const = 0;
};

/*
 *                       amplitude
 *                          ^
 *  255 ───────────────────────
 *                     /        \
 *                    /          \
 *                   /            \
 *                  /              \
 *    0 ────────────┴               ┴──────────────────> time (ms)
 *                  t0   t1     t2   t4
 *
 *
 *
 */
class TimeRamp : public TimeAlpha {
  public:
    /// @param latchMs     total active time (ms)
    /// @param risingTime  time to ramp from 0→255 (ms)
    /// @param fallingTime time to ramp from 255→0 (ms)
    TimeRamp(fl::u32 risingTime, fl::u32 latchMs, fl::u32 fallingTime);

    /// Call this when you want to (re)start the ramp cycle.
    void trigger(fl::u32 now) override;

    void trigger(fl::u32 now, fl::u32 risingTime, fl::u32 latchMs,
                 fl::u32 fallingTime);

    /// @return true iff we're still within the latch period.
    bool isActive(fl::u32 now) const override;

    /// Compute current 0–255 output based on how much time has elapsed since
    /// trigger().
    fl::u8 update8(fl::u32 now) override;

  private:
    fl::u32 mLatchMs;
    fl::u32 mRisingTime;
    fl::u32 mFallingTime;

    fl::u32 mFinishedRisingTime = 0;
    fl::u32 mFinishedPlateauTime = 0;
    fl::u32 mFinishedFallingTime = 0;

    fl::u32 mStart = 0;
    fl::u8 mLastValue = 0;
};

/*
 *                       amplitude
 *                          ^
 *  255 ──────────────────────────────────────
 *                     /
 *                    /
 *                   /
 *                  /
 *    0 ────────────┴                       --> time (ms)
 *                  t0   t1
 *
 *
 *
 */
class TimeClampedTransition : public TimeAlpha {
  public:
    TimeClampedTransition(fl::u32 duration) : mDuration(duration) {}

    void trigger(fl::u32 now) override {
        mStart = now;
        mEnd = now + mDuration;
    }

    bool isActive(fl::u32 now) const override {
        bool not_started = (mEnd == 0) && (mStart == 0);
        if (not_started) {
            // if we have not started, we are not active
            return false;
        }
        if (now < mStart) {
            // if the time is before the start, we are not active
            return false;
        }
        if (now > mEnd) {
            // if the time is after the finished rising, we are not active
            return false;
        }
        return true;
    }

    fl::u8 update8(fl::u32 now) override {
        bool not_started = (mEnd == 0) && (mStart == 0);
        if (not_started) {
            // if we have not started, we are not active
            return 0;
        }
        fl::u8 out = time_alpha8(now, mStart, mEnd);
        return out;
    }

    float updatef(fl::u32 now) override {
        bool not_started = (mEnd == 0) && (mStart == 0);
        if (not_started) {
            return 0;
        }
        float out = time_alphaf(now, mStart, mEnd);
        if (mMaxClamp > 0.f) {
            out = MIN(out, mMaxClamp);
        }
        return out;
    }

    void set_max_clamp(float max) { mMaxClamp = max; }

  private:
    fl::u32 mStart = 0;
    fl::u32 mDuration = 0;
    fl::u32 mEnd = 0;
    float mMaxClamp = -1.f; // default disabled.
};

} // namespace fl
