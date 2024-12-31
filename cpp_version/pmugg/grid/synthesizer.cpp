#include "synthesizer.h"
#include "view.h"
#include "camera_controller.h"
#include "classifier.h"
#include "guide_mutator.h"
#include "mutation_area.h"
#include "settings.h"
#include "exporter.h"
#include <iostream>

namespace ms {

Synthesizer::Synthesizer(Example* example, View* view, CameraController* cameraController)
    : seeder(nullptr)
    , observer(view)
    , model(new Model({10, 10, 1}))
    , mutator(nullptr)
    , mutationArea(nullptr)
    , numAttempts(0)
    , timer(timerG)
    , cameraController(cameraController)
    , is3D(false)
    , callback([](const std::string&){})
    , mutationOrigin(0, 0, 0)
    , timedOut(false) {
    
    if (example) {
        setExample(example);
    }
}

Synthesizer::~Synthesizer() {
    delete model;
}

Model* Synthesizer::getModel() {
    return model;
}

Vec2 Synthesizer::offset() const {
    return Vec2::ORIGIN;
}

void Synthesizer::notify(bool fullRedraw) {
    timer->start("Redraw");
    observer->redraw(getModel(), fullRedraw);
    mutator->displayStats();
    mutator->setBreakTime(getCurrentTime() + globalSettings.get("Redraw Time") * 1000);
    timer->stop("Redraw");
}

void Synthesizer::setExample(Example* example) {
    if (globalSettings.getBool("Use Boundary Cells")) {
        std::cout << "Guidelines does not use boundary cells." << std::endl;
    }
    highlightedElement = nullptr;

    auto shapes = example->getShapes();
    is3D = shapes[0]->is3D;
    auto expectedMode = is3D ? View::Mode::VIEW3D : View::Mode::GRID;
    
    if (observer->getMode() != expectedMode) {
        observer->setMode(expectedMode);
        viewport = observer->getViewport();
        cameraController->activate();
    }

    auto guideClassifier = new Classifier();
    example->updateFaces();
    
    if (example->solution) {
        guideClassifier->importSolution(example->solution);
        guideClassifier->is3D = example->shapes[0]->is3D;
    } else {
        guideClassifier->setShapes(shapes);
    }
    
    guideClassifier->boundaryGroups = guideClassifier->hierarchy.boundaryGroups;
    is3D = guideClassifier->is3D;
    mutator = new GuideMutator(guideClassifier, timer, 
                              std::bind(&Synthesizer::notify, this, std::placeholders::_1));
}

void Synthesizer::initializeMutationArea() {
    if (globalSettings.getBool("Incremental Mutation")) {
        mutationOrigin = globalSettings.getBool("Empty Border") ? 
                        Vec3(1, 1, 0) : Vec3(0, 0, 0);
        
        auto mutationSize = globalSettings.get("Extents");
        mutationSize[0] -= 2;
        mutationSize[1] -= 2;
        
        if (!is3D) {
            mutationSize[2] = 1;
        }
        mutationArea->setExtents(mutationOrigin, mutationSize);
    } else {
        mutationArea = new MutationArea(model, mutator->getNodeStats());
        startMutation();
        mutator->unresolve();
    }
}

void Synthesizer::synthesize(const std::vector<int>& extents, bool keepPaused,
                           std::function<void(const std::string&)> callback) {
    auto finalExtents = extents;
    if (!is3D) {
        finalExtents[2] = 1;
    }
    
    timer->start("everything");
    updateRandomMode();

    model = new Model(finalExtents);
    notify(true);
    this->callback = callback;
    
    resetCounters();
    
    mutationArea = new MutationArea(model, mutator->getNodeStats());
    numAttempts = 0;
    
    auto redrawTime = globalSettings.get("Redraw Time") * 1000;
    auto maxTime = globalSettings.get("Max Time") * 1000;
    mutator->setBreakTime(getCurrentTime() + redrawTime);
    mutator->setEndTime(getCurrentTime() + 0.9 * maxTime, 
                       getCurrentTime() + maxTime);
    initializeMutationArea();

    timedOut = false;
    if (keepPaused) {
        startMutation();
        pause();
    } else {
        mutateUntilDone();
    }
}

void Synthesizer::finish() {
    timer->stop("everything");
    timer->save();
    std::cout << timer->reportMean() << std::endl;
    model->setFinished(true);
    notify(true);
    callback(mutator->report());
}

void Synthesizer::mutateUntilDone() {
    if (timedOut && timer->has("Timed out")) {
        timer->stop("Timed out");
    }
    
    if (globalSettings.getBool("Incremental Mutation")) {
        auto extents = model->getExtents();
        auto SIZE = 1;  // globalSettings.get("Mutation Size")
        
        if (mutationOrigin[2] + SIZE > extents[2]) {
            if (globalSettings.getBool("Max Time Enabled")) {
                initializeMutationArea();
            } else {
                finish();
                return;
            }
        }
    } else {
        if (mutator->getStatus() != GuideMutator::Status::UNRESOLVED) {
            notify();
            timer->save();
            std::cout << timer->reportMean() << std::endl;
            return;
        }
    }
    
    mutate();
    if (mutator->getStatus() != GuideMutator::Status::PAUSED && 
        mutator->getStatus() != GuideMutator::Status::FINISHED) {
        timer->start("Timed out");
        timedOut = true;
        scheduleNextFrame(std::bind(&Synthesizer::mutateUntilDone, this));
    }
}

void Synthesizer::pause() {
    mutator->pause();
}

void Synthesizer::resume() {
    mutator->unresolve();
    scheduleNextFrame(std::bind(&Synthesizer::mutateUntilDone, this));
}

void Synthesizer::step(int taskNum) {
    mutator->step(taskNum);
    resume();
}

void Synthesizer::startMutation() {
    timer->start("Start Mutation");
    mutationArea->free();
    mutator->save();
    
    bool initialized = false;
    while (!initialized) {
        initializeMutationArea();
        mutator->reset();
        mutator->seed(mutationArea);
        mutationArea->free();
        initialized = mutator->initializeModel(model);
    }
    
    mutator->save();
    timer->stop("Start Mutation");
}

void Synthesizer::mutate() {
    auto status = mutator->getStatus();
    if (status != GuideMutator::Status::UNRESOLVED && 
        status != GuideMutator::Status::PAUSED) {
        startMutation();
    }
    
    status = mutator->resolve(model);
    switch (status) {
        case GuideMutator::Status::UNRESOLVED:
            notify(globalSettings.getBool("Show Faces"));
            break;
        case GuideMutator::Status::FINISHED:
            finish();
            break;
    }
}

void Synthesizer::onMouseDown(float x, float y) {
    auto viewport = observer->getViewport();
    auto modelPosition = viewport->inverseTransform(Vec2(x, y));
    modelPosition.y = model->getExtents()[1] - modelPosition.y;
    
    auto rayCast = model->rayCastLeft(modelPosition);
    if (rayCast) {
        auto face = rayCast->endpoint->getFace();
        face->print();
        auto enclosed = face->enclosedFace();
        if (enclosed) {
            // enclosed->print();
        }
    }
}

std::string Synthesizer::exportOutput() {
    auto xml = mutator->classifier->xml;
    return Exporter::exportModel(model->nodeStats, xml, viewport);
}

} // namespace ms 