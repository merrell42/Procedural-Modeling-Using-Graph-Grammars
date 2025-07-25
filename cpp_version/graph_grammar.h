#pragma once
#include <vector>
#include <memory>
#include <map>
#include "third_party/json.h"
#include "primitives/face_type.h"
#include "primitives/primitives.h"
#include "graph_morphism/morphism.h"

using Json = nlohmann::json;

class Graph;  
class ProductionRule;
class Primitives;
class EdgeType;
class VertexType;

// Everything needed to apply a production rule. The start and end
// graphs and morphism to the graph drawing.
struct Production {
    Graph* startGraph;
    Graph* endGraph;
    Morphism* morphism;
    bool ground;
};

// A graph grammar is a set of production rules that are used to modify the graph drawing.
class GraphGrammar {
public:
    GraphGrammar();
    ~GraphGrammar();
    static GraphGrammar* import(const Json& json);

    // Get a production rule.
    Production getProduction();
    Production getRemovalProduction();
    Production getStarterProduction(bool useGround);

    bool isGrounded() const;

    // Get the number of dimensions of the graph.
    int getDims() const { return primitives->dims; }

private:
    // The production rules.
    // Starter rules have an empty start graph.
    vector<ProductionRule*> starterRules;
    // Normal rules have a non-empty start graph.
    vector<ProductionRule*> rules;
    // Ground rules are like starter rules, but just for creating the
    // ground plane on the first iteration.
    vector<ProductionRule*> groundRules;

    Graph* emptyGraph;
    Primitives* primitives;

    // True if there is a ground plane for this model.
    bool grounded;
};

