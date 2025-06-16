#pragma once
#include <string>
#include "../third_party/json.h"
#include "json_version_manager.h"

using namespace std;
using Json = nlohmann::json;

// Read in the JSON file. Update it to the latest version.
// Optionally, write a new file with the new version.
Json readJsonFile(string filePath, bool writeNewVersion = false);