#include "net_graph_map_state.h"
#include "net_graph_map_info.h"
#include "net_graph_map.h"
#include "half_edge.h"
#include "vertex.h"
#include "network.h"
#include "util.h"

namespace ms {

NetGraphMapState::NetGraphMapState(NetGraphMapInfo* info, NetGraphMap* existingMap)
    : info(info) {
    if (existingMap) {
        map = std::unique_ptr<NetGraphMap>(existingMap->copy());
    } else {
        map = std::make_unique<NetGraphMap>(info->netB);
    }
}

void NetGraphMapState::setQueue(const std::vector<HalfEdgeData>& newQueue) {
    queue = newQueue;
}

void NetGraphMapState::assignVertex(Vertex* vertexA, int indexB) {
    auto* vertexB = info->netB->getInterior()->getVertices()[indexB];
    map->vertexBtoA[indexB] = vertexA;

    // Add all non-spliced half edges to queue and spliced ones to spliceQueue
    for (auto* halfB : vertexB->getHalfEdges()) {
        if (!halfB) continue;

        if (halfB->isSpliced()) {
            spliceQueue.push_back(HalfEdgeData(halfB, vertexA));
        } else {
            queue.push_back(HalfEdgeData(halfB, vertexA));
        }
    }
}

void NetGraphMapState::assignHalf(int indexA, int indexB) {
    map->halfAtoB[indexA] = indexB;
    map->halfBtoA[indexB] = indexA;
}

NetGraphMapState* NetGraphMapState::copy() const {
    auto* result = new NetGraphMapState(info, map.get());
    result->queue = queue;
    result->spliceQueue = spliceQueue;
    return result;
}

} // namespace ms 