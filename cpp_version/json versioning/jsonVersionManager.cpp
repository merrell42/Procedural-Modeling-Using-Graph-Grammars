#include "jsonVersionManager.h"

// Define the static members
vector<VersionUpdateFunction> JsonVersionManager::versionUpdates;
bool JsonVersionManager::initialized = false; 