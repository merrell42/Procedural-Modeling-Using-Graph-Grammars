#pragma once
#include "../geometry/vec2.h"
#include "edge_type.h"
#include "primitives.h"
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

class EdgeType;
class Primitives;

struct HalfEdgeType {
    HalfEdgeType(EdgeType* newEdge = nullptr, bool newIsAtStart = false, double newAngle = 0.0f,
            const vector<int>& faceIds = {});
        
    double adjustedAngle;
    double angle;
    Vec3 dir;
    int directedId;
    EdgeType* edge;
    bool isAtStart;
};

class VertexType {
public:
    VertexType();
    ~VertexType() = default;
    static VertexType* import(const Json& json, Primitives* shape);

    const vector<HalfEdgeType>& getHalfEdgeTypes() const;
    bool getSpliced() const;

    void addEdge(EdgeType* edge, bool isAtStart, double angle);
    void setSpliced(bool spliced);

private:
    vector<HalfEdgeType> halfEdgeTypes;
    bool spliced;
    int id;

    static int nextId;
    static constexpr double ANGLE_EPSILON = 1e-5f;

    static double getAdjustedAngle(double angle, EdgeType* edge, bool isAtStart);
};

