#pragma once
#include "vec3.h"

namespace ms {

class Face;

class BspPlane {
public:
    BspPlane(const Vec3& normal, float d);
    static BspPlane create(Face* face);

    // Core accessors
    const Vec3& getNormal() const { return normal; }
    float getD() const { return d; }

    // Operations
    bool isParallel(const BspPlane& planeB) const;
    bool sameD(const BspPlane& planeB) const;
    int sign(const Vec3& query) const;
    Vec3 crossingPoint(const Vec3& p0, const Vec3& p1) const;
    bool isValid() const { return normal != Vec3::ORIGIN; }

    static constexpr float PARALLEL_EPS = 1e-3f;
    static constexpr float COPLANAR_EPS = 1e-5f;

private:
    Vec3 normal;
    float d;
};

} // namespace ms 