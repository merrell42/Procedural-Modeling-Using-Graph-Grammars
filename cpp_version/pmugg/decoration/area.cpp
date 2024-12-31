#include "area.h"

namespace ms {

Area::Area(const std::string& fillStyle, const std::string& strokeStyle,
           const std::function<void()>& onChange)
    : Decoration(fillStyle, strokeStyle, onChange) {
    
    // Set default properties
    boolProperties["Boundary"] = false;
}

void Area::set(const std::string& name, float value) {
    floatProperties[name] = value;
    notifyChange();
}

void Area::set(const std::string& name, bool value) {
    boolProperties[name] = value;
    notifyChange();
}

void Area::set(const std::string& name, const std::string& value) {
    stringProperties[name] = value;
    notifyChange();
}

float Area::getFloat(const std::string& name) const {
    auto it = floatProperties.find(name);
    return it != floatProperties.end() ? it->second : 0.0f;
}

bool Area::getBool(const std::string& name) const {
    auto it = boolProperties.find(name);
    return it != boolProperties.end() ? it->second : false;
}

std::string Area::getString(const std::string& name) const {
    auto it = stringProperties.find(name);
    return it != stringProperties.end() ? it->second : "";
}

/* void Area::draw(Context* context,
               const std::function<Vec2(const Vec2&)>& transform) {
    // Drawing implementation for area
    context->setStrokeStyle(strokeStyle);
    context->setFillStyle(fillStyle);
    // Additional drawing logic...
} */

} // namespace ms 