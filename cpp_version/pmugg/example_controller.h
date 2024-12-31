#pragma once
#include "view.h"
#include "shape_maker.h"
#include "example_shape.h"
#include "decoration_model.h"
#include "selector.h"
#include "mover.h"
#include "camera_controller.h"
#include "shape_selector.h"
#include "main_controller.h"

namespace ms {

class ExampleController {
public:
    ExampleController(View* view, ShapeMaker* shapeMaker, 
                     ExampleShape* exampleShape, 
                     DecorationModel* decorationModel,
                     MainController* mainController);

    void activate();
    void notify();
    void updateMode(int mode);
    
    enum class ModeTypes {
        SELECT_RECT = 0,
        LASSO = 1,
        FILL = 2
    };

    enum class ActionButtons {
        MERGE = 0,
        COPY_LINKED = 1,
        COPY_UNLINKED = 2
    };

private:
    View* view;
    MainController* mainController;
    ShapeMaker* shapeMaker;
    DecorationModel* decorationModel;
    ExampleShape* exampleShape;
    Selector* selector;
    Viewport* viewport;
    Mover* mover;
    CameraController* cameraController;
    
    enum class SubModeTypes {
        NONE = 0,
        SELECT = 1,
        MOVE = 2
    };

    SubModeTypes subMode;
};

} // namespace ms 