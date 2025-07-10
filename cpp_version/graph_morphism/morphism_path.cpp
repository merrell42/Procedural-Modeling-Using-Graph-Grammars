#include "pch.h"
#include <iostream>
#include "morphism_path.h"
#include "../graph_drawing/half_edge.h"
#include "../util/util.h"

// Initialize static counter
int MorphismPath::count = 0;

MorphismPath* MorphismPath::createPath(const vector<HalfEdge*>& halfEdges) {
    vector<Edge*> pathEdges;
    for (auto* halfEdge : halfEdges) {
        pathEdges.push_back(halfEdge->getEdge());
    }
    return new MorphismPath(pathEdges);
}

MorphismPath::MorphismPath(const vector<Edge*>& pathEdges)
    : pathEdges(pathEdges)
    , extendable{true, true}
    , id(count++) {
    MemoryCounter::creation("MorphismPath");
    // Initialize to nullptr. They are set by setHalfEdges.
    halfEdges[0] = nullptr;
    halfEdges[1] = nullptr;
}

MorphismPath::~MorphismPath() {
    MemoryCounter::destruction("MorphismPath");
}

void MorphismPath::setHalfEdges(HalfEdge* halfEdges[2]) {
    this->halfEdges[0] = halfEdges[0];
    this->halfEdges[1] = halfEdges[1];
}

bool MorphismPath::isExtendable() const {
    return extendable[0] || extendable[1];
}

// Pick a random vertex from either end of the path that is extendable.
Vertex* MorphismPath::randomNextVertex() {
    vector<double> probabilities;
    for (bool e : extendable) {
        probabilities.push_back(e ? 1.0 : 0.0);
    }
    int index = Util::randomDistribution(probabilities);
    return halfEdges[index]->getVertex();
}

// This is to handle shapes with rigid edges that have a fixed length.
Vertex* MorphismPath::rigidNextVertex() {
    for (int i = 0; i < 2; i++) {
        if (extendable[i] && pathEdges.size() >= 2) {
            vector<int> iIndices;
            if (i == 0) {
                iIndices = {0, 1};
            } else {
                iIndices = {static_cast<int>(pathEdges.size() - 2),
                           static_cast<int>(pathEdges.size() - 1)};
            }
            // Two consecutive indices must be rigid.
            bool rigid = true;
            for (int j = 0; j < 2; j++) {
                auto* edge = pathEdges[iIndices[j]];
                rigid = rigid && !edge->getEdgeType()->extendable();
            }
            if (rigid) {
                return halfEdges[i]->getVertex();
            }
        }
    }
    return nullptr;
}

// Move the start of the path to the previous half-edge.
// If none exists, the path is no longer extendable.
void MorphismPath::expandBackward() {
    auto* prevHalfEdge = halfEdges[0]->prev();
    if (prevHalfEdge) {
        halfEdges[0] = prevHalfEdge;
        pathEdges.insert(pathEdges.begin(), prevHalfEdge->getEdge());
    } else {
        extendable[0] = false;
    }
}

// Move the end of the path to the next half-edge.
// If none exists, the path is no longer extendable.
void MorphismPath::expandForward() {
    pathEdges.push_back(halfEdges[1]->getEdge());
    auto* nextHalfEdge = halfEdges[1]->next();
    if (nextHalfEdge) {
        halfEdges[1] = nextHalfEdge;
    } else {
        extendable[1] = false;
    }
}

// Merge pathB onto the end of this path.
void MorphismPath::merge(MorphismPath* pathB) {
    halfEdges[1] = pathB->halfEdges[1];
    extendable[1] = pathB->extendable[1];
    pathEdges.insert(pathEdges.end(), pathB->pathEdges.begin(), pathB->pathEdges.end());
}
