#pragma once
#include "view.h"
#include "main_controller.h"
#include "camera_controller.h"
#include "synthesizer.h"
#include "model.h"

namespace ms {

class GeneratedController {
public:
    GeneratedController(View* view, MainController* mainController);

    static constexpr int BODY_MARGIN = 8;

    enum class Mode {
        FULL = 0,
        GET_GRAMMAR = 1
    };

    enum class ActionButtons {
        SYNTHESIZE = 0,
        RESET = 1,
        PAUSE = 2,
        RESUME = 3,
        PLUS1 = 4,
        PLUS10 = 5,
        PLUS100 = 6,
        TOGGLE = 7,
        GET_EXAMPLE = 8,
        GET_SETTINGS = 9,
        EXPORT = 10
    };

    void activate();
    void notify();
    void synthesize(bool reset);
    void export_();
    void onEscape();

    // Event handlers
    void onMouseMove(const MouseEvent& event);
    void onMouseDown(const MouseEvent& event);
    void onMouseUp(const MouseEvent& event);
    void onMouseOut(const MouseEvent& event);
    void onDoubleClick(const MouseEvent& event);
    void onRightClick(const MouseEvent& event);
    void onKeyPress(const KeyEvent& event);
    void onKeyUp(const KeyEvent& event);

private:
    View* view;
    MainController* mainController;
    Viewport* viewport;
    CameraController* cameraController;
    Synthesizer* synthesizer;
};

} // namespace ms 