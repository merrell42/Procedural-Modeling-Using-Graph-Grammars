#pragma once
#include <vector>
#include <memory>
#include "../shape/vec3.h"
#include "face_placement.h"
#include "../fragment/net_transistor_settings.h"
#include "../util/fast_math.h"

namespace ms {

class Endpoint;
class Vertex;
class NetTransistorSettings;
class Range;
class Matrix;

struct ChangeVecMB {
    Vec3 m;
    Vec3 b;
};

class VertexPlacement {
public:
    VertexPlacement(Vertex* vertex, int id, NetTransistorSettings* settings) : vertex(vertex), id(id), settings(settings), M(nullptr) {}
    ~VertexPlacement() = default;

    Vertex* getVertex() const { return vertex; }
    int getId() const { return id; }
    NetTransistorSettings* getSettings() const { return settings; }
    const std::vector<int>& getFreeFaceIds() const { return freeFaceIds; }
    const std::vector<int>& getUnfreeFaceIds() const { return unfreeFaceIds; }
    const std::vector<int>& getColinearFaceIds() const { return colinearFaceIds; }
    const Vec3& getSlope() const { return slope; }
    const Vec3& getValue() const { return value; }

    void initialize();
    void addFixedNeighbor(const FixedFace& fixedFace);
    void addFreeFace(int id);
    void checkThreeFaces();
    int getNumConstraints() const;
    const Vec3& getPosition() const;
    std::vector<int> getAllFaceIds() const;
    bool fixPosition();
    void addConstraint();
    void constrainFace(int id);
    void propagate();
    Range getRange();
    ChangeVecMB getChangeMB() const;
    void setPosition();

    Vec3 slope;
    Vec3 value;
    std::vector<int> freeFaceIds;
    std::vector<int> unfreeFaceIds;

private:
    Vertex* vertex;
    int id;
    NetTransistorSettings* settings;
    std::vector<int> colinearFaceIds;
    Matrix* M;

    Matrix* getA(const std::vector<int>& faceIds);
    Matrix* getM();
};

} // namespace ms