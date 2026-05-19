#pragma once
#include "settings.h"
#include "node_stats.h"
#include <cmath>

namespace ms {
class Optimizer {
public:
    explicit Optimizer(const NodeStats& nodeStats);
    
    struct Cost {
        double logLineDistances = 0;
        double reject = 0;
        double desiredLines = 0;
        double desiredVertex = 0;
        double sum = 0;
    };

    Cost computeCost();
    void accept(const Cost& cost);
    bool isAccepted(const Cost& cost);
    const Cost& getPrevCost() const;
    void verifyCost();

    static bool detailedCost;
    static constexpr double costScale = 0.01;

private:
    const NodeStats& nodeStats;
    Cost prevCost;
};

} // namespace ms 