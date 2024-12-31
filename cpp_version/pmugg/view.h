#pragma once
#include "shape_view.h"
#include "grid_view.h"
#include "view_3d.h"
#include "viewport.h"
#include <vector>

namespace ms {

class View {
public:
    View(ShapeView* shapeView, GridView* gridView, View3D* view3D);

    enum class Mode {
        SHAPE = 0,
        GRID = 1,
        VIEW3D = 2,
        WEBGL = 3
    };

    void setMode(Mode mode, int controllerType = -1);
    BaseView* getSubView(Mode mode);
    Viewport* getViewport();
    void resize(int width, int height);
    void redraw(Driver* driver, bool fullRedraw = false);

private:
    GridView* gridView;
    std::vector<BaseView*> subViews;
    Mode mode;

    struct NullDriver {
        std::vector<Renderable*> getRenderables() { return {}; }
        Vec2 offset() { return Vec2(0, 0); }
    };

    static NullDriver nullDriver;
};

} // namespace ms 