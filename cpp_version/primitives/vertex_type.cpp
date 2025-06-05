#include "pch.h"
#include "vertex_type.h"
#include "edge_type.h"
#include "primitives.h"
#include "../util/util.h"
#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>

namespace ms {

int VertexType::nextId = 0;

VertexType::VertexType()
    : spliced(false)
    , id(nextId++) {
}

const vector<Connection>& VertexType::getConnections() const {
    return connections;
}

bool VertexType::getSpliced() const {
    return spliced;
}

int VertexType::getId() const {
    return id;
}

void VertexType::addEdge(EdgeType* edge, bool isAtStart, double angle) {
    // Angle becomes the angle going out from the vertex through the edge.
    auto dir = edge->getDir();
    if (!isAtStart) {
        angle += (double)M_PI;
        dir = dir * -1.0f;
    }
    angle = ms::Util::fixAngle(angle);
    
    int directedId = 2 * edge->getId() + (isAtStart ? 0 : 1);
    double adjustedAngle = getAdjustedAngle(angle, edge, isAtStart);

    // Find insertion point to maintain sorted order.
    auto it = lower_bound(connections.begin(), connections.end(),
        Connection{},
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
    conn.isAtStart = isAtStart;

    connections.insert(it, conn);
}

void VertexType::setSpliced(bool spliced) {
    this->spliced = spliced;
}

VertexType* VertexType::import(const Json& json, Primitives* shape) {    
    auto result = new VertexType();
    bool spliced = json["spliced"];
    result->spliced = spliced;

    for (const auto& connJson : json["connections"]) {
        Connection conn;
        conn.edge = shape->edgeTypes[connJson["edge"].get<int>()];
        conn.isAtStart = connJson["isAtStart"];
        if (!spliced) {
            conn.adjustedAngle = connJson["adjustedAngle"];
            conn.angle = connJson["angle"];
            conn.dir = connJson.contains("dir") ? 
                Vec3::import(connJson["dir"]) : Vec3();
            conn.directedId = connJson["directedId"];
        }
        result->connections.push_back(conn);
    }

    return result;
}

double VertexType::getAdjustedAngle(double angle, EdgeType* edge, bool isAtStart) {
    int directedId = 2 * edge->getId() + (isAtStart ? 0 : 1);
    return angle + 1e-5f * directedId;
}

} // namespace ms 