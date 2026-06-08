#include "pch.h"
#include "edge_settings.h"
#include "../util/util.h"

EdgeSettings::EdgeSettings() {
    doubleProperties["Width"] = 1.0f;
    doubleProperties["Bend"] = 0.0f;
    boolProperties["Boundary"] = false;
}

void EdgeSettings::set(const string& name, double value) {
    doubleProperties[name] = value;
}

void EdgeSettings::set(const string& name, bool value) {
    boolProperties[name] = value;
}

void EdgeSettings::set(const string& name, const string& value) {
    stringProperties[name] = value;
}

double EdgeSettings::getDouble(const string& name) const {
    auto it = doubleProperties.find(name);
    return it != doubleProperties.end() ? it->second : 0.0f;
}

bool EdgeSettings::getBool(const string& name) const {
    auto it = boolProperties.find(name);
    return it != boolProperties.end() ? it->second : false;
}

string EdgeSettings::getString(const string& name) const {
    auto it = stringProperties.find(name);
    return it != stringProperties.end() ? it->second : "";
}

EdgeSettings* EdgeSettings::import(Json json) {
    auto edgesettings = new EdgeSettings();
    
    for (const auto& [key, val] : json.items()) {
        if (val.is_string()) {
            string converted_val = val.get<string>();
            edgesettings->set(key, converted_val);
        } else if (val.is_number_float()) {
            double converted_val = (double)val.get<float>();
            edgesettings->set(key, converted_val);
        } else if (val.is_number_integer()) {
            int converted_val = val.get<int>();
            edgesettings->set(key, (double)converted_val);
        } else if (val.is_boolean()) {
            bool converted_val = val.get<bool>();
            edgesettings->set(key, converted_val);
        }
    }
 
    return edgesettings;
}

Json EdgeSettings::exportJson() const {
    Json json;
    for (const auto& [key, val] : doubleProperties) {
        json[key] = val;
    }
    for (const auto& [key, val] : boolProperties) {
        json[key] = val;
    }
    for (const auto& [key, val] : stringProperties) {
        json[key] = val;
    }
    return json;
}

