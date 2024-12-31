#pragma once
#include "base_view.h"
#include "canvas.h"
#include "driver.h"
#include "viewport.h"
#include <memory>

namespace ms {

class ShapeView : public BaseView {
public:
    explicit ShapeView(Canvas* canvas);
    ~ShapeView() override;

    void activate(int controllerType = -1) override;
    void deactivate() override;
    void activateOver();
    void resize(int width, int height) override;
    void redraw(Driver* driver, bool fullRedraw = false) override;
    Viewport* getViewport() override;

private:
    Canvas* canvas;
    std::unique_ptr<Viewport> viewport;
    bool isOver;

    void drawShape(Driver* driver);
    void drawSelection(Driver* driver);
    void drawHighlight(Driver* driver);
    void drawDecorations(Driver* driver);
};

} // namespace ms 