#include "pch.h"
#include "optimizer.h"
#ifdef _CONSOLE
#include "minmax.h"
#endif
#include "settings.h"
#include "graph_drawing/vertex.h"
#include "primitives/vertex_type.h"
#include "util/util.h"
#include <stdexcept>

namespace ms {

bool Optimizer::detailedCost = false;
constexpr double Optimizer::costScale;

Optimizer::Optimizer(const NodeStats& nodeStats)
    : nodeStats(nodeStats) {
    prevCost = computeCost();
}

Optimizer::Cost Optimizer::computeCost() {
    Cost cost;
    cost.logLineDistances =
        prevCost.logLineDistances + nodeStats.getCostChange().lineDistance;
    cost.reject = nodeStats.getCostChange().reject;

    int numLines = nodeStats.getLineCount();
    double desiredLines = globalSettings["Desired Lines"].get<double>();
    double desiredCost = globalSettings["Desired Lines Cost"].get<double>();
    cost.desiredLines =
        desiredCost * max((desiredLines - numLines) / desiredLines, -1.0);

    double desiredVertexWeight = globalSettings["Desired Vertex Weight"].get<double>();
    double desirabilitySum = 0.0;
    for (auto* vertex : nodeStats.getVertices()) {
        desirabilitySum -= vertex->getType()->desirability;
    }
    cost.desiredVertex = desiredVertexWeight * desirabilitySum;

    cost.sum = costScale * (cost.logLineDistances + cost.reject +
                            cost.desiredLines + cost.desiredVertex);
    return cost;
}

void Optimizer::accept(const Cost& cost) {
    prevCost = cost;
}

bool Optimizer::isAccepted(const Cost& cost) {
    double beta = globalSettings["Beta"].get<double>();
    // Short-circuit at Beta=0: acceptance probability is exp(0)=1 so the
    // draw is meaningless. Skipping `randomValue()` here keeps the RNG
    // state lock-step with the pre-optimizer always-accept path, so older
    // grammars that haven't been tuned for the optimizer stay bit-identical.
    if (beta == 0.0) {
        prevCost = cost;
        return true;
    }
    double difference = prevCost.sum - cost.sum;
    double prob = std::exp(beta * difference);
    if (randomValue() < prob) {
        prevCost = cost;
        return true;
    }
    return false;
}

const Optimizer::Cost& Optimizer::getPrevCost() const {
    return prevCost;
}

void Optimizer::verifyCost() {
    double costChange = computeCost().sum - prevCost.sum;
    if (std::abs(costChange) > 1e-10) {
        throw std::runtime_error("Model was not restored correctly.");
    }
}

} // namespace ms 