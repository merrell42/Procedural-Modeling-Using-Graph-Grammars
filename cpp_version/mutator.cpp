#include "pch.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include "mutator.h"
#include "../settings.h"
#include "../util/util.h"
#include "../grammar_rules/rule_applier_settings.h"
#include "../grammar_rules/rule_applier.h"

Mutator::Mutator(Model* model, GraphGrammar* grammar)
    : model(model)
    , grammar(grammar) {
    bool groundEnabled = grammar->isGrounded();
    morphismFinder = make_unique<MorphismFinder>(model, groundEnabled);
}

void Mutator::reset() {
    morphismFinder->reset();
}

void Mutator::iterate(int steps) {
    timer->start("Total");

    int finishStep = model->numSteps + steps;
    while (model->numSteps < finishStep) {
        // cout << model->numSteps << " " << model->getCurrent()->getFaceMap().size() << endl;

        if (model->numSteps == 0) {
            mutateGround();
        } else {
            mutate();
        }

        // auto cost = optimizer.computeCost();
        // if (optimizer.isAccepted(cost)) {
        if (true) {
            model->accept();
        } else {
            model->reject();
        }
        model->numSteps++;
    }
    timer->stop("Total");
}

// Add the ground plane.
void Mutator::mutateGround() {
    bool success = addStartInstance(true);
};

void Mutator::mutate() {
    vector<double> probabilities = {1, 1, 10};
    bool done = false;

    while (!done) {
        switch(Util::randomDistribution(probabilities)) {
            case 0: {
                timer->start("Add Graph");
                bool success = addStartInstance(false);
                timer->stop("Add Graph");
                if (success) {
                    return;
                } else {
                    model->reject();
                    probabilities[0] = 0;
                }
                break;
            }
            case 1: {
                timer->start("Remove Graph");
                bool success = changeRandomInstance(true);
                timer->stop("Remove Graph");
                if (success) {
                    return;
                } else {
                    model->reject();
                    probabilities[1] = 0;
                }
                break;
            }
            case 2: {
                timer->start("Modify Graph");
                bool success = changeRandomInstance(false);
                timer->stop("Modify Graph");
                if (success) {
                    return;
                } else {
                    model->reject();
                    probabilities[2] = 0;
                }
                break;
            }
        }

        if (globalSettings["Fewer Start Productions"].get<bool>() && model->numSteps > 1) {
            return;
        }

        done = true;
        for (double probability : probabilities) {
            if (probability > 0) {
                done = false;
                break;
            }
        }
    }
}

bool Mutator::addStartInstance(bool useGround) {
    auto production = grammar->getStarterProduction(useGround);
    return applyProduction(production);
}

bool Mutator::changeRandomInstance(bool justDestructible) {
    auto production = justDestructible ?
        grammar->getRemoveProduction() :
        grammar->getProduction();
    return applyProduction(production);
}

bool Mutator::applyProduction(Production production) {
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
    int dims = grammar->getDims();
    auto ruleApplier = RuleApplier::build(production, model, dims);
    timer->stop("Build Normally");

    if (!ruleApplier) {
        delete morphism;
        return false;
    }

    int effort = 0;
    int effortLimit = globalSettings["Mutator Effort Limit"].get<int>();

    while (effort < effortLimit) {
        timer->start("RuleApplier Solve");
        bool success = ruleApplier->solve();
        timer->stop("RuleApplier Solve");

        if (success) {
            delete morphism;
            return true;
        }

        auto vertex = ruleApplier->pickVertexToFree();
        ruleApplier->freeVertex(vertex);
        effort++;
    }

    ruleApplier->reject();
    delete morphism;
    return false;
}
