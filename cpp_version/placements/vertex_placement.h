#pragma once
#include <vector>
#include <memory>
#include "../geometry/vec3.h"
#include "face_placement.h"
#include "../grammar_rules/rule_applier_settings.h"
#include "../util/matrix.h"
#include "../memory_counter.h"

class HalfEdge;
class Vertex;
class RuleApplierSettings;
class Range;
class Matrix;

// Represents possible changes to the position of a vertex along a basis direction.
// b is the initial value, m is the direction of change.
struct ChangeVecMB {
    Vec3 m;
    Vec3 b;
};

// Represents the location of a vertex in the graph drawing.
class VertexPlacement {
public:
    VertexPlacement(Vertex* vertex, int id, RuleApplierSettings* settings) : vertex(vertex), id(id), settings(settings), M(nullptr) {
        MemoryCounter::creation("VertexPlacement");
    }
    ~VertexPlacement() {
        delete M;
        MemoryCounter::destruction("VertexPlacement");
    }

    void initialize();
    void addFixedNeighbor(const FixedFace& fixedFace);
    void addFreeFace(int id);
    void guaranteeThreeFaces();
    int getNumConstraints() const;
    const Vec3& getPosition() const;
    vector<int> getAllFaceIds() const;
    bool fixPosition();
    void addConstraint();
    void constrainFace(int id);
    void propagate();
    Range getRange();
    ChangeVecMB getChangeMB() const;
    void setPosition();

    Vec3 slope;
    Vec3 value;
    vector<int> freeFaceIds;
    vector<int> constrainedFaceIds;

private:
    Vertex* vertex;
    int id;
    RuleApplierSettings* settings;
    vector<int> colinearFaceIds;
    Matrix* M;

    // The vertex is at the intersection of three planes.
    // Multiply M times the d-values for each plane to get the position.
    Matrix* getM();
    // A is the inverse of M. A depends on the normals of the three planes.
    Matrix* getA(const vector<int>& faceIds);
};
