#pragma once
#include "../shape/vec3.h"

namespace ms {

	const float PLANE_EPSILON = 1e-6f;

    class Plane {
        public: 
            Vec3 normal;
            float d;
            Plane(Vec3 normal, float d) : normal(normal), d(d) {}
            Plane(Plane* plane) : normal(plane->normal), d(plane->d) {}
            bool isAbove(Vec3 point) {
                return normal.dot(point) > d + PLANE_EPSILON;
            }
            bool isBelow(Vec3 point) {
                return normal.dot(point) < d - PLANE_EPSILON;
            }
            Vec3 intersectLine(Vec3 p1, Vec3 p2) {
                float d1 = normal.dot(p1);
                float d2 = normal.dot(p2);
                float s = (d - d1) / (d2 - d1);
                return Vec3::lerp(p1, p2, s);
            }
    };
}
