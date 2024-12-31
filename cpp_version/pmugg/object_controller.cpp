#include "object_controller.h"
#include "brush.h"
#include "area.h"

namespace ms {

ObjectController::ObjectController(View* view, ShapeMaker* shapeMaker,
                                 DecorationModel* decorationModel,
                                 MainController* mainController)
    : view(view)
    , mainController(mainController)
    , shapeMaker(shapeMaker)
    , decorationModel(decorationModel)
    , viewport(nullptr)
    , subMode(SubModeTypes::SELECT) {
    
    auto* edgeSelector = new EdgeSelector(shapeMaker);
    auto* shapeView = view->getSubView(View::Mode::SHAPE);
    selector = new Selector(shapeMaker, edgeSelector);
    selector->register_(shapeView);

    mover = new Mover(shapeMaker);
    cameraController = new CameraController(view, 
        [this]() { this->notify(); });
}

ObjectController::~ObjectController() {
    delete selector;
    delete mover;
    delete cameraController;
}

void ObjectController::activate() {
    std::vector<std::string> modeButtons = {
        "Select Rect", "Lasso", "Fill", "Draw", "Draw Curve"
    };
    std::vector<std::string> actionButtons = {
        "Merge", "Copy", "Unlinked", "Add Boundary"
    };

    mainController->setModeButtons(modeButtons);
    mainController->setActionButtons(actionButtons);
    view->setMode(View::Mode::SHAPE);
    viewport = view->getViewport();
    cameraController->activate();
    mainController->updateMode(static_cast<int>(ModeTypes::SELECT_RECT));
    notify();
}

void ObjectController::notify() {
    viewport = view->getViewport();
    view->redraw(this);
}

void ObjectController::updateMode(int mode) {
    switch (static_cast<ModeTypes>(mode)) {
        case ModeTypes::SELECT_RECT:
            subMode = SubModeTypes::SELECT;
            selector->setLasso(false);
            break;
        case ModeTypes::LASSO:
            subMode = SubModeTypes::SELECT;
            selector->setLasso(true);
            break;
        case ModeTypes::FILL:
            subMode = SubModeTypes::SELECT;
            break;
        case ModeTypes::DRAW:
            subMode = SubModeTypes::DRAW;
            break;
        case ModeTypes::DRAW_CURVE:
            subMode = SubModeTypes::DRAW_CURVE;
            break;
    }
    notify();
}

void ObjectController::onMouseMove(const MouseEvent& event) {
    if (cameraController->onMouseMove(event)) {
        return;
    }

    switch (subMode) {
        case SubModeTypes::DRAW:
        case SubModeTypes::DRAW_CURVE:
            handleDrawMode(event);
            break;
        case SubModeTypes::MOVE:
            handleMoveMode(event);
            break;
        case SubModeTypes::SELECT:
            handleSelectMode(event);
            break;
    }
}

void ObjectController::handleDrawMode(const MouseEvent& event) {
    if (shapeMaker->onMouseMove(event)) {
        notify();
    }
}

void ObjectController::handleMoveMode(const MouseEvent& event) {
    if (mover->onMouseMove(event)) {
        notify();
    }
}

void ObjectController::handleSelectMode(const MouseEvent& event) {
    selector->onMouseMove(event);
}

void ObjectController::onMouseDown(const MouseEvent& event) {
    if (cameraController->onMouseDown(event)) {
        return;
    }

    if (event.button == MouseButton::LEFT) {
        switch (subMode) {
            case SubModeTypes::DRAW:
            case SubModeTypes::DRAW_CURVE:
                if (shapeMaker->onMouseDown(event)) {
                    notify();
                }
                break;
            case SubModeTypes::MOVE:
                if (mover->onMouseDown(event)) {
                    notify();
                }
                break;
            case SubModeTypes::SELECT:
                selector->onMouseDown(event);
                break;
        }
    }
}

void ObjectController::onMouseUp(const MouseEvent& event) {
    if (cameraController->onMouseUp(event)) {
        return;
    }

    if (event.button == MouseButton::LEFT) {
        switch (subMode) {
            case SubModeTypes::DRAW:
            case SubModeTypes::DRAW_CURVE:
                if (shapeMaker->onMouseUp(event)) {
                    notify();
                }
                break;
            case SubModeTypes::MOVE:
                if (mover->onMouseUp(event)) {
                    notify();
                }
                break;
            case SubModeTypes::SELECT:
                selector->onMouseUp(event);
                break;
        }
    }
}

void ObjectController::onKeyPress(const KeyEvent& event) {
    switch (event.keyCode) {
        case KeyCode::ESCAPE:
            onEscape();
            break;
        case KeyCode::DELETE:
            shapeMaker->deleteSelected();
            notify();
            break;
        case KeyCode::SHIFT:
            mover->align();
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

void ObjectController::onKeyUp(const KeyEvent& event) {
    switch (event.keyCode) {
        case KeyCode::SHIFT:
            mover->unalign();
            notify();
            break;
        default:
            break;
    }
}

void ObjectController::onEscape() {
    selector->cancel();
    shapeMaker->cancel();
    subMode = SubModeTypes::SELECT;
    notify();
}

void ObjectController::addBoundary() {
    createNewBoundary();
}

void ObjectController::createNewBoundary() {
    auto onChange = [this]() { decorationModel->notify(); };
    
    auto* newBrush = new Brush("#000", "#000", onChange);
    newBrush->set("Bend", 0);
    newBrush->set("Boundary", true);
    decorationModel->brushCollections[0].addBrush(newBrush);
    
    if (auto* brushSelector = mainController->getBrushSelector()) {
        brushSelector->notify();
    }
    
    auto* boundaryArea = new Area("#bde", "#bde", onChange);
    boundaryArea->set("Boundary", true);
    decorationModel->brushCollections[1].addBrush(boundaryArea);
    
    shapeMaker->addBoundary(newBrush, boundaryArea);
}

void ObjectController::onMouseOut(const MouseEvent& event) {
    // Handle mouse out event if needed
}

void ObjectController::onDoubleClick(const MouseEvent& event) {
    // Handle double click event if needed
}

void ObjectController::onRightClick(const MouseEvent& event) {
    // Handle right click event if needed
}

} // namespace ms 