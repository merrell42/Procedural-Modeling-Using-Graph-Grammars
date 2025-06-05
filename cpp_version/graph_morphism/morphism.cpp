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
    auto morphism = new Morphism();
    auto graphB = info.graphB;
    morphism->vertexBtoA.resize(graphB->getVertices().size(), nullptr);
    morphism->edgeBtoA.resize(graphB->getEdges().size(), nullptr);
    return morphism;
}

} // namespace ms 