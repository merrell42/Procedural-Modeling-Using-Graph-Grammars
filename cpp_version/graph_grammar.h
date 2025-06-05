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
class NetTransition;
class Shape3D;
class EdgeType;
class VertexType;

struct Transition {
    Graph* startNet;
    Graph* endNet;
    NetGraphMap* map;
    bool ground;
};

class GraphGrammar {
public:
    GraphGrammar();
    ~GraphGrammar() = default;
    static GraphGrammar* import(const Json& json);

    void reset();
    const std::vector<std::vector<Graph*>>& getGenerations() const;
    const std::vector<NetTransition*>& getTransitions() const;
    const std::vector<NetTransition*>& getStarterTransitions() const;
    const std::vector<NetTransition*>& getGroundTransitions() const;
    bool isGrounded() const;
    int getDims() const { return shape->dims; }

    Transition getTransition();
    Transition getRemoveTransition();
    Transition getStarterTransition(bool useGround);

private:
    std::vector<std::vector<Graph*>> generations;
    std::vector<Graph*> nodeQueue;
    Graph* emptyNet;

    Shape3D* shape;
    std::vector<NetTransition*> transitions;
    std::vector<NetTransition*> starterTransitions;
    std::vector<NetTransition*> groundTransitions;
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