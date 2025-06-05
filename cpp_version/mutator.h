#pragma once
#include <functional>
#include <memory>
#include <string>
#include "graph_mutator.h"
#include "../util/timer.h"
#include "../graph_drawing/model.h"

namespace ms {

class Classifier;
class Timer;
class Model;
class MutationArea;
class GraphMutator;

class Mutator {
public:
    Mutator(Model* model, GraphMutator* graphMutator);
    ~Mutator() = default;

    void iterate(int step);
    void mutate();
    void mutateGround();
    void accept();
    void reject();
    void reset();

    // Static members
    static int taskCount;
    static constexpr int findMutableVertexAttempts = 20;
    static constexpr int findMutableEdgeAttempts = 20;

private:
    Model* model;
    std::unique_ptr<GraphMutator> graphMutator;
};

} // namespace ms 