#pragma once
#include <string>
#include <functional>
#include <vector>
#include "../third_party/json.h"

using namespace std;
using Json = nlohmann::json;

// Type definition for version update functions
using VersionUpdateFunction = function<void(Json&)>;

class JsonVersionManager {
public:
    // Register a function that updates JSON to the next version.
    static void registerUpdateFunction(VersionUpdateFunction updateFunc) {
        versionUpdates.push_back(updateFunc);
    }

    static void setInitialized(bool value) {
        initialized = value;
    }

    static bool isInitialized() {
        return initialized;
    }

    // Update JSON from its current version to the lastest version.
    static void updateToLatest(Json& json) {
        int currentVersion = json.contains("version") ? json["version"].get<int>() : 0;
        
        // Apply each update function in sequence until we reach the latest version.
        while (currentVersion < versionUpdates.size()) {
            // Apply the update function
            versionUpdates[currentVersion](json);
            currentVersion++;
            json["version"] = currentVersion;
        }
    }
    
private:
    // Vector of version update functions, where index i updates from version i to i+1.
    static vector<VersionUpdateFunction> versionUpdates;
    static bool initialized;
};