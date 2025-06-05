#include "pch.h"
#include "morphism.h"
#include "../graph/graph.h"
#include "../util/util.h"
#include "morphism_info.h"

namespace ms {

Morphism::Morphism() {}

Morphism* Morphism::copy() const {
    auto* result = new Morphism();
    result->vertexBtoA = vertexBtoA;
    result->edgeBtoA = edgeBtoA;
    return result;
}

Morphism* Morphism::create(const MorphismInfo& info) {
    auto map = new Morphism();
    auto netB = info.graphB;
    map->vertexBtoA.resize(netB->getVertices().size(), nullptr);
    map->edgeBtoA.resize(netB->getEdges().size(), nullptr);
    return map;
}

} // namespace ms 