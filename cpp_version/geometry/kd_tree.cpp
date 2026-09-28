#include "pch.h"
#include "kd_tree.h"
#include <cmath>
#include <limits>

namespace {

constexpr int KD_LEAF_SIZE = 8;
constexpr double ON_SURFACE_EPSILON = 1e-8;

double axisValue(const Vec3& point, int axis) {
    return point.getValue(axis);
}

}  // namespace

Aabb::Aabb()
    : min(
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::infinity(),
        false
    )
    , max(
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        false
    ) {
}

Aabb::Aabb(const Vec3& point)
    : min(point.getX(), point.getY(), point.getZ(), false)
    , max(point.getX(), point.getY(), point.getZ(), false) {
}

void Aabb::expand(const Vec3& point) {
    min = Vec3(
        std::min(min.getX(), point.getX()),
        std::min(min.getY(), point.getY()),
        std::min(min.getZ(), point.getZ()),
        false
    );
    max = Vec3(
        std::max(max.getX(), point.getX()),
        std::max(max.getY(), point.getY()),
        std::max(max.getZ(), point.getZ()),
        false
    );
}

void Aabb::expand(const Aabb& other) {
    expand(other.min);
    expand(other.max);
}

double Aabb::minDistance(const Aabb& other) const {
    double dx = 0.0;
    if (max.getX() < other.min.getX()) {
        dx = other.min.getX() - max.getX();
    } else if (other.max.getX() < min.getX()) {
        dx = min.getX() - other.max.getX();
    }

    double dy = 0.0;
    if (max.getY() < other.min.getY()) {
        dy = other.min.getY() - max.getY();
    } else if (other.max.getY() < min.getY()) {
        dy = min.getY() - other.max.getY();
    }

    double dz = 0.0;
    if (max.getZ() < other.min.getZ()) {
        dz = other.min.getZ() - max.getZ();
    } else if (other.max.getZ() < min.getZ()) {
        dz = min.getZ() - other.max.getZ();
    }

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double Aabb::minDistanceToPlane(const Vec3& normal, double planeD) const {
    Vec3 center(
        0.5 * (min.getX() + max.getX()),
        0.5 * (min.getY() + max.getY()),
        0.5 * (min.getZ() + max.getZ()),
        false
    );
    double hx = 0.5 * (max.getX() - min.getX());
    double hy = 0.5 * (max.getY() - min.getY());
    double hz = 0.5 * (max.getZ() - min.getZ());
    double centerDist = std::abs(normal.dot(center) - planeD);
    double extent =
        std::abs(normal.getX()) * hx +
        std::abs(normal.getY()) * hy +
        std::abs(normal.getZ()) * hz;
    return std::max(0.0, centerDist - extent);
}

void SampleKdTree::clear() {
    root.reset();
}

void SampleKdTree::build(const vector<Point>& points) {
    clear();
    if (points.empty()) {
        return;
    }
    vector<Point> copy = points;
    root = buildNode(copy, 0);
}

unique_ptr<SampleKdTree::Node> SampleKdTree::buildNode(vector<Point>& points, int depth) {
    auto node = std::make_unique<Node>();
    if (points.empty()) {
        return node;
    }

    node->aabb = Aabb(points[0].position);
    for (size_t i = 1; i < points.size(); i++) {
        node->aabb.expand(points[i].position);
    }

    if (static_cast<int>(points.size()) <= KD_LEAF_SIZE) {
        node->points = std::move(points);
        return node;
    }

    int axis = depth % 3;
    size_t mid = points.size() / 2;
    std::nth_element(points.begin(), points.begin() + static_cast<int>(mid), points.end(),
        [axis](const Point& a, const Point& b) {
            return axisValue(a.position, axis) < axisValue(b.position, axis);
        });

    vector<Point> leftPoints(points.begin(), points.begin() + static_cast<int>(mid));
    vector<Point> rightPoints(points.begin() + static_cast<int>(mid), points.end());
    if (leftPoints.empty() || rightPoints.empty()) {
        node->points = std::move(points);
        return node;
    }

    node->left = buildNode(leftPoints, depth + 1);
    node->right = buildNode(rightPoints, depth + 1);
    return node;
}

bool SampleKdTree::skipBranch(const Aabb& aabb, const ProximityQuery& query) {
    double dMin = aabb.minDistance(query.bounds);
    dMin = std::max(dMin, aabb.minDistanceToPlane(query.planeNormal, query.planeD));
    if (query.decay <= 1e-12) {
        return dMin > ON_SURFACE_EPSILON;
    }
    return std::abs(query.intensity) * std::exp(-query.decay * dMin) < query.epsilon;
}

void SampleKdTree::visitNode(
    const Node* node,
    const ProximityQuery& query,
    const std::function<void(void*)>& onPoint
) {
    if (!node || skipBranch(node->aabb, query)) {
        return;
    }
    if (!node->left && !node->right) {
        for (const Point& point : node->points) {
            onPoint(point.payload);
        }
        return;
    }
    visitNode(node->left.get(), query, onPoint);
    visitNode(node->right.get(), query, onPoint);
}

void SampleKdTree::visit(const ProximityQuery& query, const std::function<void(void*)>& onPoint) const {
    visitNode(root.get(), query, onPoint);
}
