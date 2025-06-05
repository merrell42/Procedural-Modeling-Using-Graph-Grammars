#include "pch.h"
#include <iostream>
#include <chrono>
#include <fstream>
#include "mutator.h"
#include "../settings.h"
#include "../util/util.h"

namespace ms {

GuideMutator::GuideMutator(Model* model, GraphMutator* graphMutator)
    : model(model)
    , graphMutator(graphMutator) {}

void GuideMutator::reset() {
    graphMutator->reset();
}

void GuideMutator::iterate(int steps) {
    timer->start("Guide Mutator");

    int finishStep = model->numSteps + steps;
    while (model->numSteps < finishStep) {
        std::cout << model->numSteps << " " << model->getCurrent()->getFaceMap().size() << std::endl;

        if (model->numSteps == 0) {
            mutateGround();
        } else {
            mutate();
        }

        // auto cost = optimizer.computeCost();
        timer->start("Accept Or Reject");
        // if (optimizer.isAccepted(cost)) {
        if (true) {
            accept();
        } else {
            reject();
        }
        timer->stop("Accept Or Reject");

        timer->start("Save");
        timer->stop("Save");

        bool maxTimeEnabled = globalSettings["Max Time Enabled"].get<bool>();
        int maxIterations = globalSettings["Max Iterations"].get<int>();

        model->numSteps++;
    }
    timer->stop("Guide Mutator");
}

// Add the ground plane.
void GuideMutator::mutateGround() {
    bool success = graphMutator->addStartInstance(true);
};

void GuideMutator::mutate() {
    std::vector<double> probabilities = {1, 1, 10};
    bool done = false;

    while (!done) {
        switch(Util::randomDistribution(probabilities)) {
            case 0: {
                timer->start("Add Fragment");
                bool success = graphMutator->addStartInstance(false);
                timer->stop("Add Fragment");
                if (success) {
                    return;
                } else {
                    reject();
                    probabilities[0] = 0;
                }
                break;
            }
            case 1: {
                timer->start("Remove Fragment");
                bool success = graphMutator->changeRandomInstance(true);
                timer->stop("Remove Fragment");
                if (success) {
                    return;
                } else {
                    reject();
                    probabilities[1] = 0;
                }
                break;
            }
            case 2: {
                timer->start("Modify Fragment");
                bool success = graphMutator->changeRandomInstance(false);
                timer->stop("Modify Fragment");
                if (success) {
                    return;
                } else {
                    reject();
                    probabilities[2] = 0;
                }
                break;
            }
        }

        if (globalSettings["Fewer Start Transitions"].get<bool>() && model->numSteps > 1) {
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

void GuideMutator::accept() {
    model->accept();
}

void GuideMutator::reject() {
    model->reject();
}

} // namespace ms 