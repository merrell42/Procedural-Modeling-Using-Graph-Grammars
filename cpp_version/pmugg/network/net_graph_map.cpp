#include "net_graph_map.h"
#include "network.h"
// #include "vertex.h"
// #include "edge.h"
// #include "endpoint.h"
// #include "face.h"
#include "../Util/util.h"
#include "net_graph_map_info.h"

namespace ms {

NetGraphMap::NetGraphMap() {}

NetGraphMap* NetGraphMap::copy() const {
    auto* result = new NetGraphMap();
    result->vertexBtoA = vertexBtoA;
    result->edgeBtoA = edgeBtoA;
    return result;
}

std::unique_ptr<NetGraphMap> NetGraphMap::create(const NetGraphMapInfo& info) {
    auto netB = info.networkB; // Assuming netB is accessible from info

    // Create a new instance of NetGraphMap using unique_ptr
    auto map = std::make_unique<NetGraphMap>();

    // Initialize vertexBtoA and edgeBtoA with null pointers
    map->vertexBtoA.resize(netB->getVertices().size(), nullptr);
    map->edgeBtoA.resize(netB->getEdges().size(), -1);

    return map; // Return the unique_ptr
}

// bool NetGraphMap::tryMap(Network* netB, Line* line, bool groundEnabled) {
//     auto vertices = netB->getVertices();
//     auto edges = netB->getEdges();

//     // Try to map each vertex
//     for (size_t i = 0; i < vertices.size(); i++) {
//         if (!tryMapVertex(netB, i, line)) {
//             return false;
//         }
//     }

//     // Try to map each edge
//     for (size_t i = 0; i < edges.size(); i++) {
//         if (!tryMapEdge(netB, i, line)) {
//             return false;
//         }
//     }

//     // Check for ground faces if needed
//     // if (groundEnabled) {
//     //     bool hasGround = false;
//     //     for (auto* face : netB->getFaces()) {
//     //         if (face->getType()->getNormal().z > 0.999) {
//     //             hasGround = true;
//     //             break;
//     //         }
//     //     }
//     //     if (!hasGround) {
//     //         return false;
//     //     }
//     // }

//     return true;
// }

// bool NetGraphMap::tryMapVertex(Network* netB, int vertexB, Line* line) {
//     auto* vertex = netB->getVertices()[vertexB];
//     auto* vertexType = vertex->getType();
//     auto endpoints = line->getEndpoints();

//     // Try to map to each endpoint
//     for (size_t i = 0; i < endpoints.size(); i++) {
//         auto* endpoint = endpoints[i];
//         if (!endpoint) continue;

//         auto* endpointType = endpoint->getVertex()->getType();
//         if (vertexType == endpointType) {
//             vertexBtoA[vertexB] = i;
//             return true;
//         }
//     }

//     return false;
// }

// bool NetGraphMap::tryMapEdge(Network* netB, int edgeB, Line* line) {
//     auto* edge = netB->getEdges()[edgeB];
//     auto* edgeType = edge->getType();
//     auto segments = line->getSegments();

//     // Try to map to each segment
//     for (size_t i = 0; i < segments.size(); i++) {
//         auto* segment = segments[i];
//         if (!segment) continue;

//         auto* segmentType = segment->getEdgeType();
//         if (edgeType == segmentType) {
//             // Check vertex consistency
//             auto* halfEdge = edge->getHalfEdges()[0][0];
//             auto vertexIndex = vertexBtoA[netB->vertexIndex(halfEdge->getVertex())];
//             auto nextVertexIndex = vertexBtoA[netB->vertexIndex(halfEdge->getNext()->getVertex())];

//             if ((vertexIndex == i && nextVertexIndex == i + 1) ||
//                 (vertexIndex == i + 1 && nextVertexIndex == i)) {
//                 edgeBtoA[edgeB] = i;
//                 return true;
//             }
//         }
//     }

//     return false;
// }

} // namespace ms 