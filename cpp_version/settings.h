#pragma once
#include "third_party/json.h"

using Json = nlohmann::json;
using namespace std;

// A set of global parameters influencing the generation process.
extern Json globalSettings;