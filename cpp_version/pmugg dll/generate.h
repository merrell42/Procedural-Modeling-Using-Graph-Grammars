#pragma once

#if defined(_WIN32)
  #ifdef PMUGGDLL_EXPORTS
    #define GENERATE_API __declspec(dllexport)
  #else
    #define GENERATE_API __declspec(dllimport)
  #endif
#else
  #define GENERATE_API
#endif

#include <string>
#include "../geometry/mesh.h"

// The functions that are exported from the DLL.
extern "C" {
    GENERATE_API void initialize(const char* filePath, char* result, int len, int seed);
    GENERATE_API void getLastWarning(char* result, int len);
    GENERATE_API void reset(int seed);
    GENERATE_API void iterate(int steps);
    GENERATE_API int iterateToTime(float timeSeconds);
    GENERATE_API int getNumFaces();
    GENERATE_API MeshCpp getMesh();
    GENERATE_API void setSize(float x, float y, float z);
    GENERATE_API void destroyMesh(MeshCpp& mesh);
}
