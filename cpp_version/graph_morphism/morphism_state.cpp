#include "pch.h"
#include "morphism_state.h"
#include "morphism_info.h"
#include "morphism.h"
#include "../graph/graph_half_edge.h"
#include "../graph/graph_vertex.h"
#include "../graph/graph.h"
#include "../util/util.h"

MorphismState::MorphismState(MorphismInfo* info) : info(info) {
    MemoryCounter::creation("MorphismState");
    morphism = Morphism::create(*info);
}

MorphismState::~MorphismState() {
    MemoryCounter::destruction("MorphismState");
}

void MorphismState::assignVertex(Vertex* vertexA, int indexB) {
    auto* vertexB = info->graphB->getVertices()[indexB];
    morphism->vertexBtoA[indexB] = vertexA;

    // Add all non-spliced half edges to queue and spliced ones to spliceQueue.
    for (auto* halfB : vertexB->getHalfEdges()) {
        if (!halfB) {
            continue;
        }
        HalfEdgeData data(halfB, vertexA);
        if (halfB->isSpliced()) {
            spliceQueue.push_back(data);
        } else {
            queue.push_back(data);
        }
    }
}

MorphismInfo* MorphismState::getInfo() const {
    return info;
}

Morphism* MorphismState::getMorphism() {
    return morphism;
}

vector<HalfEdgeData>& MorphismState::getQueue() {
    return queue;
}

vector<HalfEdgeData>& MorphismState::getSpliceQueue() {
    return spliceQueue;
}
