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

// Subdivides faces into a grid of triangles and displaces the vertices.
// Samples along a shared edge are built once, so both faces get the same positions.
class FaceSubdivider {
public:
    explicit FaceSubdivider(const map<int, Face*>& faces);
    ~FaceSubdivider();

    FaceSubdivider(const FaceSubdivider&) = delete;
    FaceSubdivider& operator=(const FaceSubdivider&) = delete;

    void append(
        const vector<Face*>& faces,
        vector<Vec3>& positions,
        vector<Vec3>& normals,
        vector<int>& triangles,
        vector<int>& faceIndices
    ) const;

private:
    unique_ptr<FaceSubdivisionData> data;
};
