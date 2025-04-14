#include "pch.h"
#include "brush.h"
#include "../util/util.h"

namespace ms {

Brush::Brush(const std::string& fillStyle, const std::string& strokeStyle)
    : Decoration(fillStyle, strokeStyle) {

    // Set default properties
    floatProperties["Width"] = 1.0f;
    floatProperties["Bend"] = 0.0f;
    boolProperties["Boundary"] = false;
}

Brush::Brush(const std::string& fillStyle)
    : Brush(fillStyle, "") {}

void Brush::set(const std::string& name, float value) {
    floatProperties[name] = value;
    notifyChange();
}

void Brush::set(const std::string& name, bool value) {
    boolProperties[name] = value;
    notifyChange();
}

void Brush::set(const std::string& name, const std::string& value) {
    stringProperties[name] = value;
    notifyChange();
}

float Brush::getFloat(const std::string& name) const {
    auto it = floatProperties.find(name);
    return it != floatProperties.end() ? it->second : 0.0f;
}

bool Brush::getBool(const std::string& name) const {
    auto it = boolProperties.find(name);
    return it != boolProperties.end() ? it->second : false;
}

std::string Brush::getString(const std::string& name) const {
    auto it = stringProperties.find(name);
    return it != stringProperties.end() ? it->second : "";
}

/* void Brush::draw(Context* context,
                const std::function<Vec2(const Vec2&)>& transform) {
    // Drawing implementation for brush
    context->setStrokeStyle(strokeStyle);
    context->setFillStyle(fillStyle);
    context->setLineWidth(getFloat("Width"));
    // Additional drawing logic...
} */

Brush* Brush::import(Json json) {
    auto brush = new Brush("", "");
    
    for (const auto& [key, val] : json.items()) {
        if (val.is_string()) {
            std::string converted_val = val.get<std::string>();
            brush->set(key, converted_val);
        } else if (val.is_number_float()) {
            float converted_val = val.get<float>();
            brush->set(key, converted_val);
        } else if (val.is_number_integer()) {
            int converted_val = val.get<int>();
            brush->set(key, (float)converted_val);
        } else if (val.is_boolean()) {
            bool converted_val = val.get<bool>();
            brush->set(key, converted_val);
        }
    }
 
    return brush;
}

} // namespace ms 