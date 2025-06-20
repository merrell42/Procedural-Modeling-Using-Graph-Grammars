#pragma once
#include "vec3.h"

const double PLANE_EPSILON = 1e-6f;

class Plane {
    public: 
        Vec3 normal;
        double d;
        Plane(Vec3 normal, double d) : normal(normal), d(d) {}
        Plane(Plane* plane) : normal(plane->normal), d(plane->d) {}
        bool isAbove(Vec3 point) const {
            return normal.dot(point) > d + PLANE_EPSILON;
        }
        bool isBelow(Vec3 point) const {
            return normal.dot(point) < d - PLANE_EPSILON;
        }
        Vec3 intersectEdge(Vec3 p1, Vec3 p2) const {
            double d1 = normal.dot(p1);
            double d2 = normal.dot(p2);
            double s = (d - d1) / (d2 - d1);
            return Vec3::lerp(p1, p2, s);
        }
};

