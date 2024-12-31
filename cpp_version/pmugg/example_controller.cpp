#include "example_controller.h"
#include "shape_selector.h"
#include "main_controller.h"

namespace ms {

ExampleController::ExampleController(View* view, ShapeMaker* shapeMaker, 
                                   ExampleShape* exampleShape, 
                                   DecorationModel* decorationModel,
                                   MainController* mainController)
    : view(view)
    , mainController(mainController)
    , shapeMaker(shapeMaker)
    , decorationModel(decorationModel)
    , exampleShape(exampleShape)
    , viewport(nullptr)
    , subMode(SubModeTypes::SELECT) {
    
    auto* shapeSelector = new ShapeSelector(exampleShape);
    auto* shapeView = view->getSubView(View::Mode::SHAPE);
    selector = new Selector(shapeMaker, shapeSelector);
    selector->register_(shapeView);

    mover = new Mover(shapeMaker->exampleShape);
    cameraController = new CameraController(view, 
        [this]() { this->notify(); });
}

void ExampleController::activate() {
    mainController->setModeButtons({"Select Rect", "Lasso", "Fill"});
    mainController->setActionButtons({"Merge", "Copy", "Unlinked"});
    view->setMode(View::Mode::SHAPE, MainController::ControllerTypes::EXAMPLE);
    viewport = view->getViewport();
    cameraController->activate();
    mainController->updateMode(static_cast<int>(ModeTypes::SELECT_RECT));
    exampleShape->setSelectedShape(nullptr);
    notify();
}

void ExampleController::notify() {
    viewport = view->getViewport();
    view->redraw(this);
}

void ExampleController::updateMode(int mode) {
    switch(static_cast<ModeTypes>(mode)) {
        case ModeTypes::SELECT_RECT:
            subMode = SubModeTypes::SELECT;
            break;
        case ModeTypes::LASSO:
            selector->startLasso();
            break;
        case ModeTypes::FILL:
            // Handle fill mode
            break;
    }
    notify();
}

void ExampleController::onMouseMove(const MouseEvent& event) {
    if (cameraController->onMouseMove(event)) {
        return;
    }

    switch(subMode) {
        case SubModeTypes::SELECT:
            selector->onMouseMove(event);
            break;
        case SubModeTypes::MOVE:
            if (mover->onMouseMove(event)) {
                notify();
            }
            break;
        default:
            break;
    }
}

void ExampleController::onMouseDown(const MouseEvent& event) {
    if (cameraController->onMouseDown(event)) {
        return;
    }

    if (event.button == MouseButton::LEFT) {
        switch(subMode) {
            case SubModeTypes::SELECT:
                selector->onMouseDown(event);
                break;
            case SubModeTypes::MOVE:
                if (mover->onMouseDown(event)) {
                    notify();
                }
                break;
            default:
                break;
        }
    }
}

void ExampleController::onMouseUp(const MouseEvent& event) {
    if (cameraController->onMouseUp(event)) {
        return;
    }

    if (event.button == MouseButton::LEFT) {
        switch(subMode) {
            case SubModeTypes::SELECT:
                selector->onMouseUp(event);
                break;
            case SubModeTypes::MOVE:
                if (mover->onMouseUp(event)) {
                    notify();
                }
                break;
            default:
                break;
        }
    }
}

void ExampleController::onKeyPress(const KeyEvent& event) {
    switch(event.keyCode) {
        case KeyCode::ESCAPE:
            onEscape();
            break;
        case KeyCode::DELETE:
            exampleShape->deleteSelected();
            notify();
            break;
        case KeyCode::SHIFT:
            mover->align(exampleShape);
            notify();
            break;
        case KeyCode::A:
            if (event.ctrlKey) {
                selector->selectAll();
            }
            break;
        default:
            break;
    }
}

void ExampleController::onKeyUp(const KeyEvent& event) {
    switch(event.keyCode) {
        case KeyCode::SHIFT:
            mover->unalign();
            notify();
            break;
        default:
            break;
    }
}

void ExampleController::onEscape() {
    selector->cancel();
    subMode = SubModeTypes::SELECT;
    notify();
}

ExampleShape* ExampleController::getExample() {
    return shapeMaker->exampleShape;
}

} // namespace ms 