#include "net_graph_map.h"
#include "network.h"
#include "line.h"
#include "vertex.h"
#include "edge.h"
#include "endpoint.h"
#include "face.h"
#include "util.h"

namespace ms {

NetGraphMap::NetGraphMap(Network* netB, Line* line, bool groundEnabled)
    : valid(false) {
    initialize(netB, line, groundEnabled);
}

void NetGraphMap::initialize(Network* netB, Line* line, bool groundEnabled) {
    auto interiorB = netB->getInterior();
    vertexBtoA.resize(interiorB->getVertices().size(), -1);
    edgeBtoA.resize(interiorB->getEdges().size(), -1);
    valid = tryMap(netB, line, groundEnabled);
}

NetGraphMap* NetGraphMap::copy() const {
    auto* result = new NetGraphMap();
    result->vertexBtoA = vertexBtoA;
    result->edgeBtoA = edgeBtoA;
    result->valid = valid;
    return result;
}

bool NetGraphMap::tryMap(Network* netB, Line* line, bool groundEnabled) {
    auto interiorB = netB->getInterior();
    auto vertices = interiorB->getVertices();
    auto edges = interiorB->getEdges();

    // Try to map each vertex
    for (size_t i = 0; i < vertices.size(); i++) {
        if (!tryMapVertex(netB, i, line)) {
            return false;
        }
    }

    // Try to map each edge
    for (size_t i = 0; i < edges.size(); i++) {
        if (!tryMapEdge(netB, i, line)) {
            return false;
        }
    }

    // Check for ground faces if needed
    if (groundEnabled) {
        bool hasGround = false;
        for (auto* face : interiorB->getFaces()) {
            if (face->getPrimal()->getType()->getNormal().z > 0.999) {
                hasGround = true;
                break;
            }
        }
        if (!hasGround) {
            return false;
        }
    }

    return true;
}

bool NetGraphMap::tryMapVertex(Network* netB, int vertexB, Line* line) {
    auto* vertex = netB->getInterior()->getVertices()[vertexB];
    auto* vertexType = vertex->getPrimal()->getType();
    auto endpoints = line->getEndpoints();

    // Try to map to each endpoint
    for (size_t i = 0; i < endpoints.size(); i++) {
        auto* endpoint = endpoints[i];
        if (!endpoint) continue;

        auto* endpointType = endpoint->getVertexState()->getType();
        if (vertexType == endpointType) {
            vertexBtoA[vertexB] = i;
            return true;
        }
    }

    return false;
}

bool NetGraphMap::tryMapEdge(Network* netB, int edgeB, Line* line) {
    auto* edge = netB->getInterior()->getEdges()[edgeB];
    auto* edgeType = edge->getPrimal()->getType();
    auto segments = line->getSegments();

    // Try to map to each segment
    for (size_t i = 0; i < segments.size(); i++) {
        auto* segment = segments[i];
        if (!segment) continue;

        auto* segmentType = segment->getEdgeType();
        if (edgeType == segmentType) {
            // Check vertex consistency
            auto* halfEdge = edge->getHalfEdges()[0][0];
            auto vertexIndex = vertexBtoA[netB->vertexIndex(halfEdge->getVertex())];
            auto nextVertexIndex = vertexBtoA[netB->vertexIndex(halfEdge->getNext()->getVertex())];

            if ((vertexIndex == i && nextVertexIndex == i + 1) ||
                (vertexIndex == i + 1 && nextVertexIndex == i)) {
                edgeBtoA[edgeB] = i;
                return true;
            }
        }
    }

    return false;
}

} // namespace ms 