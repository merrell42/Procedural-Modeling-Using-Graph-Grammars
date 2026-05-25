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
    , grammar(grammar)
    , nodeStats(model)
    , optimizer(nodeStats) {
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
        if (model->numSteps == 0) {
            auto production = grammar->getStarterProduction(true);
            applyProduction(production);
        } else {
            mutate();
        }
        // Short-circuit the optimizer when Beta=0 — `Optimizer::isAccepted`
        // returns true unconditionally in that regime, so computing the cost
        // is dead work. Keeps the bench identical to upstream's pre-optimizer
        // always-accept path when nobody has overridden Beta.
        if (globalSettings["Beta"].get<double>() == 0.0) {
            model->accept();
        } else {
            ms::Optimizer::Cost cost = optimizer.computeCost();
            if (optimizer.isAccepted(cost)) {
                model->accept();
                optimizer.accept(cost);
            } else {
                model->reject();
            }
            nodeStats.resetCostChange();
        }
        model->numSteps++;
    }
    timer->stop("Total");
}

void Mutator::mutate() {
    // Weighted toward normal productions.
    bool adaptive = globalSettings["Adaptive Rule Weights"].get<bool>();
    vector<double> probabilities = adaptive
        ? vector<double>{ successRate[0], successRate[1], successRate[2] * 10 }
        : vector<double>{ 1, 1, 10 };
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

        if (adaptive) {
            constexpr double alpha = 0.1;
            successRate[ruleType] = alpha * (success ? 1.0 : 0.0) + (1.0 - alpha) * successRate[ruleType];
            if (successRate[ruleType] < 0.01) {
                successRate[ruleType] = 0.01;
            }
        }

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
        nodeStats.setReject(1e6);
        return false;
    }
    production.morphism = morphism;

    timer->start("Build Rule Applier");
    int dims = grammar->getDims();
    auto ruleApplier = RuleApplier::build(production, model, dims);
    timer->stop("Build Rule Applier");

    if (!ruleApplier) {
        nodeStats.setReject(1e6);
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
            nodeStats.setReject(0.0);
            delete morphism;
            return true;
        }

        // Free a vertex so that it is allowed to move.
        // This can turn an overconstrained problem in to a solvable one.
        auto vertex = ruleApplier->pickVertexToFree();
        ruleApplier->freeVertex(vertex);
        effort++;
    }

    nodeStats.setReject(1e6);
    ruleApplier->reject();
    delete morphism;
    return false;
}
