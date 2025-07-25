#include "pch.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include "mutator.h"
#include "settings.h"
#include "util/util.h"
#include "grammar_rules/rule_applier_settings.h"
#include "grammar_rules/rule_applier.h"

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
            auto production = grammar->getStarterProduction(true);
            applyProduction(production);
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

void Mutator::mutate() {
    // The probability of picking a particular production rule.
    // This is weighted towards normal production rules instead of starter and removal rules.
    vector<double> probabilities = {1, 1, 10};
    bool done = false;

    while (!done) {
        auto ruleType = Util::randomDistribution(probabilities);
        Production production;
        switch(ruleType) {
            case 0:
                // Creates a new graph from nothing.
                production = grammar->getStarterProduction(false);
                break;
            case 1:
                // Removes an existing graph to nothing.
                production = grammar->getRemovalProduction();
                break;
            case 2:
                // Use normal rule that modify graphs.
                production = grammar->getProduction();
                break;
        }
        bool success = applyProduction(production);
        if (success) {
            return;
        } else {
            // If unsuccessful, try again with a different rule type.
            model->reject();
            probabilities[ruleType] = 0;
        }

        if (globalSettings["Fewer Start Productions"].get<bool>() && model->numSteps > 1) {
            return;
        }

        // If all rule types have been eliminated return unsuccessfully.
        done = true;
        for (double probability : probabilities) {
            if (probability > 0) {
                done = false;
                break;
            }
        }
    }
}

// Apply a production rule. Return true if successful.
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

    timer->start("Build Rule Applier");
    int dims = grammar->getDims();
    auto ruleApplier = RuleApplier::build(production, model, dims);
    timer->stop("Build Rule Applier");

    if (!ruleApplier) {
        delete morphism;
        return false;
    }

    // Try to apply the rule by sampling different solutions until effort limit is reached.
    int effort = 0;
    int effortLimit = globalSettings["Mutator Effort Limit"].get<int>();
    while (effort < effortLimit) {
        // This repeatedly tries solutions with the current configuration.
        timer->start("RuleApplier Solve");
        bool success = ruleApplier->solve();
        timer->stop("RuleApplier Solve");

        if (success) {
            delete morphism;
            return true;
        }

        // Free a vertex so that it is allowed to move.
        // This can turn an overconstrained problem in to a solvable one.
        auto vertex = ruleApplier->pickVertexToFree();
        ruleApplier->freeVertex(vertex);
        effort++;
    }

    ruleApplier->reject();
    delete morphism;
    return false;
}
