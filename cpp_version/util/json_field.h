#pragma once
#include <string>
#include "../third_party/json.h"

inline bool hasJson(const nlohmann::json& json, const std::string& key, bool (nlohmann::json::*isType)() const noexcept) {
    auto it = json.find(key);
    return it != json.end() && ((*it).*isType)();
}

inline bool hasJson(const nlohmann::json& json, const std::string& key) {
    auto it = json.find(key);
    return it != json.end() && !it->is_null();
}

inline bool hasString(const nlohmann::json& json, const std::string& key) {
    return hasJson(json, key, &nlohmann::json::is_string);
}

inline bool hasNumber(const nlohmann::json& json, const std::string& key) {
    return hasJson(json, key, &nlohmann::json::is_number);
}

inline bool hasArray(const nlohmann::json& json, const std::string& key) {
    return hasJson(json, key, &nlohmann::json::is_array);
}

inline bool hasObject(const nlohmann::json& json, const std::string& key) {
    return hasJson(json, key, &nlohmann::json::is_object);
}
