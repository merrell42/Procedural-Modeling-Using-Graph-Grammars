#pragma once
#include "view.h"
#include "shape_maker.h"
#include "decoration_model.h"
#include "main_controller.h"
#include "edge_selector.h"
#include "selector.h"
#include "camera_controller.h"
#include "mover.h"

namespace ms {

class ObjectController {
public:
    ObjectController(View* view, ShapeMaker* shapeMaker, 
                    DecorationModel* decorationModel,
                    MainController* mainController);
    ~ObjectController();

    enum class SubModeTypes {
        SELECT = 0,
        MOVE = 1,
        DRAW = 2,
        DRAW_CURVE = 3
    };

    enum class ModeTypes {
        SELECT_RECT = 0,
        LASSO = 1,
        FILL = 2,
        DRAW = 3,
        DRAW_CURVE = 4
    };

    enum class ActionButtons {
        MERGE = 0,
        COPY_LINKED = 1,
        COPY_UNLINKED = 2,
        ADD_BOUNDARY = 3
    };

    void activate();
    void notify();
    void updateMode(int mode);
    void onEscape();
    void addBoundary();

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
    ShapeMaker* shapeMaker;
    DecorationModel* decorationModel;
    Selector* selector;
    Viewport* viewport;
    CameraController* cameraController;
    Mover* mover;
    SubModeTypes subMode;

    void handleDrawMode(const MouseEvent& event);
    void handleMoveMode(const MouseEvent& event);
    void handleSelectMode(const MouseEvent& event);
    void createNewBoundary();
};

} // namespace ms 