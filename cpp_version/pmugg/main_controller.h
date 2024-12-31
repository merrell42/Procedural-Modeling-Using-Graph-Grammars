#pragma once
#include "view.h"
#include "example_shape.h"
#include "decoration_model.h"
#include "brush_selector.h"
#include "shape_maker.h"
#include "screen_saver.h"
#include <vector>
#include <memory>

namespace ms {

class MainController {
public:
    MainController(View* view, ScreenSaver* screenSaver);
    ~MainController();

    enum class ControllerTypes {
        OBJECT = 0,
        EXAMPLE = 1,
        GENERATED = 2,
        GRAPH = 3
    };

    void setModeButtons(const std::vector<std::string>& buttons);
    void setActionButtons(const std::vector<std::string>& buttons);
    void updateMode(int mode);
    void selectController(ControllerTypes type);
    ExampleShape* getExample();

    // Event handlers
    void onMouseMove(const MouseEvent& event);
    void onMouseDown(const MouseEvent& event);
    void onMouseUp(const MouseEvent& event);
    void onMouseOut(const MouseEvent& event);
    void onDoubleClick(const MouseEvent& event);
    void onRightClick(const MouseEvent& event);
    void onKeyPress(const KeyEvent& event);
    void onKeyUp(const KeyEvent& event);

    // Canvas dimensions
    static int getCanvasWidth();
    static int getCanvasHeight();

    static constexpr float ZOOM_AMOUNT = 1.2f;

private:
    View* view;
    BaseController* currentController;
    ExampleShape* exampleShape;
    DecorationModel* decorationModel;
    BrushSelector* brushSelector;
    std::vector<std::unique_ptr<BaseController>> modeControllers;
};

} // namespace ms 