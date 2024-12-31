#pragma once
#include "vec2.h"

namespace ms {

class Viewport {
public:
    Viewport(float scale, float centerX, float centerY);

    void setBaseScale(float baseScale);
    void resize(int w, int h);
    
    float transformX(float x);
    float transformY(float y);
    Vec2 inverseTransform(const Vec2& screenPosition);
    void transform(const Vec2& position, const Vec2& offset, Vec2& result);
    Vec2 transform1(const Vec2& position);
    
    void move(float dx, float dy);
    void zoom(int dir, float focusX, float focusY);

    float scale;
    float centerX;
    float centerY;

private:
    float baseScale;
    float baseCenterX;
    float baseCenterY;
    int width;
    int height;
};

} // namespace ms 