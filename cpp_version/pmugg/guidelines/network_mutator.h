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
    NetworkMutator(NetworkHierarchy* hierarchy, Model* model);
    ~NetworkMutator() = default;

    bool addStartInstance(bool useGround);
    bool changeRandomInstance(bool justDestructible = false);
    void reset();

private:
    std::vector<bool> edgeTypeStarted;
    NetworkHierarchy* hierarchy;
    Model* model;
    std::unique_ptr<NetGraphMapFinder> mapFinder;

    bool applyTransition(Transition transition);
};

} // namespace ms 