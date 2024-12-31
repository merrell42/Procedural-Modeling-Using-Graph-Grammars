#pragma once
#include "viewport.h"
#include "canvas.h"
#include "face.h"
#include <vector>

namespace ms {

class GridView {
public:
    GridView(Canvas* gridCanvas, Canvas* objectCanvas, Canvas* offscreenCanvas);

    static constexpr int MARGIN = 25;

    Viewport* getViewport();
    void setExtents(const std::vector<float>& extents);
    void resize(int width, int height);
    void redraw(Driver* driver, bool fullRedraw = false);
    void drawGrid();
    void drawObject(Driver* driver);

private:
    Canvas* gridCanvas;
    Context* context;
    Canvas* objectCanvas;
    Canvas* offscreenCanvas;
    Viewport* viewport;

    static void drawFace(Face* face, 
                        std::vector<Face*>& drawnFaces,
                        Context* context,
                        const std::function<Vec2(const Vec2&)>& convertToScreen);

    void drawGridLines(float minX, float maxX, float minY, float maxY);
    void drawAxes(float minX, float maxX, float minY, float maxY);
};

} // namespace ms 