#include "pch.h"
#include "node_stats.h"
#include "graph_drawing/model.h"
#include "graph_drawing/graph_drawing.h"
#include "graph_drawing/edge.h"
#include "graph_drawing/half_edge.h"
#include "graph_drawing/vertex.h"
#include <cmath>

namespace ms {

NodeStats::NodeStats(Model* model) : model(model) {}

// Accumulate log(1 + |delta|) over every edge present in both `current` and
// `prev`. Edges added during this step contribute 0 (no prev length); edges
// removed contribute 0 (no current length). The log keeps very large length
// jumps from dominating the cost.
// This function is currently not used. It was used in the optimizer in the
// web version of the code. However, I'm reluctant to put it back in because
// I don't think it's really necessary.
void NodeStats::computeLineDistanceChange() {
    costChange.lineDistance = 0.0;
    auto* current = model->getCurrent();
    auto* prev = model->getPrev();

    const auto& currentEdges = current->getEdgeMap();
    const auto& prevEdges = prev->getEdgeMap();
    const auto& prevVertices = prev->getVertexMap();

    for (const auto& [id, edge] : currentEdges) {
        auto pit = prevEdges.find(id);
        if (pit == prevEdges.end()) continue;

        auto currentHEs = edge->getHalfEdges();
        if (currentHEs.size() < 2 || !currentHEs[0] || !currentHEs[1]) continue;
        double currentLen =
            (currentHEs[0]->getPosition() - currentHEs[1]->getPosition()).length();

        auto prevHEs = pit->second->getHalfEdges();
        if (prevHEs.size() < 2 || !prevHEs[0] || !prevHEs[1]) continue;
        // HalfEdge stores its vertex by id; resolve through the prev-drawing's
        // vertex map so we read positions as they were before this step's
        // mutation (current's vertex positions may have moved).
        auto vit0 = prevVertices.find(prevHEs[0]->getVertex()->getId());
        auto vit1 = prevVertices.find(prevHEs[1]->getVertex()->getId());
        if (vit0 == prevVertices.end() || vit1 == prevVertices.end()) continue;

        double prevLen =
            (vit0->second->getPosition() - vit1->second->getPosition()).length();
        costChange.lineDistance += std::log(1.0 + std::abs(currentLen - prevLen));
    }
}

void NodeStats::setReject(double penalty) {
    costChange.reject = penalty;
}

int NodeStats::getLineCount() const {
    return static_cast<int>(model->getCurrent()->getEdgeMap().size());
}

std::vector<Vertex*> NodeStats::getVertices() const {
    std::vector<Vertex*> result;
    for (auto& [id, vertex] : model->getCurrent()->getVertexMap()) {
        result.push_back(vertex);
    }
    return result;
}

void NodeStats::resetCostChange() {
    costChange = {};
}

} // namespace ms
