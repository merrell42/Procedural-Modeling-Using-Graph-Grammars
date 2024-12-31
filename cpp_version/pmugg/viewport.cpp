#include "viewport.h"
#include "main_controller.h"

namespace ms {

Viewport::Viewport(float scale, float centerX, float centerY)
    : scale(scale)
    , centerX(centerX)
    , centerY(centerY)
    , baseScale(scale)
    , baseCenterX(centerX)
    , baseCenterY(centerY)
    , width(MainController::getCanvasWidth())
    , height(MainController::getCanvasHeight()) {
}

void Viewport::setBaseScale(float baseScale) {
    this->baseScale = baseScale;
}

void Viewport::resize(int w, int h) {
    width = w;
    height = h;
}

float Viewport::transformX(float x) {
    return scale * (x - centerX) + width / 2;
}

float Viewport::transformY(float y) {
    return scale * (y - centerY) + height / 2;
}

Vec2 Viewport::inverseTransform(const Vec2& screenPosition) {
    float x = (screenPosition.x - width / 2) / scale + centerX;
    float y = (screenPosition.y - height / 2) / scale + centerY;
    return Vec2(x, y);
}

void Viewport::transform(const Vec2& position, const Vec2& offset, Vec2& result) {
    float x = position.x + offset.x;
    float y = position.y + offset.y;
    result.x = transformX(x);
    result.y = transformY(y);
}

Vec2 Viewport::transform1(const Vec2& position) {
    Vec2 result(0, 0);
    transform(position, Vec2(0, 0), result);
    return result;
}

void Viewport::move(float dx, float dy) {
    centerX += dx / scale;
    centerY += dy / scale;
}

void Viewport::zoom(int dir, float focusX, float focusY) {
    float startScale = scale;
    float z = (dir == 1) ? 
        MainController::ZOOM_AMOUNT : 
        1.0f / MainController::ZOOM_AMOUNT;
    
    scale *= z;
    
    if (scale < baseScale) {
        scale = baseScale;
        centerX = baseCenterX;
        centerY = baseCenterY;
    } else {
        float fx = centerX + (focusX - width / 2) / startScale;
        float fy = centerY + (focusY - height / 2) / startScale;
        centerX = fx + (centerX - fx) / z;
        centerY = fy + (centerY - fy) / z;
    }
}

} // namespace ms 