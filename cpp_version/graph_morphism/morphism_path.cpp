#include "pch.h"
#include <iostream>
#include "morphism_path.h"
#include "../graph_drawing/half_edge.h"
#include "../util/util.h"

namespace ms {

// Initialize static counter
int MorphismPath::count = 0;

MorphismPath* MorphismPath::createPath(const std::vector<HalfEdge*>& halfedges,
                                        const std::vector<Edge*>& edges,
                                        std::vector<Edge*>* lines) {
    std::vector<IndexInfo> indices;
    for (auto* halfedge : halfedges) {
        auto it = std::find_if(edges.begin(), edges.end(),
            [halfedge](Edge* edge) {
                return edge == halfedge->getEdge();
            });
        if (it == edges.end()) {
            std::cout << "halfedge is not found in edges." << std::endl;
            return nullptr;
        }
        int index = (int)std::distance(edges.begin(), it);
        indices.push_back({index, !halfedge->getIsAtStart()});
    }
    return new MorphismPath(indices, lines);
}

MorphismPath::MorphismPath(const std::vector<IndexInfo>& indices,
                             std::vector<Edge*>* edges)
    : indices(indices)
    , edges(edges)
    , extendable{true, true}
    , id(count++) {
}

void MorphismPath::setHalfEdges(const std::vector<HalfEdge*>& halfedges) {
    this->halfedges = halfedges;
}

int MorphismPath::extendableness() const {
    return extendable[0] + extendable[1];
}

Vertex* MorphismPath::randomNextVertex() {
    std::vector<double> probabilities;
    for (bool e : extendable) {
        probabilities.push_back(e ? 1.0 : 0.0);
    }
    int index = Util::randomDistribution(probabilities);
    return halfedges[index]->getVertex();
}

Vertex* MorphismPath::rigidNextVertex() {
    for (int i = 0; i < 2; i++) {
        if (extendable[i] && indices.size() >= 2) {
            std::vector<int> iIndices;
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
                return halfedges[i]->getVertex();
            }
        }
    }
    return nullptr;
}

Edge* MorphismPath::edgeFromIndex(int index) {
    return (*edges)[indices[index].index];
}

MorphismPath::IndexInfo MorphismPath::indexForHalfEdge(HalfEdge* halfedge) {
    auto it = std::find_if(edges->begin(), edges->end(), 
        [halfedge](Edge* edge) {
            return edge == halfedge->getEdge();
        });
    return {
        static_cast<int>(std::distance(edges->begin(), it)),
        halfedge->getIsAtStart()
    };
}

void MorphismPath::expandBackward() {
    auto* prevHalfEdge = halfedges[0]->prev();
    if (prevHalfEdge) {
        halfedges[0] = prevHalfEdge;
        indices.insert(indices.begin(), indexForHalfEdge(prevHalfEdge));
    } else {
        extendable[0] = false;
    }
}

void MorphismPath::expandForward() {
    indices.push_back(indexForHalfEdge(halfedges[1]));
    auto* nextHalfEdge = halfedges[1]->next();
    if (nextHalfEdge) {
        halfedges[1] = nextHalfEdge;
    } else {
        extendable[1] = false;
    }
}

void MorphismPath::merge(MorphismPath* pathB) {
    halfedges[1] = pathB->halfedges[1];
    extendable[1] = pathB->extendable[1];
    indices.insert(indices.end(), pathB->indices.begin(), pathB->indices.end());
}

} // namespace ms 