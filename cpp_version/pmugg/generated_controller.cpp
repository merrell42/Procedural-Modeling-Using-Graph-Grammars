#include "generated_controller.h"

namespace ms {

GeneratedController::GeneratedController(View* view, MainController* mainController)
    : view(view)
    , mainController(mainController)
    , viewport(nullptr)
    , cameraController(new CameraController(view, [this]() { this->notify(); }))
    , synthesizer(new Synthesizer(new Model(Vec3(0, 0, 0)), view, cameraController)) {
}

void GeneratedController::activate() {
    std::vector<std::string> actionButtons = {
        "Synthesize", "Reset", "Pause", "Resume",
        "+1", "+10", "+100", "Toggle",
        "Get Example", "Get Settings", "Export"
    };

    mainController->setModeButtons({});
    mainController->setActionButtons(actionButtons);
    view->setMode(View::Mode::GRID);
    viewport = view->getViewport();
    cameraController->activate();
    notify();
}

void GeneratedController::notify() {
    viewport = view->getViewport();
    view->redraw(this);
}

void GeneratedController::synthesize(bool reset) {
    if (reset) {
        synthesizer->reset();
    }
    synthesizer->resume();
}

void GeneratedController::export_() {
    // Implementation depends on export functionality requirements
}

void GeneratedController::onMouseMove(const MouseEvent& event) {
    cameraController->onMouseMove(event);
}

void GeneratedController::onMouseDown(const MouseEvent& event) {
    cameraController->onMouseDown(event);
}

void GeneratedController::onMouseUp(const MouseEvent& event) {
    cameraController->onMouseUp(event);
}

void GeneratedController::onMouseOut(const MouseEvent& event) {
    // Handle mouse out event if needed
}

void GeneratedController::onDoubleClick(const MouseEvent& event) {
    // Handle double click event if needed
}

void GeneratedController::onRightClick(const MouseEvent& event) {
    // Handle right click event if needed
}

void GeneratedController::onEscape() {
    synthesizer->pause();
}

void GeneratedController::onKeyPress(const KeyEvent& event) {
    // Handle numeric keys for step sizes
    if (event.keyCode >= KeyCode::NUM_1 && event.keyCode <= KeyCode::NUM_9) {
        int power = event.keyCode - KeyCode::NUM_1;
        synthesizer->step(std::pow(10, power));
        return;
    }

    switch (event.keyCode) {
        case KeyCode::SPACE:
            synthesizer->resume();
            break;
        case KeyCode::ESCAPE:
            onEscape();
            break;
        case KeyCode::E:
            export_();
            break;
        case KeyCode::R:
            synthesize(true);
            break;
        case KeyCode::S:
            synthesize(false);
            break;
        default:
            break;
    }
}

void GeneratedController::onKeyUp(const KeyEvent& event) {
    // Handle key up events if needed
}

} // namespace ms 