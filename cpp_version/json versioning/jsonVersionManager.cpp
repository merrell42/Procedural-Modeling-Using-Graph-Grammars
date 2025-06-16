#include "pch.h"
#include "jsonVersionManager.h"

vector<VersionUpdateFunction> JsonVersionManager::versionUpdates;
bool JsonVersionManager::initialized = false; 