#include "pch.h"
#include "json_version_manager.h"

vector<VersionUpdateFunction> JsonVersionManager::versionUpdates;
bool JsonVersionManager::initialized = false; 