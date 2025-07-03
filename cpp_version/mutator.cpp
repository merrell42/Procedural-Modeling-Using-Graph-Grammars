#include "pch.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include "mutator.h"
#include "../settings.h"
#include "../util/util.h"

Mutator::Mutator(Model* model, GraphMutator* graphMutator)
    : model(model)
    , graphMutator(graphMutator) {}

void Mutator::reset() {
    graphMutator->reset();
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
            accept();
        } else {
            reject();
        }

        bool maxTimeEnabled = globalSettings["Max Time Enabled"].get<bool>();
        int maxIterations = globalSettings["Max Iterations"].get<int>();

        model->numSteps++;
    }
    timer->stop("Total");
}

// Add the ground plane.
void Mutator::mutateGround() {
    bool success = graphMutator->addStartInstance(true);
};

void Mutator::mutate() {
    vector<double> probabilities = {1, 1, 10};
    bool done = false;

    while (!done) {
        switch(Util::randomDistribution(probabilities)) {
            case 0: {
                timer->start("Add Graph");
                bool success = graphMutator->addStartInstance(false);
                timer->stop("Add Graph");
                if (success) {
                    return;
                } else {
                    reject();
                    probabilities[0] = 0;
                }
                break;
            }
            case 1: {
                timer->start("Remove Graph");
                bool success = graphMutator->changeRandomInstance(true);
                timer->stop("Remove Graph");
                if (success) {
                    return;
                } else {
                    reject();
                    probabilities[1] = 0;
                }
                break;
            }
            case 2: {
                timer->start("Modify Graph");
                bool success = graphMutator->changeRandomInstance(false);
                timer->stop("Modify Graph");
                if (success) {
                    return;
                } else {
                    reject();
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

void Mutator::accept() {
    model->accept();
}

void Mutator::reject() {
    model->reject();
}

