#include "pch.h"
#include "mutator.h"
#include "util/timer.h"
#include "grammar_rules/rule_applier_settings.h"
#include "grammar_rules/rule_applier.h"
#include "settings.h"

namespace ms {

GraphMutator::GraphMutator(GraphGrammar* hierarchy, Model* model/*, NodeStats* nodeStats*/)
    : hierarchy(hierarchy)
    , model(model)
    , edgeTypeStarted() {
    
    bool groundEnabled = hierarchy->getGroundTransitions().size() > 0;
    mapFinder = std::make_unique<NetGraphMapFinder>(model, groundEnabled);
}

void GraphMutator::reset() {
    mapFinder->reset();
}

bool GraphMutator::addStartInstance(bool useGround) {
    auto transition = hierarchy->getStarterTransition(useGround);
    return applyTransition(transition);
}

bool GraphMutator::changeRandomInstance(bool justDestructible) {
    auto transition = justDestructible ?
        hierarchy->getRemoveTransition() :
        hierarchy->getTransition();
    return applyTransition(transition);
}

bool GraphMutator::applyTransition(Transition transition) {
    if (!transition.startNet && !transition.endNet) {
        return false;
    }
    timer->start("Find Transition Map");
    auto netGraphMap = mapFinder->findMap(transition.startNet);
    timer->stop("Find Transition Map");

    if (!netGraphMap) {
        return false;
    }

    transition.map = netGraphMap;

    timer->start("Build Normally");

    int dims = hierarchy->getDims();
    auto transistor = NetTransistor::buildNormally(transition, model, dims);
    timer->stop("Build Normally");

    if (!transistor) {
        return false;
    }

    int effort = 0;
    int effortLimit = globalSettings["Mutator Effort Limit"].get<int>();
    int verticesToFree = globalSettings["Vertices to Free"].get<int>();

    while (effort < effortLimit) {
        timer->start("Transistor Solve");
        bool success = transistor->solve();
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

} // namespace ms 