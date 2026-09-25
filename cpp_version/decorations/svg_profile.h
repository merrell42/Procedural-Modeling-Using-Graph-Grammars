#pragma once
#include <string>
#include <vector>
#include "../geometry/vec2.h"

using namespace std;

vector<Vec2> loadSvgProfile(const string& name, const string& directory);
