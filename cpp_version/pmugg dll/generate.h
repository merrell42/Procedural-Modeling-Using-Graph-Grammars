#pragma once

#ifdef PMUGGDLL_EXPORTS
#define GENERATE_API __declspec(dllexport)
#else
#define GENERATE_API __declspec(dllimport)
#endif
#include <string>
#include "../geometry/mesh.h"

extern "C" {
    GENERATE_API void initialize(const char* filePath, char* result, int len, int seed);
    GENERATE_API void reset(int seed);
    GENERATE_API void iterate(int steps);
    GENERATE_API int getNumFaces();
    GENERATE_API Mesh getMesh();
    GENERATE_API void setSize(float x, float y, float z);
}
