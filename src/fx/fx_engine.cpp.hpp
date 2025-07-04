#include "fx_engine.h"
#include "video.h"
#include "fx/1d/cylon.h"
#include "fx/1d/fire2012.h"
#include "fx/1d/pride2015.h"
#include "fx/1d/demoreel100.h"
#include "fx/1d/pacifica.h"
#include "fx/1d/twinklefox.h"
#include "fl/string.h"

namespace fl {

FxEngine::FxEngine(uint16_t numLeds, bool interpolate)
    : mNumLeds(numLeds), mTimeFunction(0), mCompositor(numLeds), mCurrId(0),
      mInterpolate(interpolate) {}

FxEngine::~FxEngine() {}

fl::FixedMap<fl::string, FxEngine::FxFunction, 16> FxEngine::getEffectMap() {
    fl::FixedMap<fl::string, FxEngine::FxFunction, 16> effectMap;
    effectMap.insert("cylon", [](uint16_t numLeds) -> FxPtr { return CylonPtr::New(numLeds); });
    effectMap.insert("fire2012", [](uint16_t numLeds) -> FxPtr { return Fire2012Ptr::New(numLeds); });
    effectMap.insert("pride2015", [](uint16_t numLeds) -> FxPtr { return Pride2015Ptr::New(numLeds); });
    effectMap.insert("demoreel100", [](uint16_t numLeds) -> FxPtr { return DemoReel100Ptr::New(numLeds); });
    effectMap.insert("pacifica", [](uint16_t numLeds) -> FxPtr { return PacificaPtr::New(numLeds); });
    effectMap.insert("twinklefox", [](uint16_t numLeds) -> FxPtr { return TwinkleFoxPtr::New(numLeds); });
    return effectMap;
}

FxEngine::FxFunction FxEngine::findFxByName(const char* name) {
    if (!name) {
        return nullptr;
    }
    
    auto effectMap = getEffectMap();
    fl::string searchName(name);
    
    // Convert to lowercase for case-insensitive matching
    for (size_t i = 0; i < searchName.size(); ++i) {
        char& c = searchName[i];
        if (c >= 'A' && c <= 'Z') {
            c = c - 'A' + 'a';
        }
    }
    
    auto it = effectMap.find(searchName);
    if (it != effectMap.end()) {
        return it->second;
    }
    
    return nullptr;
}

bool FxEngine::setFx(const char* name) {
    FxFunction factory = findFxByName(name);
    if (!factory) {
        return false;
    }
    
    // Create the effect
    FxPtr effect = factory(mNumLeds);
    if (!effect) {
        return false;
    }
    
    // Clear existing effects and add the new one
    mEffects.clear();
    mCounter = 0;
    
    int id = addFx(effect);
    if (id >= 0) {
        mCurrId = id;
        return true;
    }
    
    return false;
}

int FxEngine::addFx(FxPtr effect) {
    float fps = 0;
    if (mInterpolate && effect->hasFixedFrameRate(&fps)) {
        // Wrap the effect in a VideoFxWrapper so that we can get
        // interpolation.
        VideoFxWrapperPtr vid_fx = VideoFxWrapperPtr::New(effect);
        vid_fx->setFade(0, 0); // No fade for interpolated effects
        effect = vid_fx;
    }
    bool auto_set = mEffects.empty();
    bool ok = mEffects.insert(mCounter, effect).first;
    if (!ok) {
        return -1;
    }
    if (auto_set) {
        mCurrId = mCounter;
        mCompositor.startTransition(0, 0, effect);
    }
    return mCounter++;
}

bool FxEngine::nextFx(uint16_t duration) {
    bool ok = mEffects.next(mCurrId, &mCurrId, true);
    if (!ok) {
        return false;
    }
    setNextFx(mCurrId, duration);
    return true;
}

bool FxEngine::setNextFx(int index, uint16_t duration) {
    if (!mEffects.has(index)) {
        return false;
    }
    mCurrId = index;
    mDuration = duration;
    mDurationSet = true;
    return true;
}

FxPtr FxEngine::removeFx(int index) {
    if (!mEffects.has(index)) {
        return FxPtr();
    }

    FxPtr removedFx;
    bool ok = mEffects.get(index, &removedFx);
    if (!ok) {
        return FxPtr();
    }

    if (mCurrId == index) {
        // If we're removing the current effect, switch to the next one
        mEffects.next(mCurrId, &mCurrId, true);
        mDurationSet = true;
        mDuration = 0; // Instant transition
    }

    return removedFx;
}

FxPtr FxEngine::getFx(int id) {
    if (mEffects.has(id)) {
        FxPtr fx;
        mEffects.get(id, &fx);
        return fx;
    }
    return FxPtr();
}

bool FxEngine::draw(uint32_t now, CRGB *finalBuffer) {
    mTimeFunction.update(now);
    uint32_t warpedTime = mTimeFunction.time();

    if (mEffects.empty()) {
        return false;
    }
    if (mDurationSet) {
        FxPtr fx;
        bool ok = mEffects.get(mCurrId, &fx);
        if (!ok) {
            // something went wrong.
            return false;
        }
        mCompositor.startTransition(now, mDuration, fx);
        mDurationSet = false;
    }
    if (!mEffects.empty()) {
        mCompositor.draw(now, warpedTime, finalBuffer);
    }
    return true;
}

} // namespace fl
