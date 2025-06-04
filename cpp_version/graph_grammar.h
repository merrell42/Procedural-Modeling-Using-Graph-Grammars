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

class Network;  
class NetTransition;
class Shape3D;
// class Matcher;
class EdgeType;
class VertexType;

struct Transition {
    Network* startNet;
    Network* endNet;
    NetGraphMap* map;
    bool ground;
};

class NetworkHierarchy {
public:
    NetworkHierarchy();
    ~NetworkHierarchy() = default;
    static NetworkHierarchy* import(const Json& json);

    void reset();
    const std::vector<std::vector<Network*>>& getGenerations() const;
    const std::vector<NetTransition*>& getTransitions() const;
    const std::vector<NetTransition*>& getStarterTransitions() const;
    const std::vector<NetTransition*>& getGroundTransitions() const;
    bool isGrounded() const;
    int getDims() const { return shape->dims; }

    Transition getTransition();
    Transition getRemoveTransition();
    Transition getStarterTransition(bool useGround);

private:
    std::vector<std::vector<Network*>> generations;
    std::vector<Network*> nodeQueue;
    Network* emptyNet;

    Shape3D* shape;
    std::vector<NetTransition*> transitions;
    std::vector<NetTransition*> starterTransitions;
    std::vector<NetTransition*> groundTransitions;
    bool grounded;

    struct NetworkSet {
        std::vector<Network*> face;
        std::vector<Network*> edge;
        std::vector<Network*> vertex;
        std::vector<EdgeType*> edgeTypes;
        std::unordered_map<std::string, EdgeType*> splicedEdgeTypes;
    };

    struct Match {
        std::vector<int> vertices;
        std::vector<std::array<int, 4>> edges;
    };
};

} // namespace ms 