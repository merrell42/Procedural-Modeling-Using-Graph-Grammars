#include "pch.h"
#include <iostream>
#include "morphism_path.h"
#include "../graph_drawing/half_edge.h"
#include "../util/util.h"

// Initialize static counter
int MorphismPath::count = 0;

MorphismPath* MorphismPath::createPath(const vector<HalfEdge*>& halfEdges,
                                        const vector<Edge*>& edges,
                                        vector<Edge*>* lines) {
    vector<IndexInfo> indices;
    for (auto* halfEdge : halfEdges) {
        auto it = find_if(edges.begin(), edges.end(),
            [halfEdge](Edge* edge) {
                return edge == halfEdge->getEdge();
            });
        if (it == edges.end()) {
            cout << "halfEdge is not found in edges." << endl;
            return nullptr;
        }
        int index = (int)distance(edges.begin(), it);
        indices.push_back({index, !halfEdge->getIsAtStart()});
    }
    return new MorphismPath(indices, lines);
}

MorphismPath::MorphismPath(const vector<IndexInfo>& indices,
                             vector<Edge*>* edges)
    : indices(indices)
    , edges(edges)
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

int MorphismPath::extendableness() const {
    return extendable[0] + extendable[1];
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
        if (extendable[i] && indices.size() >= 2) {
            vector<int> iIndices;
            if (i == 0) {
                iIndices = {0, 1};
            } else {
                iIndices = {static_cast<int>(indices.size() - 2),
                           static_cast<int>(indices.size() - 1)};
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
    return (*edges)[indices[index].index];
}

MorphismPath::IndexInfo MorphismPath::indexForHalfEdge(HalfEdge* halfEdge) {
    auto it = find_if(edges->begin(), edges->end(), 
        [halfEdge](Edge* edge) {
            return edge == halfEdge->getEdge();
        });
    return {
        static_cast<int>(distance(edges->begin(), it)),
        halfEdge->getIsAtStart()
    };
}

void MorphismPath::expandBackward() {
    auto* prevHalfEdge = halfEdges[0]->prev();
    if (prevHalfEdge) {
        halfEdges[0] = prevHalfEdge;
        indices.insert(indices.begin(), indexForHalfEdge(prevHalfEdge));
    } else {
        extendable[0] = false;
    }
}

void MorphismPath::expandForward() {
    indices.push_back(indexForHalfEdge(halfEdges[1]));
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
    indices.insert(indices.end(), pathB->indices.begin(), pathB->indices.end());
}

