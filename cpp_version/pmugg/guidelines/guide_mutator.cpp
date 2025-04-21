#include "pch.h"
#include "guide_mutator.h"
// #include "classifier.h"
// #include "timer.h"
// #include "model.h"
// #include "mutation_area.h"
// #include "hierarchy_mutator.h"
// #include "network_mutator.h"
// #include "family_tree_mutator.h"
#include "../grid/settings.h"
#include "../util/util.h"
#include <iostream>
#include <chrono>

namespace ms {

// bool GuideMutator::forceContinued = false;

GuideMutator::GuideMutator(Model* model, NetworkMutator* networkMutator)
    : // classifier(classifier)
    // , notify(notifyFunc)
    // , tasks()
    //, status(Status::SUCCESS)
    //, breakTime(0)
    //, earlyEndTime(0)
    //, endTime(0)
    //, seedCount(0)
    //, pauseCount(-1)
    // , nodeStats()
    // , optimizer(&nodeStats)
    model(model)
    , networkMutator(networkMutator) {
    
    // initializeHierarchyMutator();
    // ms::globalMutator = this;  // For debugging
}

//void GuideMutator::seed(MutationArea* mutationArea) {
//    hierarchyMutator->setMutationArea(mutationArea);
//    seedCount = 0;
//}

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

        auto now = std::chrono::system_clock::now();
        auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();

        bool maxTimeEnabled = globalSettings["Max Time Enabled"].get<bool>();
        int maxIterations = globalSettings["Max Iterations"].get<int>();

        // model->getCurrent()->save(std::to_string(model->numSteps));
        model->numSteps++;
    }
    timer->stop("Guide Mutator");
    // model->getCurrent()->save("");
}

// Add the ground plane.
void GuideMutator::mutateGround() {
    bool success = networkMutator->addStartInstance(true);
};

void GuideMutator::mutate() {
    std::vector<double> probabilities = {1, 1, 10};
    bool done = false;

    while (!done) {
        switch(Util::randomDistribution(probabilities)) {
            case 0: {
                timer->start("Add Fragment");
                bool success = networkMutator->addStartInstance(false);
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
                bool success = networkMutator->changeRandomInstance(true);
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
                bool success = networkMutator->changeRandomInstance(false);
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

        /*if (nodeStats.getCostChange("reject") > 0) {
            return;
        }*/

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
    /*nodeStats.restore();
    if (closeInspection(model->numSteps) || globalSettings.getBool("Debug Alerts")) {
        optimizer.verifyCost();
    }
    if (closeInspection(model->numSteps)) {
        std::cout << model->numSteps << " rejected." << std::endl;
    }*/
}

//void GuideMutator::save(Model* model) {
//    auto cost = optimizer.computeCost();
//    optimizer.accept(cost);
//    nodeStats.save();
//}

//bool GuideMutator::initializeModel(Model* model) {
//    model->setNodeStats(&nodeStats);
//    auto boundaryGroups = classifier->getBoundaryGroups();
//    
//    if (!boundaryGroups.empty()) {
//        auto emptyRing = classifier->getEmptyRing();
//        auto ringInstance = std::make_unique<RingInstance>(emptyRing, &nodeStats);
//        auto ringGroup = RingGroup::createFromRing(ringInstance->getRing());
//        auto startInstance = std::make_unique<RingGroupInstance>(ringGroup);
//        startInstance->addRingInstance(0, ringInstance.get());
//        
//        Transition transition{
//            startInstance.get(),
//            Util::pick(boundaryGroups),
//            true  // initialBoundary
//        };
//        
//        return hierarchyMutator->applyTransition(transition);
//    }
//    
//    return true;
//}

//void GuideMutator::reset() {
//    status = Status::UNRESOLVED;
//}
//
//void GuideMutator::pause() {
//    displayStats();
//    notify(true);
//    status = Status::PAUSED;
//}
//
//void GuideMutator::unresolve() {
//    status = Status::UNRESOLVED;
//}
//
//void GuideMutator::step(int numTasks) {
//    displayStats();
//    pauseCount = model->numSteps + numTasks;
//}

//void GuideMutator::setBreakTime(double newBreakTime) {
//    breakTime = newBreakTime;
//}
//
//void GuideMutator::setEndTime(double newEarlyEndTime, double newEndTime) {
//    earlyEndTime = newEarlyEndTime;
//    endTime = newEndTime;
//}

//Status GuideMutator::getStatus() const {
//    return status;
//}

//void GuideMutator::displayStats() {
//    std::string stats = "Iteration: " + std::to_string(model->numSteps) + " / " + 
//                       std::to_string(globalSettings.getInt("Max Iterations"));
//    
//    if (!globalSettings.getBool("MVP")) {
//        stats += " " + std::to_string(optimizer.getPrevCost().sum);
//    }
//    
//    // Update statistics display (implementation depends on UI system)
//    // document.getElementById('statistics').innerHTML = stats;
//}
//
//std::string GuideMutator::report() const {
//    auto cost = optimizer.getPrevCost();
//    std::string summary = std::to_string(cost.sum) + " " + 
//                         std::to_string(timer->getTiming("everything") / 1000.0) + "s " +
//                         std::to_string(model->numSteps / timer->getTiming("everything"));
//    
//    std::string details = cost.toString() + "\n" + timer->reportMean();
//    std::cout << "# of vertices: " << nodeStats.getCount("vertex") << std::endl;
//    
//    return summary + "\n" + details;
//}

//NodeStats* GuideMutator::getNodeStats() {
//    return &nodeStats;
//}
//
//Classifier* GuideMutator::getClassifier() const {
//    return classifier;
//}
//
//bool GuideMutator::closeInspection(int taskCount) {
//    if (forceContinued) {
//        return false;
//    }
//    
//    int taskStop = globalSettings.getInt("Task Stop");
//    int taskStep = globalSettings.getInt("Task Step");
//    
//    if (taskStop >= 0) {
//        return model->numSteps >= taskStop && ((model->numSteps - taskStop) % taskStep == 0);
//    }
//    
//    return globalSettings.getBool("Debug Each Task");
//}
//
//void GuideMutator::forceContinue() {
//    forceContinued = true;
//}
//
//void GuideMutator::hideStats() {
//    // Clear statistics display (implementation depends on UI system)
//    // document.getElementById('statistics').innerHTML = '';
//}
//
//Vertex* GuideMutator::findMutableVertex() {
//    auto vertexNodes = nodeStats.getNodes("vertex");
//    if (!vertexNodes.empty()) {
//        for (int i = 0; i < findMutableVertexAttempts; i++) {
//            if (auto vertex = dynamic_cast<Vertex*>(Util::pick(vertexNodes)->getElement())) {
//                if (vertex->isMoveable()) {
//                    return vertex;
//                }
//            }
//        }
//    }
//    return nullptr;
//}
//
//void GuideMutator::initializeHierarchyMutator() {
//    if (globalSettings.getBool("Use Network")) {
//        hierarchyMutator = std::make_unique<NetworkMutator>(classifier, &nodeStats);
//    } else {
//        hierarchyMutator = std::make_unique<FamilyTreeMutator>(classifier, &nodeStats);
//    }
//}

} // namespace ms 