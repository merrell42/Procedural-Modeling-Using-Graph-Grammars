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
    double adjustedAngle;
    double angle;
    Vec3 dir;
    int directedId;
    EdgeType* edge;
    bool isAtStart;

    HalfEdgeType(EdgeType* edge = nullptr, bool isAtStart = false, double angle = 0.0f,
              const vector<int>& faceIds = {})
        : adjustedAngle(0)
        , angle(angle)
        , dir()
        , directedId(0)
        , edge(edge)
        , isAtStart(isAtStart) {}

    HalfEdgeType copy() const {
        HalfEdgeType result;
        result.adjustedAngle = adjustedAngle;
        result.angle = angle;
        result.dir = dir;
        result.directedId = directedId;
        result.edge = edge;
        result.isAtStart = isAtStart;
        return result;
    }
};

class VertexType {
public:
    VertexType();
    ~VertexType() = default;
    static VertexType* import(const Json& json, Primitives* shape);

    const vector<HalfEdgeType>& getHalfEdgeTypes() const;
    bool getSpliced() const;
    int getId() const;

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

