#pragma once
#include <vector>
#include <memory>
#include "../shape/vec3.h"
#include "face_placement.h"

namespace ms {

class Vertex;
class PlacementSettings;
class Range;
class Matrix3;
struct ChangeMB;

class VertexPlacement {
public:
    VertexPlacement(Vertex* vertex, int id, PlacementSettings* settings);
    ~VertexPlacement() = default;

    // Core accessors
    Vertex* getVertex() const { return vertex; }
    int getId() const { return id; }
    PlacementSettings* getSettings() const { return settings; }
    const std::vector<int>& getFreeFaceIds() const { return freeFaceIds; }
    const std::vector<int>& getUnfreeFaceIds() const { return unfreeFaceIds; }
    const std::vector<int>& getColinearFaceIds() const { return colinearFaceIds; }
    const Vec3& getSlope() const { return slope; }
    const Vec3& getValue() const { return value; }

    // Operations
    void initialize();
    void addFixedNeighbor(Face* fixedFace);
    void addFreeFace(int id);
    void checkThreeFaces(int id);
    int getNumConstraints() const;
    const Vec3& getPosition() const;
    std::vector<int> getAllFaceIds() const;
    bool fixPosition();
    void addConstraint();
    void constrainFace(int id);
    void propagate();
    Range getRange();
    ChangeMB getChangeMB() const;

    // Debug
    void print() const;

    Vec3 slope;
    Vec3 value;

private:
    Vertex* vertex;
    int id;
    PlacementSettings* settings;
    std::vector<int> freeFaceIds;
    std::vector<int> unfreeFaceIds;
    std::vector<int> colinearFaceIds;
    Matrix3* M;

    // Helper methods
    Matrix3* getA(const std::vector<int>& faceIds);
    Matrix3* getM();
    void setPosition();

    struct ChangeMB {
        Vec3 m;
        Vec3 b;
    };
};

} // namespace ms