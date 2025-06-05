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
class Primitives;
class EdgeType;
class VertexType;

struct Transition {
    Graph* startNet;
    Graph* endNet;
    Morphism* map;
    bool ground;
};

class GraphGrammar {
public:
    GraphGrammar();
    ~GraphGrammar() = default;
    static GraphGrammar* import(const Json& json);

    void reset();
    const vector<vector<Graph*>>& getGenerations() const;
    const vector<NetTransition*>& getTransitions() const;
    const vector<NetTransition*>& getStarterTransitions() const;
    const vector<NetTransition*>& getGroundTransitions() const;
    bool isGrounded() const;
    int getDims() const { return shape->dims; }

    Transition getTransition();
    Transition getRemoveTransition();
    Transition getStarterTransition(bool useGround);

private:
    vector<vector<Graph*>> generations;
    vector<Graph*> nodeQueue;
    Graph* emptyNet;

    Primitives* shape;
    vector<NetTransition*> transitions;
    vector<NetTransition*> starterTransitions;
    vector<NetTransition*> groundTransitions;
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

} // namespace ms 