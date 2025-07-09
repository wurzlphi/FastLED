#include "platforms/shared/ui/json/ui_element_base.h"

namespace fl {

JsonUiElementBase::JsonUiElementBase(const fl::string &name) {
    // Create a default JsonUiInternal with empty update and toJson functions
    // Derived classes will override these as needed
    auto updateFunc = JsonUiInternal::UpdateFunction([](const FLArduinoJson::JsonVariantConst &value) {
        // Default empty update function
    });
    
    auto toJsonFunc = JsonUiInternal::ToJsonFunction([](FLArduinoJson::JsonObject &json) {
        // Default empty toJson function
    });
    
    mInternal = JsonUiInternalPtr::New(name, fl::move(updateFunc), fl::move(toJsonFunc));
}

JsonUiElementBase::~JsonUiElementBase() {
    // The JsonUiInternalPtr will automatically clean up when it goes out of scope
}

} // namespace fl
