#include "grid_view.h"

namespace ms {

GridView::GridView(Canvas* gridCanvas, Canvas* objectCanvas, Canvas* offscreenCanvas)
    : gridCanvas(gridCanvas)
    , context(gridCanvas->getContext())
    , objectCanvas(objectCanvas)
    , offscreenCanvas(offscreenCanvas)
    , viewport(new Viewport(1, 1, 1)) {
}

Viewport* GridView::getViewport() {
    return viewport;
}

void GridView::setExtents(const std::vector<float>& extents) {
    int margin = MARGIN;
    int canvasWidth = gridCanvas->getWidth();
    int canvasHeight = gridCanvas->getHeight();
    
    // The mutation area has a margin of 1 for historical reasons.
    // Subtract the margin to better fill the screen.
    float scale = std::min(
        (canvasWidth - margin) / (extents[0] - 1),
        (canvasHeight - margin) / (extents[1] - 1));
        
    viewport = new Viewport(scale, extents[0] / 2, extents[1] / 2);
}

void GridView::resize(int width, int height) {
    gridCanvas->resize(width, height);
    objectCanvas->resize(width, height);
    offscreenCanvas->resize(width, height);
}

void GridView::redraw(Driver* driver, bool fullRedraw) {
    if (fullRedraw) {
        drawGrid();
    }
    drawObject(driver);
}

void GridView::drawGrid() {
    context->clearRect(0, 0, gridCanvas->getWidth(), gridCanvas->getHeight());
    
    float scale = viewport->scale;
    float minX = viewport->centerX - gridCanvas->getWidth() / (2 * scale);
    float maxX = viewport->centerX + gridCanvas->getWidth() / (2 * scale);
    float minY = viewport->centerY - gridCanvas->getHeight() / (2 * scale);
    float maxY = viewport->centerY + gridCanvas->getHeight() / (2 * scale);
    
    drawGridLines(minX, maxX, minY, maxY);
    drawAxes(minX, maxX, minY, maxY);
}

void GridView::drawGridLines(float minX, float maxX, float minY, float maxY) {
    context->beginPath();
    context->setStrokeStyle("#eee");
    
    // Draw vertical lines
    for (int x = std::ceil(minX); x <= maxX; x++) {
        float screenX = viewport->transformX(x);
        context->moveTo(screenX, 0);
        context->lineTo(screenX, gridCanvas->getHeight());
    }
    
    // Draw horizontal lines
    for (int y = std::ceil(minY); y <= maxY; y++) {
        float screenY = viewport->transformY(y);
        context->moveTo(0, screenY);
        context->lineTo(gridCanvas->getWidth(), screenY);
    }
    
    context->stroke();
}

void GridView::drawAxes(float minX, float maxX, float minY, float maxY) {
    context->beginPath();
    context->setStrokeStyle("#888");
    
    // Draw x-axis if visible
    if (minY <= 0 && 0 <= maxY) {
        float y = viewport->transformY(0);
        context->moveTo(0, y);
        context->lineTo(gridCanvas->getWidth(), y);
    }
    
    // Draw y-axis if visible
    if (minX <= 0 && 0 <= maxX) {
        float x = viewport->transformX(0);
        context->moveTo(x, 0);
        context->lineTo(x, gridCanvas->getHeight());
    }
    
    context->stroke();
}

void GridView::drawObject(Driver* driver) {
    auto* objContext = objectCanvas->getContext();
    objContext->clearRect(0, 0, objectCanvas->getWidth(), objectCanvas->getHeight());
    
    auto convertToScreen = [this](const Vec2& pos) {
        return viewport->transform1(pos);
    };
    
    std::vector<Face*> drawnFaces;
    for (auto* face : driver->getRenderables()) {
        drawFace(face, drawnFaces, objContext, convertToScreen);
    }
}

void GridView::drawFace(Face* face, 
                       std::vector<Face*>& drawnFaces,
                       Context* context,
                       const std::function<Vec2(const Vec2&)>& convertToScreen) {
    if (std::find(drawnFaces.begin(), drawnFaces.end(), face) != drawnFaces.end()) {
        return;
    }

    // Draw enclosed faces first
    for (auto* endpoint : face->getEndpoints()) {
        for (auto* connection : endpoint->getConnections()) {
            auto* endpointB = connection->getEndpoints()[0];
            auto* faceB = endpointB ? endpointB->getFace() : nullptr;
            if (faceB && (faceB != face) && 
                std::find(drawnFaces.begin(), drawnFaces.end(), faceB) == drawnFaces.end()) {
                drawFace(faceB, drawnFaces, context, convertToScreen);
            }
        }
    }
    
    face->draw(context, convertToScreen);
    drawnFaces.push_back(face);
}

} // namespace ms 