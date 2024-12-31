#include "setup.h"
#include "window.h"
#include "event_handler.h"

namespace ms {

// Initialize static members
Canvas* Setup::canvas1 = nullptr;
Canvas* Setup::gridCanvas = nullptr;
Canvas* Setup::canvas2 = nullptr;
Canvas* Setup::canvas3d = nullptr;
ShapeView* Setup::shapeView = nullptr;
GridView* Setup::gridView = nullptr;
View3D* Setup::view3d = nullptr;
MainController* Setup::controller = nullptr;

void Setup::initialize() {
    createCanvases();
    setupViews();
    setupController();
    setupGlobalSettings();
    setupEventListeners(controller);
}

void Setup::createCanvases() {
    // Create main canvas
    canvas1 = Window::getElementById<Canvas>("canvas1");
    if (!canvas1) {
        throw std::runtime_error("Failed to create main canvas");
    }

    // Create grid canvas
    gridCanvas = Window::getElementById<Canvas>("grid-canvas");
    if (!gridCanvas) {
        throw std::runtime_error("Failed to create grid canvas");
    }

    // Create offscreen canvas
    canvas2 = Window::getElementById<Canvas>("canvas2");
    if (!canvas2) {
        throw std::runtime_error("Failed to create offscreen canvas");
    }

    // Create 3D canvas
    canvas3d = Window::getElementById<Canvas>("canvas-3d");
    if (!canvas3d) {
        throw std::runtime_error("Failed to create 3D canvas");
    }
}

void Setup::setupViews() {
    // Create shape view
    shapeView = new ShapeView(canvas1);
    if (!shapeView) {
        throw std::runtime_error("Failed to create shape view");
    }

    // Create grid view
    gridView = new GridView(gridCanvas, canvas1, canvas2);
    if (!gridView) {
        throw std::runtime_error("Failed to create grid view");
    }

    // Create 3D view
    view3d = new View3D(canvas3d);
    if (!view3d) {
        throw std::runtime_error("Failed to create 3D view");
    }
}

void Setup::setupController() {
    // Create screen saver
    auto* screenSaver = new ScreenSaver(gridCanvas, canvas2);
    if (!screenSaver) {
        throw std::runtime_error("Failed to create screen saver");
    }

    // Create view
    auto* view = new View(shapeView, gridView, view3d);
    if (!view) {
        throw std::runtime_error("Failed to create view");
    }

    // Create main controller
    controller = new MainController(view, screenSaver);
    if (!controller) {
        throw std::runtime_error("Failed to create main controller");
    }
}

void Setup::setupGlobalSettings() {
    auto* settingsContainer = Window::getElementById("settings-container");
    if (settingsContainer) {
        GlobalSettings::draw(settingsContainer);
    }
}

void Setup::setupEventListeners(MainController* controller) {
    // Mouse event handlers
    auto mouseMove = [controller](const MouseEvent& event) {
        controller->onMouseMove(event);
    };
    
    auto mouseDown = [controller](const MouseEvent& event) {
        controller->onMouseDown(event);
    };
    
    auto mouseUp = [controller](const MouseEvent& event) {
        controller->onMouseUp(event);
    };
    
    auto mouseOut = [controller](const MouseEvent& event) {
        controller->onMouseOut(event);
    };

    // Keyboard event handlers
    auto keyPress = [controller](const KeyEvent& event) {
        controller->onKeyPress(event);
    };
    
    auto keyUp = [controller](const KeyEvent& event) {
        controller->onKeyUp(event);
    };

    // Register event listeners
    EventHandler::addEventListener(EventType::MOUSEMOVE, mouseMove);
    EventHandler::addEventListener(EventType::MOUSEDOWN, mouseDown);
    EventHandler::addEventListener(EventType::MOUSEUP, mouseUp);
    EventHandler::addEventListener(EventType::MOUSEOUT, mouseOut);
    EventHandler::addEventListener(EventType::KEYPRESS, keyPress);
    EventHandler::addEventListener(EventType::KEYUP, keyUp);

    // Window resize handler
    auto resize = [controller]() {
        // Handle window resize
        // Update canvas sizes and notify controller
    };
    
    EventHandler::addEventListener(EventType::RESIZE, resize);
}

} // namespace ms 