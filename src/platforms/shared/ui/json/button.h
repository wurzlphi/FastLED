#pragma once

#include "fl/stdint.h"

#include "fl/engine_events.h"
#include "fl/str.h"
#include "platforms/shared/ui/json/ui_internal.h"
#include "platforms/shared/ui/json/ui_element_base.h"

namespace fl {

class JsonButtonImpl : public JsonUiElementBase {
  public:
    JsonButtonImpl(const fl::string &name);
    ~JsonButtonImpl();
    JsonButtonImpl &Group(const fl::string &name);

    bool isPressed() const;
    bool clicked() const;
    int clickedCount() const;
    
    // Override the virtual toJson method from the base class
    void toJson(FLArduinoJson::JsonObject &json) const override;

    void click();

  private:
    struct Updater : fl::EngineEvents::Listener {
        void init(JsonButtonImpl *owner);
        ~Updater();
        void onPlatformPreLoop2() override;
        JsonButtonImpl *mOwner = nullptr;
    };

    Updater mUpdater;

    void updateInternal(const FLArduinoJson::JsonVariantConst &value);

    bool mPressed = false;
    bool mPressedLast = false;
    bool mClickedHappened = false;
    int mClickedCount = 0;
};

} // namespace fl
