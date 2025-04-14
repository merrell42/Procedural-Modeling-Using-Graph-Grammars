#include "pch.h"
#include "vertex_type.h"
#include "../shapes3D/edge_type3d.h"
// #include "vertex_decoration.h"
#include "../shapes3d/shape3d.h"
#include "../util/util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

namespace ms {

int VertexType::nextId = 0;

VertexType::VertexType(/* VertexDecoration* decoration */)
    // : decoration(decoration)
    : spliced(false)
    , id(nextId++) {
}

const std::vector<Connection>& VertexType::getConnections() const {
    return connections;
}

/* VertexDecoration* VertexType::getDecoration() const {
    return decoration;
} */

bool VertexType::getSpliced() const {
    return spliced;
}

int VertexType::getId() const {
    return id;
}

void VertexType::addEdge(EdgeType3D* edge, bool isAtStart, float angle, 
                        const std::vector<int>& faceIds) {
    // Angle becomes the angle going out from the vertex through the edge
    auto dir = edge->getDir();
    if (!isAtStart) {
        angle += (float)M_PI;
        dir = dir * -1.0f;
    }
    angle = ms::Util::fixAngle(angle);
    
    int directedId = 2 * edge->getId() + (isAtStart ? 0 : 1);
    float adjustedAngle = getAdjustedAngle(angle, edge, isAtStart);

    // Find insertion point to maintain sorted order
    auto it = std::lower_bound(connections.begin(), connections.end(),
        Connection{},  // Dummy connection for comparison
        [adjustedAngle, directedId](const Connection& a, const Connection& b) {
            return (a.adjustedAngle < adjustedAngle) ||
                   (a.adjustedAngle == adjustedAngle && a.directedId < directedId);
        });

    Connection conn;
    conn.adjustedAngle = adjustedAngle;
    conn.angle = angle;
    conn.dir = dir;
    conn.directedId = directedId;
    conn.edge = edge;
    conn.faceIds = faceIds;
    conn.isAtStart = isAtStart;

    connections.insert(it, conn);
}

void VertexType::setSpliced(bool spliced) {
    this->spliced = spliced;
}

void VertexType::computeFaceIds() {
    int N = (int)connections.size();
    for (int i = 0; i < N; i++) {
        std::vector<int> faceIds = {i, (i + 1) % N};
        if (!connections[i].isAtStart) {
            std::reverse(faceIds.begin(), faceIds.end());
        }
        connections[i].faceIds = faceIds;
    }
}

VertexType* VertexType::import(const Json& json, Shape3D* shape) {
    // auto decoration = json.contains("decoration") ? 
    //     VertexDecoration::import(json["decoration"], [](){}) : nullptr;
    
    auto result = new VertexType(/*decoration*/);
    
    for (const auto& connJson : json["connections"]) {
        Connection conn;
        conn.adjustedAngle = connJson["adjustedAngle"];
        conn.angle = connJson["angle"];
        conn.dir = connJson.contains("dir") ? 
            Vec3::import(connJson["dir"]) : Vec3();
        conn.directedId = connJson["directedId"];
        conn.edge = shape->edgeTypes[connJson["edge"].get<int>()];
        conn.faceIds = connJson["faceIds"].get<std::vector<int>>();
        conn.isAtStart = connJson["isAtStart"];
        result->connections.push_back(conn);
    }
    
    result->spliced = json["spliced"];
    return result;
}

/* VertexType* VertexType::partialImport(const Json& json,
                                    const std::vector<EdgeType*>& edgeTypes) {
    auto result = new VertexType(
        new VertexDecoration([](){})  // Empty callback
    );

    for (const auto& connJson : json["connections"]) {
        auto edge = edgeTypes[connJson["edge"].get<int>()];
        std::vector<int> faceIds;
        
        if (connJson.contains("faceIds") && !connJson["faceIds"].empty()) {
            faceIds = connJson["faceIds"].get<std::vector<int>>();
        } else {
            for (const auto& datum : edge->getFaceData()) {
                faceIds.push_back(datum.type->getId());
            }
        }

        auto dir = edge->getDir();
        bool isAtStart = connJson["isAtStart"].get<bool>();
        if (!isAtStart) {
            dir = dir * -1.0f;
        }

        Connection conn;
        conn.dir = dir;
        conn.edge = edge;
        conn.faceIds = faceIds;
        conn.isAtStart = isAtStart;
        result->connections.push_back(conn);
    }

    result->id = json["id"];
    return result;
} */

/* Json VertexType::export(const Types& types) const {
    Json json;
    
    Json connsJson = Json::array();
    for (const auto& conn : connections) {
        Json connJson;
        connJson["adjustedAngle"] = conn.adjustedAngle;
        connJson["angle"] = conn.angle;
        if (conn.dir != Vec2()) {
            connJson["dir"] = conn.dir.export();
        }
        connJson["directedId"] = conn.directedId;
        connJson["edge"] = std::find(types.edgeTypes.begin(), 
                                   types.edgeTypes.end(), 
                                   conn.edge) - types.edgeTypes.begin();
        connJson["faceIds"] = conn.faceIds;
        connJson["isAtStart"] = conn.isAtStart;
        connsJson.push_back(connJson);
    }
    json["connections"] = connsJson;
    
    if (decoration) {
        json["decoration"] = decoration->export();
    }
    json["spliced"] = spliced;
    
    return json;
} */

float VertexType::getAdjustedAngle(float angle, EdgeType3D* edge, bool isAtStart) {
    int directedId = 2 * edge->getId() + (isAtStart ? 0 : 1);
    return angle + 1e-5f * directedId;
}

} // namespace ms 