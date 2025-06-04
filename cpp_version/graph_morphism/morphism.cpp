#include "pch.h"
#include "morphism.h"
#include "../graph/graph.h"
#include "../util/util.h"
#include "morphism_info.h"

namespace ms {

NetGraphMap::NetGraphMap() {}

NetGraphMap* NetGraphMap::copy() const {
    auto* result = new NetGraphMap();
    result->vertexBtoA = vertexBtoA;
    result->edgeBtoA = edgeBtoA;
    return result;
}

NetGraphMap* NetGraphMap::create(const NetGraphMapInfo& info) {
    auto map = new NetGraphMap();
    auto netB = info.networkB;
    map->vertexBtoA.resize(netB->getVertices().size(), nullptr);
    map->edgeBtoA.resize(netB->getEdges().size(), nullptr);
    return map;
}

} // namespace ms 