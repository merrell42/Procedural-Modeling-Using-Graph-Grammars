#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "../third_party/json.h"
#include "../shapes3D/face_type3d.h"
#include "../shapes3D/shape3d.h"
#include "../network/net_graph_map.h"

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

    // Core functionality
    void reset();
    const std::vector<std::vector<Network*>>& getGenerations() const;
    const std::vector<NetTransition*>& getTransitions() const;
    const std::vector<NetTransition*>& getStarterTransitions() const;
    const std::vector<NetTransition*>& getGroundTransitions() const;
    // Network* getEmptyNet() const;
    bool isGrounded() const;
    int getDims() const { return shape->dims; }

    // Transition operations
    Transition getTransition();
    Transition getRemoveTransition();
    Transition getStarterTransition(bool useGround);

    // Generation methods
    // void partialGenerate(const std::vector<Network*>& transitionNets, NetworkSet& networks);

    // Import/Export
    static NetworkHierarchy* import(const Json& json);
    // static NetworkHierarchy* partialImport(const Json& json, const Json& decoration);
    // static Network* glueMatch(const Match& match, const std::vector<Network*>& vertexNetworks,
    //                         const std::vector<std::vector<int>>& connectionOrder);

private:
    std::vector<std::vector<Network*>> generations;
    std::vector<Network*> nodeQueue;
    // std::unordered_map<std::string, EdgeOptions> edgeOptions;
    // std::unordered_map<std::string, GlueOptions> gluingOptions;
    // std::unique_ptr<Matcher> matcher;
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

        EdgeType* getSpliceEdgeType(FaceType3D* faceType, const Vec2& dir);
    };

    struct Match {
        std::vector<int> vertices;
        std::vector<std::array<int, 4>> edges;
    };
};

} // namespace ms 