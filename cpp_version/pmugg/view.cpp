#include "view.h"
#include "global_settings.h"

namespace ms {

View::NullDriver View::nullDriver;

View::View(ShapeView* shapeView, GridView* gridView, View3D* view3D) 
    : gridView(gridView)
    , mode(Mode::SHAPE) {
    
    subViews.resize(4);
    subViews[static_cast<int>(Mode::SHAPE)] = shapeView;
    subViews[static_cast<int>(Mode::GRID)] = gridView;
    subViews[static_cast<int>(Mode::VIEW3D)] = view3D;
}

void View::setMode(Mode mode, int controllerType) {
    if (mode == Mode::GRID && GlobalSettings::get("Use WebGL")) {
        mode = Mode::WEBGL;
    }
    
    this->mode = mode;
    for (size_t i = 0; i < subViews.size(); i++) {
        if (i == static_cast<int>(mode)) {
            subViews[i]->activate(controllerType);
        } else {
            subViews[i]->deactivate();
        }
    }
}

BaseView* View::getSubView(Mode mode) {
    if (mode == Mode::GRID && GlobalSettings::get("Use WebGL")) {
        mode = Mode::WEBGL;
    }
    return subViews[static_cast<int>(mode)];
}

Viewport* View::getViewport() {
    return subViews[static_cast<int>(mode)]->getViewport();
}

void View::resize(int width, int height) {
    for (auto* view : subViews) {
        if (view) {
            view->resize(width, height);
        }
    }
}

void View::redraw(Driver* driver, bool fullRedraw) {
    getSubView(mode)->redraw(driver, fullRedraw);
    
    // Handle highlighted elements that require shape view
    if (HighlightedElement::current && 
        HighlightedElement::current->requiresShapeView() &&
        mode != Mode::SHAPE && 
        mode != Mode::VIEW3D) {
        
        auto* shapeView = getSubView(Mode::SHAPE);
        shapeView->activateOver();
        shapeView->redraw(&nullDriver);
    }
}

} // namespace ms 