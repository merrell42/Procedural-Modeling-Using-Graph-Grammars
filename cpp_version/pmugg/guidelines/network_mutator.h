#pragma once
#include "hierarchy_mutator.h"
#include <vector>
#include <memory>

namespace ms {

class Classifier;
class NodeStats;
class NetGraphMap;
class MutationArea;
class Transition;

class NetworkMutator : public HierarchyMutator {
public:
    NetworkMutator(Classifier* classifier, NodeStats* nodeStats);
    ~NetworkMutator() override = default;

    // Core functionality
    void setMutationArea(MutationArea* area) override;
    bool addStartInstance() override;
    bool changeRandomInstance(bool justDestructible = false) override;

    // Static members
    static constexpr int LINES_TO_CHOOSE = 10;

private:
    std::vector<bool> edgeTypeStarted;
    MutationArea* mutationArea;
    std::unique_ptr<NetGraphMapFinder> mapFinder;

    // Helper methods
    bool applyTransition(Transition* transition);
    bool findAndApplyTransition(Transition* transition);
};

class NetGraphMapFinder {
public:
    NetGraphMapFinder(NodeStats* nodeStats, bool groundEnabled);

    NetGraphMap* findMap(NetGraph* netGraph);
    void addStartInstance();

private:
    NodeStats* nodeStats;
    bool groundEnabled;
    std::vector<NetGraphMap*> maps;

    // Helper methods
    NetGraphMap* createMap(NetGraph* netGraph);
    void destroyMaps();
};

} // namespace ms 