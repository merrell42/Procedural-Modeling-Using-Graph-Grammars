#pragma once
#include <vector>
#include <memory>
#include "../hierarchy/network_hierarchy.h"
#include "../graph_drawing/model.h"
#include "../network/net_graph_map_finder.h"

namespace ms {

// class NetGraphMapFinder;
// class Classifier;
// class NodeStats;
class NetGraphMap;
// class MutationArea;
struct Transition;
class NetworkHierarchy;

class NetworkMutator {
public:
    NetworkMutator(NetworkHierarchy* hierarchy, Model* model /*, NodeStats* nodeStats*/);
    ~NetworkMutator() = default;

    // Core functionality
    // void setMutationArea(MutationArea* area);
    bool addStartInstance(bool useGround);
    bool changeRandomInstance(bool justDestructible = false);
    void reset();

    // Static members
    static constexpr int LINES_TO_CHOOSE = 10;

private:
    std::vector<bool> edgeTypeStarted;
    NetworkHierarchy* hierarchy;
    Model* model;
    // MutationArea* mutationArea;
    std::unique_ptr<NetGraphMapFinder> mapFinder;

    // Helper methods
    bool applyTransition(Transition transition);
    // bool findAndApplyTransition(Transition* transition);
};

//class NetGraphMapFinder {
//public:
//    NetGraphMapFinder(NodeStats* nodeStats, bool groundEnabled);
//
//    NetGraphMap* findMap(NetGraph* netGraph);
//    void addStartInstance();
//
//private:
//    NodeStats* nodeStats;
//    bool groundEnabled;
//    std::vector<NetGraphMap*> maps;
//
//    // Helper methods
//    NetGraphMap* createMap(NetGraph* netGraph);
//    void destroyMaps();
//};

} // namespace ms 