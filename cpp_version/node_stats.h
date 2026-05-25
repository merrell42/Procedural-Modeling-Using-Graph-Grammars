#pragma once
#include <vector>

class Model;
class Vertex;

namespace ms {

// This class should probably be renamed and possibly moved into the optimizer.
// The web version of the code has the concept of a node (in ms.node), but this
// is not present in the C++ version.
class NodeStats {
public:
    struct CostChange {
        double lineDistance = 0.0;
        double reject = 0.0;
    };

    explicit NodeStats(Model* model);

    // Computes accumulated log-distance change between current and previous drawing.
    // Currently not used.
    void computeLineDistanceChange();

    // Call when a production structurally fails (no morphism / solve failed).
    void setReject(double penalty = 1e6);

    const CostChange& getCostChange() const { return costChange; }

    // Number of edges in the current drawing.
    int getLineCount() const;

    // All vertices in the current drawing.
    std::vector<Vertex*> getVertices() const;

    // Reset cost change after optimizer accept/reject decision.
    void resetCostChange();

private:
    Model* model;
    CostChange costChange;
};

} // namespace ms
