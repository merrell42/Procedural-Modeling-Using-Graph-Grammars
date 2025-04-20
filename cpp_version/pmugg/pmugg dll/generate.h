#pragma once

#ifdef PMUGGDLL_EXPORTS
#define GENERATE_API __declspec(dllexport)
#else
#define GENERATE_API __declspec(dllimport)
#endif
#include <string>
#include "../shape/mesh.h"

namespace ms {

// C-compatible struct
struct Vec3_C {
    float x = 1;
    float y = 2;
    float z = 3;
};

Vec3_C testVector;

extern "C" {
    GENERATE_API int SayHello();
    GENERATE_API Vec3_C GetTestVector();
    GENERATE_API void DoubleVector();
    GENERATE_API void ResetVector();

    GENERATE_API int initialize();
    GENERATE_API void reset();
    GENERATE_API void iterate(int steps);
    GENERATE_API int getNumFaces();
    GENERATE_API Mesh getMesh();
}

}