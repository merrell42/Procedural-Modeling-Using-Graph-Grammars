#pragma once

#include <map>
#include <memory>
#include <vector>
#include "../geometry/vec3.h"

class Face;
struct FaceSubdivisionData;

constexpr int FACE_NOISE_OCTAVES = 4;

// Fraction of the grid spacing used to nudge samples off the lattice.
constexpr double FACE_GRID_JITTER = 0.25;

// Skip a tree branch when the conservative field bound is below this.
constexpr double FACE_NOISE_EPSILON = 1e-4;

// Subdivides faces into a grid of triangles. Each face adds an independent
// displacement field along its normal; nearby samples (in original space)
// receive the sum. Edges with two adjacent faces add a circular-arc rounding
// field. Shared edge samples stay watertight.
class FaceSubdivider {
public:
    FaceSubdivider();
    ~FaceSubdivider();

    FaceSubdivider(const FaceSubdivider&) = delete;
    FaceSubdivider& operator=(const FaceSubdivider&) = delete;

    void clear();
    void sync(const map<int, Face*>& faces, bool deform);

    void append(
        const vector<Face*>& faces,
        vector<Vec3>& positions,
        vector<Vec3>& normals,
        vector<int>& triangles,
        vector<int>& faceIndices,
        bool deform
    ) const;

private:
    unique_ptr<FaceSubdivisionData> data;
};
