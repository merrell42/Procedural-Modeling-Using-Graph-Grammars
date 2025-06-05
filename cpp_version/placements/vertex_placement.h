#pragma once
#include <vector>
#include <memory>
#include "../geometry/vec3.h"
#include "face_placement.h"
#include "../grammar_rules/rule_applier_settings.h"
#include "../util/matrix.h"

namespace ms {

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
    VertexPlacement(Vertex* vertex, int id, RuleApplierSettings* settings) : vertex(vertex), id(id), settings(settings), M(nullptr) {}
    ~VertexPlacement() = default;

    Vertex* getVertex() const { return vertex; }
    int getId() const { return id; }
    RuleApplierSettings* getSettings() const { return settings; }
    const vector<int>& getFreeFaceIds() const { return freeFaceIds; }
    const vector<int>& getUnfreeFaceIds() const { return unfreeFaceIds; }
    const vector<int>& getCoedgearFaceIds() const { return coedgearFaceIds; }
    const Vec3& getSlope() const { return slope; }
    const Vec3& getValue() const { return value; }

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
    vector<int> coedgearFaceIds;
    Matrix* M;

    Matrix* getA(const vector<int>& faceIds);
    Matrix* getM();
};

} // namespace ms