#include "fl/json.h"
#include "fl/namespace.h"

#include "platforms/shared/ui/json/button.h"
#include "platforms/shared/ui/json/ui.h"

#if FASTLED_ENABLE_JSON



namespace fl {

JsonButtonImpl::JsonButtonImpl(const string &name) : JsonUiElementBase(name), mPressed(false) {
    // Override the default JsonUiInternal with our specific update and toJson functions
    auto updateFunc = JsonUiInternal::UpdateFunction(
        [this](const FLArduinoJson::JsonVariantConst &value) {
            static_cast<JsonButtonImpl *>(this)->updateInternal(value);
        });

    auto toJsonFunc =
        JsonUiInternal::ToJsonFunction([this](FLArduinoJson::JsonObject &json) {
            static_cast<JsonButtonImpl *>(this)->toJson(json);
        });
    
    // Replace the default internal with our specific one
    mInternal = JsonUiInternalPtr::New(name, fl::move(updateFunc),
                                     fl::move(toJsonFunc));
    addJsonUiComponent(mInternal);
    mUpdater.init(this);
}

JsonButtonImpl::~JsonButtonImpl() { removeJsonUiComponent(mInternal); }

JsonButtonImpl &JsonButtonImpl::Group(const fl::string &name) {
    setGroup(name);
    return *this;
}

bool JsonButtonImpl::clicked() const { return mClickedHappened; }

void JsonButtonImpl::toJson(FLArduinoJson::JsonObject &json) const {
    json["name"] = name();
    json["group"] = groupName().c_str();
    json["type"] = "button";
    json["id"] = id();
    json["pressed"] = mPressed;
}

bool JsonButtonImpl::isPressed() const {
    return mPressed;
}

int JsonButtonImpl::clickedCount() const { return mClickedCount; }

void JsonButtonImpl::click() { mPressed = true; }

void JsonButtonImpl::Updater::init(JsonButtonImpl *owner) {
    mOwner = owner;
    fl::EngineEvents::addListener(this);
}

JsonButtonImpl::Updater::~Updater() { fl::EngineEvents::removeListener(this); }

void JsonButtonImpl::Updater::onPlatformPreLoop2() {
    mOwner->mClickedHappened =
        mOwner->mPressed && (mOwner->mPressed != mOwner->mPressedLast);
    mOwner->mPressedLast = mOwner->mPressed;
    if (mOwner->mClickedHappened) {
        mOwner->mClickedCount++;
    }
}

void JsonButtonImpl::updateInternal(
    const FLArduinoJson::JsonVariantConst &value) {
    if (value.is<bool>()) {
        bool newPressed = value.as<bool>();
        mPressed = newPressed;
    }
}

} // namespace fl

#endif // __EMSCRIPTEN__
