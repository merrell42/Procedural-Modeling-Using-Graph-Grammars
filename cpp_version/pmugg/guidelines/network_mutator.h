#pragma once
#include <vector>
#include <memory>
#include "../hierarchy/network_hierarchy.h"

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
    NetworkMutator(NetworkHierarchy* hierarchy/*, NodeStats* nodeStats*/);
    ~NetworkMutator() = default;

    // Core functionality
    // void setMutationArea(MutationArea* area);
    bool addStartInstance();
    bool changeRandomInstance(bool justDestructible = false);

    // Static members
    static constexpr int LINES_TO_CHOOSE = 10;

private:
    std::vector<bool> edgeTypeStarted;
    NetworkHierarchy* hierarchy;
    // MutationArea* mutationArea;
    // std::unique_ptr<NetGraphMapFinder> mapFinder;

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