#pragma once

#ifdef PMUGGDLL_EXPORTS
#define GENERATE_API __declspec(dllexport)
#else
#define GENERATE_API __declspec(dllimport)
#endif
#include <string>
#include "../shape/mesh.h"

namespace ms {

extern "C" {
    GENERATE_API void initialize(const char* filePath, char* result, int len);
    GENERATE_API void reset();
    GENERATE_API void iterate(int steps);
    GENERATE_API int getNumFaces();
    GENERATE_API Mesh getMesh();
}

}