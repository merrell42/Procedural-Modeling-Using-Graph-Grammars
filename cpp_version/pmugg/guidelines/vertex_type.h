#pragma once
#include "../shape/vec2.h"
#include "../shapes3D/edge_type3d.h"
#include "../shapes3D/shape3d.h"
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class EdgeType3D;
class Shape3D;

struct Connection {
    float adjustedAngle;
    float angle;
    Vec3 dir;
    int directedId;
    EdgeType3D* edge;
    bool isAtStart;

    Connection(EdgeType3D* edge = nullptr, bool isAtStart = false, float angle = 0.0f,
              const std::vector<int>& faceIds = {})
        : adjustedAngle(0)
        , angle(angle)
        , dir()
        , directedId(0)
        , edge(edge)
        , isAtStart(isAtStart) {}

    Connection copy() const {
        Connection result;
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
    static VertexType* import(const Json& json, Shape3D* shape);

    const std::vector<Connection>& getConnections() const;
    bool getSpliced() const;
    int getId() const;

    void addEdge(EdgeType3D* edge, bool isAtStart, float angle);
    void setSpliced(bool spliced);

private:
    std::vector<Connection> connections;
    bool spliced;
    int id;

    static int nextId;
    static constexpr float ANGLE_EPSILON = 1e-5f;

    static float getAdjustedAngle(float angle, EdgeType3D* edge, bool isAtStart);
};

} // namespace ms 