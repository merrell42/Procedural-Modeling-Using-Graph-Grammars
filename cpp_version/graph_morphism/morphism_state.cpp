#include "pch.h"
#include "morphism_state.h"
#include "morphism_info.h"
#include "morphism.h"
#include "../graph/graph_half_edge.h"
#include "../graph/graph_vertex.h"
#include "../graph/graph.h"
#include "../util/util.h"

MorphismState::MorphismState(MorphismInfo* info, Morphism* existingMorphism)
    : info(info) {
    if (existingMorphism) {
        morphism = existingMorphism->copy();
    } else {
        morphism = Morphism::create(*info);
    }
}

void MorphismState::setQueue(const vector<HalfEdgeData>& newQueue) {
    queue = newQueue;
}

void MorphismState::assignVertex(Vertex* vertexA, int indexB) {
    auto* vertexB = info->graphB->getVertices()[indexB];
    morphism->vertexBtoA[indexB] = vertexA;

    // Add all non-spliced half edges to queue and spliced ones to spliceQueue.
    for (auto* halfB : vertexB->getHalfEdges()) {
        if (!halfB) {
            continue;
        }
        if (halfB->isSpliced()) {
            spliceQueue.push_back(HalfEdgeData(halfB, vertexA));
        } else {
            queue.push_back(HalfEdgeData(halfB, vertexA));
        }
    }
}

MorphismState* MorphismState::copy() const {
    auto* result = new MorphismState(info, morphism);
    result->queue = queue;
    result->spliceQueue = spliceQueue;
    return result;
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
