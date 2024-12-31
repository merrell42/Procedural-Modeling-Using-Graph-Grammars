#pragma once
#include "../shape/vec2.h"
#include "../node/node.h"
#include "../shapes3D/edge_type3d.h"
#include "../shapes3D/shape3d.h"
#include <vector>
#include <memory>
#include "../third_party/json.h"

using Json = nlohmann::json;

namespace ms {

class EdgeType3D;
class Shape3D;
// class VertexDecoration;

struct Connection {
    float adjustedAngle;
    float angle;
    Vec3 dir;
    int directedId;
    EdgeType3D* edge;
    std::vector<int> faceIds;
    bool isAtStart;

    Connection(EdgeType3D* edge = nullptr, bool isAtStart = false, float angle = 0.0f,
              const std::vector<int>& faceIds = {})
        : adjustedAngle(0)
        , angle(angle)
        , dir()
        , directedId(0)
        , edge(edge)
        , faceIds(faceIds)
        , isAtStart(isAtStart) {}

    Connection copy() const {
        Connection result;
        result.adjustedAngle = adjustedAngle;
        result.angle = angle;
        result.dir = dir;
        result.directedId = directedId;
        result.edge = edge;
        result.faceIds = faceIds;
        result.isAtStart = isAtStart;
        return result;
    }
};

class VertexType {
public:
    VertexType(/* VertexDecoration* decoration = nullptr */);
    ~VertexType() = default;

    // Core functionality
    const std::vector<Connection>& getConnections() const;
    // VertexDecoration* getDecoration() const;
    bool getSpliced() const;
    int getId() const;

    // Operations
    void addEdge(EdgeType3D* edge, bool isAtStart, float angle, const std::vector<int>& faceIds);
    void setSpliced(bool spliced);
    void computeFaceIds();

    // Import/Export
    static VertexType* import(const Json& json, Shape3D* shape); 
    // static VertexType* partialImport(const Json& json, const std::vector<EdgeType*>& edgeTypes);
    // Json export(const Types& types) const;

private:
    std::vector<Connection> connections;
    // VertexDecoration* decoration;
    bool spliced;
    int id;

    static int nextId;
    static constexpr float ANGLE_EPSILON = 1e-5f;

    // Helper methods
    static float getAdjustedAngle(float angle, EdgeType3D* edge, bool isAtStart);
};

} // namespace ms 