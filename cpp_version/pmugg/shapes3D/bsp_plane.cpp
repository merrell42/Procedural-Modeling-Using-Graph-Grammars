#include "bsp_plane.h"
#include "face.h"
#include "face_type.h"
#include "endpoint.h"

namespace ms {

BspPlane::BspPlane(const Vec3& n, float dist)
    : normal(n)
    , d(dist) {}

BspPlane BspPlane::create(Face* face) {
    auto* endpoint = face->getEndpoints()[0];
    Vec3 n = endpoint->getFaceType()->getNormal();
    float d = n.dot(endpoint->getPosition());
    return BspPlane(n, d);
}

bool BspPlane::isParallel(const BspPlane& planeB) const {
    return std::abs(normal.dot(planeB.getNormal())) > 1.0f - PARALLEL_EPS;
}

bool BspPlane::sameD(const BspPlane& planeB) const {
    return std::abs(d - planeB.d) < COPLANAR_EPS;
}

int BspPlane::sign(const Vec3& query) const {
    float dist = normal.dot(query);
    if (dist > d + COPLANAR_EPS) {
        return 1;
    } else if (dist > d - COPLANAR_EPS) {
        return 0;
    } else {
        return -1;
    }
}

Vec3 BspPlane::crossingPoint(const Vec3& p0, const Vec3& p1) const {
    float d0 = normal.dot(p0);
    float d1 = normal.dot(p1);
    float s = (d - d0) / (d1 - d0);
    return Vec3::lerp(p0, p1, s);
}

} // namespace ms 