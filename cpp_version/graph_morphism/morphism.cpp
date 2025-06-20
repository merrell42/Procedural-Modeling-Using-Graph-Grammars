#include "pch.h"
#include "morphism.h"
#include "../graph/graph.h"
#include "../util/util.h"
#include "morphism_info.h"

Morphism::Morphism() {
    MemoryCounter::creation("Morphism");
}

Morphism::~Morphism() {
    MemoryCounter::destruction("Morphism");
}

Morphism* Morphism::create(const MorphismInfo& info) {
    auto morphism = new Morphism();
    auto graphB = info.graphB;
    morphism->vertexBtoA.resize(graphB->getVertices().size(), nullptr);
    morphism->edgeBtoA.resize(graphB->getEdges().size(), nullptr);
    return morphism;
}

const vector<Vertex*>& Morphism::getVertexBtoA() const {
    return vertexBtoA;
}

const vector<Edge*>& Morphism::getEdgeBtoA() const {
    return edgeBtoA;
}


