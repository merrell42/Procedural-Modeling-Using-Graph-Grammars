#pragma once
#include <vector>
#include <memory>
#include "vec3.h"

namespace ms {

class Face;
class Node;
class SingleProperty;

class BspPolygon {
public:
    BspPolygon(const std::vector<Vec3>& points, Face* face);
    ~BspPolygon() = default;

    // Core accessors
    const std::vector<Vec3>& getPoints() const { return points; }
    Face* getFace() const;
    Node* getNode() const { return node.get(); }

    // Operations
    bool selfIntersects() const;
    void print() const;

private:
    std::vector<Vec3> points;
    std::unique_ptr<Node> node;

    struct Properties {
        std::unique_ptr<SingleProperty> bspNode;
        std::unique_ptr<SingleProperty> face;
    };
    Properties properties;
};

} // namespace ms 