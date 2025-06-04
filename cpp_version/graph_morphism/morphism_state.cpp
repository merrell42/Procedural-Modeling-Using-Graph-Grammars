#include "pch.h"
#include "morphism_state.h"
#include "morphism_info.h"
#include "morphism.h"
#include "../graph/graph_half_edge.h"
#include "../graph/graph_vertex.h"
#include "../graph/graph.h"
#include "../util/util.h"

namespace ms {

NetGraphMapState::NetGraphMapState(NetGraphMapInfo* info, NetGraphMap* existingMap)
    : info(info) {
    if (existingMap) {
        map = existingMap->copy();
    } else {
        map = NetGraphMap::create(*info);
    }
}

void NetGraphMapState::setQueue(const std::vector<EndpointData>& newQueue) {
    queue = newQueue;
}

void NetGraphMapState::assignVertex(Vertex* vertexA, int indexB) {
    auto* vertexB = info->networkB->getVertices()[indexB];
    map->vertexBtoA[indexB] = vertexA;

    // Add all non-spliced half edges to queue and spliced ones to spliceQueue.
    for (auto* halfB : vertexB->getHalfEdges()) {
        if (!halfB) {
            continue;
        }
        if (halfB->isSpliced()) {
            spliceQueue.push_back(EndpointData(halfB, vertexA));
        } else {
            queue.push_back(EndpointData(halfB, vertexA));
        }
    }
}

NetGraphMapState* NetGraphMapState::copy() const {
    auto* result = new NetGraphMapState(info, map);
    result->queue = queue;
    result->spliceQueue = spliceQueue;
    return result;
}

} // namespace ms 