#pragma once

#include "vec3.h"
#include <functional>
#include <memory>
#include <vector>

struct Aabb {
    Vec3 min;
    Vec3 max;

    Aabb();
    explicit Aabb(const Vec3& point);

    void expand(const Vec3& point);
    void expand(const Aabb& other);
    double minDistance(const Aabb& other) const;
    double minDistanceToPlane(const Vec3& normal, double planeD) const;
};

class SampleKdTree {
public:
    struct Point {
        Vec3 position;
        void* payload = nullptr;
    };

    struct ProximityQuery {
        Aabb bounds;
        Vec3 planeNormal;
        double planeD = 0.0;
        double decay = 0.0;
        double intensity = 0.0;
        double epsilon = 1e-4;
    };

    void build(const vector<Point>& points);
    void clear();

    void visit(const ProximityQuery& query, const std::function<void(void*)>& onPoint) const;

private:
    struct Node {
        Aabb aabb;
        unique_ptr<Node> left;
        unique_ptr<Node> right;
        vector<Point> points;
    };

    unique_ptr<Node> root;

    static unique_ptr<Node> buildNode(vector<Point>& points, int depth);
    static bool skipBranch(const Aabb& aabb, const ProximityQuery& query);
    static void visitNode(const Node* node, const ProximityQuery& query, const std::function<void(void*)>& onPoint);
};
