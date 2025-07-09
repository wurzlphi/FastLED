#pragma once

#include "fl/str.h"
#include "platforms/shared/ui/json/ui_internal.h"

namespace fl {

/**
 * Base class for all JSON UI elements that provides common functionality
 * including JsonUiInternal management and the id() method.
 */
class JsonUiElementBase {
public:
    /**
     * Constructor that takes a name and creates the JsonUiInternal object
     * @param name The name of the UI element
     */
    JsonUiElementBase(const fl::string &name);
    
    /**
     * Virtual destructor to allow proper cleanup in derived classes
     */
    virtual ~JsonUiElementBase();

    /**
     * Get the unique ID of this UI element
     * @return The unique integer ID
     */
    int id() const {
        return mInternal->id();
    }

    /**
     * Get the name of this UI element
     * @return The name string
     */
    const fl::string &name() const {
        return mInternal->name();
    }

    /**
     * Get the group name of this UI element
     * @return The group name string
     */
    const fl::string &groupName() const {
        return mInternal->groupName();
    }

    /**
     * Set the group name for this UI element
     * @param groupName The group name to set
     */
    void setGroup(const fl::string &groupName) {
        mInternal->setGroup(groupName);
    }

    /**
     * Convert this UI element to JSON representation
     * @param json The JSON object to populate
     */
    virtual void toJson(FLArduinoJson::JsonObject &json) const = 0;

    /**
     * Get the internal JsonUiInternal pointer for advanced operations
     * @return Weak pointer to the internal object
     */
    fl::WeakPtr<JsonUiInternal> getInternal() const {
        return mInternal;
    }

protected:
    /**
     * Protected access to the internal JsonUiInternal object
     * for derived classes that need direct access
     */
    JsonUiInternalPtr mInternal;
};

} // namespace fl
