#include "main_controller.h"
#include "object_controller.h"
#include "example_controller.h"
#include "generated_controller.h"
#include "graph_controller.h"
#include "decoration_view.h"
#include "global_settings.h"

namespace ms {

MainController::MainController(View* view, ScreenSaver* screenSaver)
    : view(view)
    , currentController(nullptr)
    , exampleShape(new ExampleShape())
    , decorationModel(new DecorationModel(exampleShape))
    , brushSelector(nullptr) {

    // Initialize decoration view
    auto* decorationView = new DecorationView(
        "decoration-container", decorationModel);

    // Initialize brush selector if not in MVP mode
    if (!GlobalSettings::isMvpMode()) {
        brushSelector = new BrushSelector(
            "brush-selector", decorationModel);
    }

    // Create shape maker
    auto* shapeMaker = new ShapeMaker(exampleShape, decorationModel);
    shapeMaker->register_(view);

    // Initialize controllers
    modeControllers.emplace_back(new ObjectController(
        view, shapeMaker, decorationModel, this));
    modeControllers.emplace_back(new GeneratedController(
        view, this));
    modeControllers.emplace_back(new GraphController(
        view, this));

    // Set initial controller
    selectController(ControllerTypes::OBJECT);
}

MainController::~MainController() {
    delete exampleShape;
    delete decorationModel;
    delete brushSelector;
}

void MainController::setModeButtons(const std::vector<std::string>& buttons) {
    // Implementation depends on UI framework
}

void MainController::setActionButtons(const std::vector<std::string>& buttons) {
    // Implementation depends on UI framework
}

void MainController::updateMode(int mode) {
    if (currentController) {
        currentController->updateMode(mode);
    }
}

void MainController::selectController(ControllerTypes type) {
    int index = static_cast<int>(type);
    if (index >= 0 && index < modeControllers.size()) {
        currentController = modeControllers[index].get();
        currentController->activate();
    }
}

ExampleShape* MainController::getExample() {
    return exampleShape;
}

void MainController::onMouseMove(const MouseEvent& event) {
    if (currentController) {
        currentController->onMouseMove(event);
    }
}

void MainController::onMouseDown(const MouseEvent& event) {
    if (currentController) {
        currentController->onMouseDown(event);
    }
}

void MainController::onMouseUp(const MouseEvent& event) {
    if (currentController) {
        currentController->onMouseUp(event);
    }
}

void MainController::onMouseOut(const MouseEvent& event) {
    if (currentController) {
        currentController->onMouseOut(event);
    }
}

void MainController::onDoubleClick(const MouseEvent& event) {
    if (currentController) {
        currentController->onDoubleClick(event);
    }
}

void MainController::onRightClick(const MouseEvent& event) {
    if (currentController) {
        currentController->onRightClick(event);
    }
}

void MainController::onKeyPress(const KeyEvent& event) {
    // Handle global shortcuts
    switch (event.keyCode) {
        case KeyCode::NUM_1:
            selectController(ControllerTypes::OBJECT);
            break;
        case KeyCode::NUM_2:
            selectController(ControllerTypes::GENERATED);
            break;
        case KeyCode::NUM_3:
            selectController(ControllerTypes::GRAPH);
            break;
        default:
            if (currentController) {
                currentController->onKeyPress(event);
            }
            break;
    }
}

void MainController::onKeyUp(const KeyEvent& event) {
    if (currentController) {
        currentController->onKeyUp(event);
    }
}

int MainController::getCanvasWidth() {
    // Implementation depends on UI framework
    return 800; // Default value
}

int MainController::getCanvasHeight() {
    // Implementation depends on UI framework
    return 600; // Default value
}

} // namespace ms 