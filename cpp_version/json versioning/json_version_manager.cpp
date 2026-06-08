#include "pch.h"
#include "json_version_manager.h"

vector<VersionUpdateFunction> JsonVersionManager::versionUpdates;
bool JsonVersionManager::initialized = false;

void JsonVersionManager::registerUpdateFunction(VersionUpdateFunction updateFunc) {
    versionUpdates.push_back(updateFunc);
}

void JsonVersionManager::setInitialized(bool value) {
    initialized = value;
}

bool JsonVersionManager::isInitialized() {
    return initialized;
}

void JsonVersionManager::updateToLatest(Json& json) {
    int currentVersion = json.contains("version") ? json["version"].get<int>() : 0;
    
    // Apply each update function in sequence until we reach the latest version.
    while (currentVersion < versionUpdates.size()) {
        versionUpdates[currentVersion](json);
        currentVersion++;
        json["version"] = currentVersion;
    }
}

int JsonVersionManager::getLatestVersion() {
    return (int)versionUpdates.size();
}