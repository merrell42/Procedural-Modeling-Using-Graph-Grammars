#include "network_mutator.h"
#include "classifier.h"
#include "node_stats.h"
#include "net_graph_map.h"
#include "mutation_area.h"
#include "transition.h"
#include "net_transistor.h"
#include "timer.h"
#include "settings.h"
#include "util.h"
#include <algorithm>

namespace ms {

NetworkMutator::NetworkMutator(Classifier* classifier, NodeStats* nodeStats)
    : HierarchyMutator(classifier, nodeStats)
    , edgeTypeStarted()
    , mutationArea(nullptr) {
    
    bool groundEnabled = classifier->getHierarchy()->getGroundTransitions().size() > 0;
    mapFinder = std::make_unique<NetGraphMapFinder>(nodeStats, groundEnabled);
}

void NetworkMutator::setMutationArea(MutationArea* area) {
    mutationArea = area;
}

bool NetworkMutator::addStartInstance() {
    auto transition = getHierarchy()->getStarterTransition();
    return applyTransition(transition);
}

bool NetworkMutator::changeRandomInstance(bool justDestructible) {
    auto transition = justDestructible ?
        getHierarchy()->getRemoveTransition() :
        getHierarchy()->getTransition();
    return applyTransition(transition);
}

bool NetworkMutator::applyTransition(Transition* transition) {
    timer->start("Find Transition Map");
    auto netGraphMap = transition ? mapFinder->findMap(transition->getStartNet()) : nullptr;
    timer->stop("Find Transition Map");

    if (!netGraphMap) {
        return false;
    }

    transition->setMap(netGraphMap);

    timer->start("Build Normally");
    auto transistor = NetTransistor::buildNormally(transition, getNodeStats(), is3D());
    timer->stop("Build Normally");

    if (!transistor) {
        getNodeStats()->setCostChange("reject", 1e6);
        return false;
    }

    taskDebug();
    int effort = 0;
    int effortLimit = globalSettings.getInt("Mutator Effort Limit");
    int verticesToFree = globalSettings.getInt("Vertices to Free");

    while (effort < effortLimit) {
        timer->start("Transistor Solve");
        bool success = transistor->solve(mutationArea);
        timer->stop("Transistor Solve");

        if (success) {
            return true;
        }

        for (int i = 0; i < verticesToFree; i++) {
            transistor->freeVertex();
        }
        effort++;
    }

    transistor->reject();
    return false;
}

NetGraphMapFinder::NetGraphMapFinder(NodeStats* stats, bool ground)
    : nodeStats(stats)
    , groundEnabled(ground)
    , maps() {
}

NetGraphMap* NetGraphMapFinder::findMap(NetGraph* netGraph) {
    // Try to find an existing compatible map
    for (auto* map : maps) {
        if (map->isCompatible(netGraph)) {
            return map;
        }
    }

    // Create a new map if none found
    auto* newMap = createMap(netGraph);
    if (newMap) {
        maps.push_back(newMap);
    }
    return newMap;
}

void NetGraphMapFinder::addStartInstance() {
    destroyMaps();
}

NetGraphMap* NetGraphMapFinder::createMap(NetGraph* netGraph) {
    std::vector<Line*> lines;
    auto nodes = nodeStats->getNodes("line");

    // Choose random lines to consider
    for (int i = 0; i < NetworkMutator::LINES_TO_CHOOSE && !nodes.empty(); i++) {
        int index = Util::random(nodes.size());
        if (auto* line = dynamic_cast<Line*>(nodes[index]->getElement())) {
            lines.push_back(line);
        }
        nodes.erase(nodes.begin() + index);
    }

    // Sort lines by length to prefer shorter ones
    std::sort(lines.begin(), lines.end(),
        [](Line* a, Line* b) {
            return a->getLength() < b->getLength();
        });

    // Try to create a map with each line
    for (auto* line : lines) {
        auto* map = new NetGraphMap(netGraph, line, groundEnabled);
        if (map->isValid()) {
            return map;
        }
        delete map;
    }

    return nullptr;
}

void NetGraphMapFinder::destroyMaps() {
    for (auto* map : maps) {
        delete map;
    }
    maps.clear();
}

} // namespace ms 