#pragma once

#include <memory>

#include "fl/engine_events.h"
#include "fl/int.h"
#include "fl/map.h"
#include "fl/namespace.h"
#include "fl/screenmap.h"
#include "fl/singleton.h"
#include "fl/span.h"
#include "strip_id_map.h"


namespace fl {

typedef fl::span<const fl::u8> SliceUint8;

// Zero copy data transfer of strip information from C++ to JavaScript.
class ActiveStripData : public fl::EngineEvents::Listener {
  public:
    typedef fl::SortedHeapMap<int, SliceUint8> StripDataMap;
    typedef fl::SortedHeapMap<int, fl::ScreenMap> ScreenMapMap;

    static ActiveStripData &Instance();
    void update(int id, uint32_t now, const fl::u8 *pixel_data, size_t size);
    void updateScreenMap(int id, const fl::ScreenMap &screenmap);

    fl::string infoJsonString();

    const StripDataMap &getData() const { return mStripMap; }

    ~ActiveStripData() { fl::EngineEvents::removeListener(this); }

    void onBeginFrame() override { mStripMap.clear(); }

    void onCanvasUiSet(CLEDController *strip,
                       const fl::ScreenMap &screenmap) override {
        int id = StripIdMap::addOrGetId(strip);
        updateScreenMap(id, screenmap);
    }

    bool hasScreenMap(int id) const { return mScreenMap.has(id); }

  private:
    friend class fl::Singleton<ActiveStripData>;
    ActiveStripData() {
        fl::EngineEvents::Listener *listener = this;
        fl::EngineEvents::addListener(listener);
    }

    StripDataMap mStripMap;
    ScreenMapMap mScreenMap;
};

} // namespace fl
