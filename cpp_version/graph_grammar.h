#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "../third_party/json.h"
#include "../primitives/face_type.h"
#include "../primitives/primitives.h"
#include "../graph_morphism/morphism.h"

using Json = nlohmann::json;

class Graph;  
class ProductionRule;
class Primitives;
class EdgeType;
class VertexType;

struct Production {
    Graph* startGraph;
    Graph* endGraph;
    Morphism* morphism;
    bool ground;
};

class GraphGrammar {
public:
    GraphGrammar();
    ~GraphGrammar() = default;
    static GraphGrammar* import(const Json& json);

    void reset();
    const vector<ProductionRule*>& getGroundRules() const;
    int getDims() const { return shape->dims; }

    Production getProduction();
    Production getRemoveProduction();
    Production getStarterProduction(bool useGround);

private:
    vector<vector<Graph*>> generations;
    vector<Graph*> nodeQueue;
    Graph* emptyGraph;

    Primitives* shape;
    vector<ProductionRule*> rules;
    vector<ProductionRule*> starterRules;
    vector<ProductionRule*> groundRules;
    bool grounded;

    struct GraphSet {
        vector<Graph*> face;
        vector<Graph*> edge;
        vector<Graph*> vertex;
        vector<EdgeType*> edgeTypes;
        unordered_map<string, EdgeType*> splicedEdgeTypes;
    };

    struct Match {
        vector<int> vertices;
        vector<array<int, 4>> edges;
    };
};

