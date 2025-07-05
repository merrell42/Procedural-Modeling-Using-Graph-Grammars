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
}

MorphismPath::~MorphismPath() {
    MemoryCounter::destruction("MorphismPath");
}

void MorphismPath::setHalfEdges(const vector<HalfEdge*>& halfEdges) {
    this->halfEdges = halfEdges;
}

bool MorphismPath::isExtendable() const {
    return extendable[0] || extendable[1];
}

Vertex* MorphismPath::randomNextVertex() {
    vector<double> probabilities;
    for (bool e : extendable) {
        probabilities.push_back(e ? 1.0 : 0.0);
    }
    int index = Util::randomDistribution(probabilities);
    return halfEdges[index]->getVertex();
}

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
                auto* edge = edgeFromIndex(iIndices[j]);
                rigid = rigid && !edge->getEdgeType()->extendable();
            }
            if (rigid) {
                return halfEdges[i]->getVertex();
            }
        }
    }
    return nullptr;
}

Edge* MorphismPath::edgeFromIndex(int index) {
    return pathEdges[index];
}

void MorphismPath::expandBackward() {
    auto* prevHalfEdge = halfEdges[0]->prev();
    if (prevHalfEdge) {
        halfEdges[0] = prevHalfEdge;
        pathEdges.insert(pathEdges.begin(), prevHalfEdge->getEdge());
    } else {
        extendable[0] = false;
    }
}

void MorphismPath::expandForward() {
    pathEdges.push_back(halfEdges[1]->getEdge());
    auto* nextHalfEdge = halfEdges[1]->next();
    if (nextHalfEdge) {
        halfEdges[1] = nextHalfEdge;
    } else {
        extendable[1] = false;
    }
}

void MorphismPath::merge(MorphismPath* pathB) {
    halfEdges[1] = pathB->halfEdges[1];
    extendable[1] = pathB->extendable[1];
    pathEdges.insert(pathEdges.end(), pathB->pathEdges.begin(), pathB->pathEdges.end());
}

