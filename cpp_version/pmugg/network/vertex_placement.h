#pragma once
#include <vector>
#include <memory>
#include "vec3.h"

namespace ms {

class Vertex;
class PlacementSettings;
class Range;
class Matrix3;

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
    void addFixedNeighbor(Vertex* fixedFace);
    void addFreeFace(int id);
    void checkThreeFaces(int id);
    int getNumConstraints() const;
    const Vec3& getPosition() const;
    std::vector<int> getAllFaceIds() const;
    bool fixPosition();
    void addConstraint();
    void constrainFace(int id);
    void propagate(int id);
    Range getRange();
    ChangeMB getChangeMB() const;

    // Debug
    void print() const;

private:
    Vertex* vertex;
    int id;
    PlacementSettings* settings;
    std::vector<int> freeFaceIds;
    std::vector<int> unfreeFaceIds;
    std::vector<int> colinearFaceIds;
    Matrix3* M;
    Vec3 slope;
    Vec3 value;

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