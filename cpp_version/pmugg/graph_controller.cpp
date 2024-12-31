#include "graph_controller.h"
#include "timer.h"

namespace ms {

GraphController::GraphController(View* view, MainController* mainController)
    : view(view)
    , mainController(mainController)
    , viewport(nullptr)
    , cameraController(new CameraController(view, [this]() { this->notify(); }))
    , classifier(new Classifier())
    , familyTree() {
}

void GraphController::notify() {
    Timer::start("Redraw");
    
    auto expectedMode = classifier->is3D ? View::Mode::VIEW3D : View::Mode::SHAPE;
    if (view->getMode() != expectedMode) {
        view->setMode(expectedMode);
        viewport = view->getViewport();
        cameraController->activate();
    }
    
    view->redraw(this);
    Timer::stop("Redraw");
}

void GraphController::activate(int mode, std::function<void()> callback) {
    if (mainController->getExample()->isEmpty()) {
        mainController->addTestObject();
    }

    std::vector<std::string> actionButtons = {"Classify"};
    mainController->setModeButtons({});
    mainController->setActionButtons(actionButtons);

    view->setMode(View::Mode::SHAPE);
    viewport = view->getViewport();
    cameraController->activate();

    if (callback) {
        callback();
    }

    notify();
}

void GraphController::classify() {
    Timer::start("Classify");
    classifier->classify(mainController->getExample());
    Timer::stop("Classify");
    notify();
}

void GraphController::onMouseMove(const MouseEvent& event) {
    cameraController->onMouseMove(event);
}

void GraphController::onMouseDown(const MouseEvent& event) {
    cameraController->onMouseDown(event);
}

void GraphController::onMouseUp(const MouseEvent& event) {
    cameraController->onMouseUp(event);
}

void GraphController::onMouseOut(const MouseEvent& event) {
    // Handle mouse out event if needed
}

void GraphController::onDoubleClick(const MouseEvent& event) {
    // Handle double click event if needed
}

void GraphController::onRightClick(const MouseEvent& event) {
    // Handle right click event if needed
}

void GraphController::onEscape(const KeyEvent& event) {
    // Handle escape key event if needed
}

void GraphController::onKeyPress(const KeyEvent& event) {
    switch (event.keyCode) {
        default:
            break;
    }
}

void GraphController::onKeyUp(const KeyEvent& event) {
    // Handle key up events if needed
}

} // namespace ms 