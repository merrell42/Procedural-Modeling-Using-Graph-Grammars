#pragma once
#include "vec3.h"
#include "mat4.h"
#include <vector>

namespace ms {

class Viewport3D {
public:
    Viewport3D(float width, float height);

    // Camera properties
    Vec3 position;
    Vec3 target;
    Vec3 up;
    float fov;
    float near;
    float far;

    // Viewport properties
    float width;
    float height;
    float aspect;

    // Matrix getters
    Mat4 getProjectionMatrix() const;
    Mat4 getViewMatrix() const;
    Mat4 getModelMatrix() const;

    // Camera manipulation
    void orbit(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float factor);
    void fitToPoints(const std::vector<Vec3>& points);

    // Transformation helpers
    Vec3 screenToWorld(float x, float y, float z) const;
    Vec3 worldToScreen(const Vec3& point) const;

private:
    // Camera state
    float distance;
    float theta;
    float phi;
    Mat4 modelMatrix;

    // Internal methods
    void updateCamera();
    void updateAspect();
    Vec3 getOrbitPosition() const;
    static Vec3 sphericalToCartesian(float theta, float phi, float radius);
};

} // namespace ms 