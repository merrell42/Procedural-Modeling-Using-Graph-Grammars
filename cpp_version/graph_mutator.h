#pragma once
#include <vector>
#include <memory>
#include "graph_grammar.h"
#include "graph_drawing/model.h"
#include "graph_morphism/morphism_finder.h"

namespace ms {

class Morphism;
struct Production;
class GraphGrammar;

class GraphMutator {
public:
    GraphMutator(GraphGrammar* hierarchy, Model* model);
    ~GraphMutator() = default;

    bool addStartInstance(bool useGround);
    bool changeRandomInstance(bool justDestructible = false);
    void reset();

private:
    std::vector<bool> edgeTypeStarted;
    GraphGrammar* hierarchy;
    Model* model;
    std::unique_ptr<MorphismFinder> morphismFider;

    bool applyProduction(Production production);
};

} // namespace ms 