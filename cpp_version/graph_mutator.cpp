#include "pch.h"
#include "mutator.h"
#include "util/timer.h"
#include "grammar_rules/rule_applier_settings.h"
#include "grammar_rules/rule_applier.h"
#include "settings.h"



GraphMutator::GraphMutator(GraphGrammar* hierarchy, Model* model/*, NodeStats* nodeStats*/)
    : hierarchy(hierarchy)
    , model(model)
    , edgeTypeStarted() {
    
    bool groundEnabled = hierarchy->getGroundRules().size() > 0;
    morphismFinder = make_unique<MorphismFinder>(model, groundEnabled);
}

void GraphMutator::reset() {
    morphismFinder->reset();
}

bool GraphMutator::addStartInstance(bool useGround) {
    auto production = hierarchy->getStarterProduction(useGround);
    return applyProduction(production);
}

bool GraphMutator::changeRandomInstance(bool justDestructible) {
    auto production = justDestructible ?
        hierarchy->getRemoveProduction() :
        hierarchy->getProduction();
    return applyProduction(production);
}

bool GraphMutator::applyProduction(Production production) {
    if (!production.startGraph && !production.endGraph) {
        return false;
    }
    timer->start("Find Morphism");
    auto morphism = morphismFinder->findMorphism(production.startGraph);
    timer->stop("Find Morphism");

    if (!morphism) {
        return false;
    }

    production.morphism = morphism;

    timer->start("Build Normally");

    int dims = hierarchy->getDims();
    auto transistor = RuleApplier::buildNormally(production, model, dims);
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

