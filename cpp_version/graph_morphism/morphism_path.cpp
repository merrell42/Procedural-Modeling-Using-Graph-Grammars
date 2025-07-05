#include "pch.h"
#include <iostream>
#include "morphism_path.h"
#include "../graph_drawing/half_edge.h"
#include "../util/util.h"

// Initialize static counter
int MorphismPath::count = 0;

MorphismPath* MorphismPath::createPath(const vector<HalfEdge*>& halfEdges,
                                        const vector<Edge*>& edges) {
    vector<EdgeInfo> edgeInfos;
    for (auto* halfEdge : halfEdges) {
        auto it = find_if(edges.begin(), edges.end(),
            [halfEdge](Edge* edge) {
                return edge == halfEdge->getEdge();
            });
        if (it == edges.end()) {
            cout << "halfEdge is not found in edges." << endl;
            return nullptr;
        }
        Edge* edge = *it;
        edgeInfos.push_back({edge, !halfEdge->getIsAtStart()});
    }
    return new MorphismPath(edgeInfos);
}

MorphismPath::MorphismPath(const vector<EdgeInfo>& edgeInfos)
    : edgeInfos(edgeInfos)
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
        if (extendable[i] && edgeInfos.size() >= 2) {
            vector<int> iIndices;
            if (i == 0) {
                iIndices = {0, 1};
            } else {
                iIndices = {static_cast<int>(edgeInfos.size() - 2),
                           static_cast<int>(edgeInfos.size() - 1)};
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
    return edgeInfos[index].edge;
}

MorphismPath::EdgeInfo MorphismPath::edgeInfoForHalfEdge(HalfEdge* halfEdge) {
    return {
        halfEdge->getEdge(),
        halfEdge->getIsAtStart()
    };
}

void MorphismPath::expandBackward() {
    auto* prevHalfEdge = halfEdges[0]->prev();
    if (prevHalfEdge) {
        halfEdges[0] = prevHalfEdge;
        edgeInfos.insert(edgeInfos.begin(), edgeInfoForHalfEdge(prevHalfEdge));
    } else {
        extendable[0] = false;
    }
}

void MorphismPath::expandForward() {
    edgeInfos.push_back(edgeInfoForHalfEdge(halfEdges[1]));
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
    edgeInfos.insert(edgeInfos.end(), pathB->edgeInfos.begin(), pathB->edgeInfos.end());
}

