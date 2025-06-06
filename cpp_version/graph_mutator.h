#pragma once
#include <vector>
#include <memory>
#include "graph_grammar.h"
#include "graph_drawing/model.h"
#include "graph_morphism/morphism_finder.h"

class Morphism;
class GraphGrammar;

class GraphMutator {
public:
    GraphMutator(GraphGrammar* hierarchy, Model* model);
    ~GraphMutator() = default;

    bool addStartInstance(bool useGround);
    bool changeRandomInstance(bool justDestructible = false);
    void reset();

private:
    vector<bool> edgeTypeStarted;
    GraphGrammar* hierarchy;
    Model* model;
    unique_ptr<MorphismFinder> morphismFinder;

    bool applyProduction(Production production);
};

