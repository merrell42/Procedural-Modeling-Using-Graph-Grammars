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

struct ChangeVecMB {
    Vec3 m;
    Vec3 b;
};

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
    void checkThreeFaces();
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
    vector<int> unfreeFaceIds;

private:
    Vertex* vertex;
    int id;
    RuleApplierSettings* settings;
    vector<int> colinearFaceIds;
    Matrix* M;

    Matrix* getA(const vector<int>& faceIds);
    Matrix* getM();
};
