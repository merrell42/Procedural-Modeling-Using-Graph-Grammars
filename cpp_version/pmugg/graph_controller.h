#pragma once
#include "view.h"
#include "main_controller.h"
#include "camera_controller.h"
#include "classifier.h"
#include <vector>

namespace ms {

class GraphController {
public:
    GraphController(View* view, MainController* mainController);

    enum class ActionButtons {
        CLASSIFY = 0
    };

    void notify();
    void activate(int mode = -1, std::function<void()> callback = nullptr);
    void classify();

    // Event handlers
    void onMouseMove(const MouseEvent& event);
    void onMouseDown(const MouseEvent& event);
    void onMouseUp(const MouseEvent& event);
    void onMouseOut(const MouseEvent& event);
    void onDoubleClick(const MouseEvent& event);
    void onRightClick(const MouseEvent& event);
    void onEscape(const KeyEvent& event);
    void onKeyPress(const KeyEvent& event);
    void onKeyUp(const KeyEvent& event);

private:
    View* view;
    MainController* mainController;
    Viewport* viewport;
    CameraController* cameraController;
    Classifier* classifier;
    std::vector<Shape*> familyTree;
};

} // namespace ms 