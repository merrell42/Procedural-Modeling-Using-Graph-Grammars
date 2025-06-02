#include "pch.h"
#include "net_graph_map_state.h"
#include "net_graph_map_info.h"
#include "net_graph_map.h"
#include "half_edge_net.h"
#include "vertex_net.h"
#include "network.h"
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