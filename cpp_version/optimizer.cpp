#include "optimizer.h"
#include "settings.h"
#include <stdexcept>

// TODO: Use this code. It is not used yet.

namespace ms {

bool Optimizer::detailedCost = false;
constexpr double Optimizer::costScale;  // Definition of static constexpr member

Optimizer::Optimizer(const NodeStats& nodeStats) 
    : nodeStats(nodeStats) {
    prevCost = computeCost();
}

Cost Optimizer::computeCost() {
    Cost cost;

    // Compute line distance from the change
    double prevLogLineDistances = prevCost.logLineDistances;
    cost.logLineDistances = prevLogLineDistances + nodeStats.getCostChange().lineDistance;
    cost.reject = nodeStats.getCostChange().reject;

    int numLines = nodeStats.getCount().line;
    double desiredLines = GlobalSettings::get("Desired Lines");
    double desiredCost = GlobalSettings::get("Desired Lines Cost");
    cost.desiredLines = desiredCost * std::max((desiredLines - numLines) / desiredLines, -1.0);

    double desiredVertexWeight = GlobalSettings::get("Desired Vertex Weight");
    double desirabilitySum = 0;
    
    for (const auto& vertex : nodeStats.getVertices()) {
        if (auto decoration = vertex.getState().getType().getDecoration()) {
            desirabilitySum -= decoration.getDesirability();
        }
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
    double difference = prevCost.sum - cost.sum;
    double prob = std::exp(GlobalSettings::get("Beta") * difference);
    
    if (static_cast<double>(std::rand()) / RAND_MAX < prob) {
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