#pragma once
#include <string>
#include <functional>
#include <vector>
#include "../third_party/json.h"

using namespace std;
using Json = nlohmann::json;

// Type definition for version update functions.
using VersionUpdateFunction = function<void(Json&)>;

// Manages the versioning of the JSON file, so that we can change the format of the
// JSON file without breaking existing files.
class JsonVersionManager {
public:
    // Register a function that updates JSON to the next version.
    static void registerUpdateFunction(VersionUpdateFunction updateFunc);

    static void setInitialized(bool value);

    static bool isInitialized();

    // Update JSON from its current version to the latest version.
    static void updateToLatest(Json& json);

    // Number of registered migrations; equals the latest schema version number.
    static int getLatestVersion();
    
private:
    // List of functions that update the JSON to the next version.
    static vector<VersionUpdateFunction> versionUpdates;
    static bool initialized;
};