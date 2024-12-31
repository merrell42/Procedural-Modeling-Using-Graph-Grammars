#include "viewport_3d.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace ms {

Viewport3D::Viewport3D(float width, float height)
    : position(0, 0, 5)
    , target(0, 0, 0)
    , up(0, 1, 0)
    , fov(45.0f * M_PI / 180.0f)  // Convert to radians
    , near(0.1f)
    , far(1000.0f)
    , width(width)
    , height(height)
    , distance(5.0f)
    , theta(0.0f)
    , phi(M_PI / 4.0f)
    , modelMatrix(Mat4::identity()) {
    
    updateAspect();
    updateCamera();
}

Mat4 Viewport3D::getProjectionMatrix() const {
    return Mat4::perspective(fov, aspect, near, far);
}

Mat4 Viewport3D::getViewMatrix() const {
    return Mat4::lookAt(position, target, up);
}

Mat4 Viewport3D::getModelMatrix() const {
    return modelMatrix;
}

void Viewport3D::orbit(float dx, float dy) {
    // Convert screen deltas to angular changes
    float deltaTheta = -dx * 0.01f;
    float deltaPhi = -dy * 0.01f;

    // Update angles
    theta += deltaTheta;
    phi = std::clamp(phi + deltaPhi, 0.01f, M_PI - 0.01f);

    updateCamera();
}

void Viewport3D::pan(float dx, float dy) {
    // Convert screen deltas to world space movement
    Vec3 right = Vec3::cross(target - position, up).normalized();
    Vec3 worldUp = up.normalized();
    
    float scale = distance * 0.001f;
    Vec3 delta = right * (-dx * scale) + worldUp * (dy * scale);

    target += delta;
    position += delta;
}

void Viewport3D::zoom(float factor) {
    // Update distance with limits
    distance = std::clamp(distance * factor, 0.1f, 1000.0f);
    updateCamera();
}

void Viewport3D::fitToPoints(const std::vector<Vec3>& points) {
    if (points.empty()) {
        return;
    }

    // Find bounding box
    Vec3 min(std::numeric_limits<float>::max());
    Vec3 max(std::numeric_limits<float>::lowest());

    for (const auto& point : points) {
        min = Vec3::min(min, point);
        max = Vec3::max(max, point);
    }

    // Calculate center and size
    Vec3 center = (min + max) * 0.5f;
    Vec3 size = max - min;
    float maxSize = std::max({size.x, size.y, size.z});

    // Update camera
    target = center;
    distance = maxSize * 2.0f;
    updateCamera();
}

Vec3 Viewport3D::screenToWorld(float x, float y, float z) const {
    // Convert screen coordinates to normalized device coordinates
    float ndcX = (2.0f * x / width) - 1.0f;
    float ndcY = 1.0f - (2.0f * y / height);
    float ndcZ = 2.0f * z - 1.0f;

    // Create NDC point
    Vec4 ndcPoint(ndcX, ndcY, ndcZ, 1.0f);

    // Transform back to world space
    Mat4 invProjection = getProjectionMatrix().inverse();
    Mat4 invView = getViewMatrix().inverse();
    Mat4 invModel = getModelMatrix().inverse();

    Vec4 worldPoint = invModel * invView * invProjection * ndcPoint;
    return Vec3(worldPoint.x / worldPoint.w,
                worldPoint.y / worldPoint.w,
                worldPoint.z / worldPoint.w);
}

Vec3 Viewport3D::worldToScreen(const Vec3& point) const {
    // Transform point to clip space
    Mat4 mvp = getProjectionMatrix() * getViewMatrix() * getModelMatrix();
    Vec4 clipSpace = mvp * Vec4(point, 1.0f);

    // Perform perspective division
    Vec3 ndc = Vec3(clipSpace.x / clipSpace.w,
                    clipSpace.y / clipSpace.w,
                    clipSpace.z / clipSpace.w);

    // Convert to screen space
    return Vec3((ndc.x + 1.0f) * width * 0.5f,
                (1.0f - ndc.y) * height * 0.5f,
                (ndc.z + 1.0f) * 0.5f);
}

void Viewport3D::updateCamera() {
    position = getOrbitPosition();
}

void Viewport3D::updateAspect() {
    aspect = width / height;
}

Vec3 Viewport3D::getOrbitPosition() const {
    Vec3 offset = sphericalToCartesian(theta, phi, distance);
    return target + offset;
}

Vec3 Viewport3D::sphericalToCartesian(float theta, float phi, float radius) {
    float x = radius * std::sin(phi) * std::cos(theta);
    float y = radius * std::cos(phi);
    float z = radius * std::sin(phi) * std::sin(theta);
    return Vec3(x, y, z);
}

} // namespace ms 