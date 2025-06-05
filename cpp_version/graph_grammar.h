#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "../third_party/json.h"
#include "../primitives/face_type.h"
#include "../primitives/primitives.h"
#include "../graph_morphism/morphism.h"

using Json = nlohmann::json;

namespace ms {

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
    const std::vector<std::vector<Graph*>>& getGenerations() const;
    const std::vector<ProductionRule*>& getProductions() const;
    const std::vector<ProductionRule*>& getStarterProductions() const;
    const std::vector<ProductionRule*>& getGroundProductions() const;
    bool isGrounded() const;
    int getDims() const { return shape->dims; }

    Production getProduction();
    Production getRemoveProduction();
    Production getStarterProduction(bool useGround);

private:
    std::vector<std::vector<Graph*>> generations;
    std::vector<Graph*> nodeQueue;
    Graph* emptyGraph;

    Primitives* shape;
    std::vector<ProductionRule*> productions;
    std::vector<ProductionRule*> starterProductions;
    std::vector<ProductionRule*> groundProductions;
    bool grounded;

    struct GraphSet {
        std::vector<Graph*> face;
        std::vector<Graph*> edge;
        std::vector<Graph*> vertex;
        std::vector<EdgeType*> edgeTypes;
        std::unordered_map<std::string, EdgeType*> splicedEdgeTypes;
    };

    struct Match {
        std::vector<int> vertices;
        std::vector<std::array<int, 4>> edges;
    };
};

} // namespace ms 