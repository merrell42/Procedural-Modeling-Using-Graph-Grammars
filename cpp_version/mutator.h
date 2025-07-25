#pragma once
#include <functional>
#include <memory>
#include <string>
#include "graph_grammar.h"
#include "util/timer.h"
#include "graph_drawing/model.h"
#include "graph_morphism/morphism_finder.h"

class Classifier;
class Timer;
class Model;
class MutationArea;
class Morphism;
class GraphGrammar;

class Mutator {
public:
    Mutator(Model* model, GraphGrammar* grammar);
    ~Mutator() = default;

    void iterate(int steps);
    void mutate();
    void reset();

private:
    Model* model;
    GraphGrammar* grammar;
    unique_ptr<MorphismFinder> morphismFinder;

    bool applyProduction(Production production);
};

