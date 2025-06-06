#include "pch.h"
#include "brush.h"
#include "../util/util.h"



Brush::Brush(const string& fillStyle, const string& strokeStyle)
    : Decoration(fillStyle, strokeStyle) {

    // Set default properties
    doubleProperties["Width"] = 1.0f;
    doubleProperties["Bend"] = 0.0f;
    boolProperties["Boundary"] = false;
}

Brush::Brush(const string& fillStyle)
    : Brush(fillStyle, "") {}

void Brush::set(const string& name, double value) {
    doubleProperties[name] = value;
    notifyChange();
}

void Brush::set(const string& name, bool value) {
    boolProperties[name] = value;
    notifyChange();
}

void Brush::set(const string& name, const string& value) {
    stringProperties[name] = value;
    notifyChange();
}

double Brush::getDouble(const string& name) const {
    auto it = doubleProperties.find(name);
    return it != doubleProperties.end() ? it->second : 0.0f;
}

bool Brush::getBool(const string& name) const {
    auto it = boolProperties.find(name);
    return it != boolProperties.end() ? it->second : false;
}

string Brush::getString(const string& name) const {
    auto it = stringProperties.find(name);
    return it != stringProperties.end() ? it->second : "";
}

Brush* Brush::import(Json json) {
    auto brush = new Brush("", "");
    
    for (const auto& [key, val] : json.items()) {
        if (val.is_string()) {
            string converted_val = val.get<string>();
            brush->set(key, converted_val);
        } else if (val.is_number_float()) {
            double converted_val = (double)val.get<float>();
            brush->set(key, converted_val);
        } else if (val.is_number_integer()) {
            int converted_val = val.get<int>();
            brush->set(key, (double)converted_val);
        } else if (val.is_boolean()) {
            bool converted_val = val.get<bool>();
            brush->set(key, converted_val);
        }
    }
 
    return brush;
}

