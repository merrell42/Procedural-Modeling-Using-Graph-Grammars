#include "bsp_node.h"
#include "bsp_polygon.h"
#include "bsp_edge.h"
#include <algorithm>

namespace ms {

BspNode::BspNode(const Plane& p, const std::vector<BspPolygon*>& polys)
    : plane(p) {
    edges.resize(3, nullptr);
    for (auto* poly : polys) {
        addPolygon(poly);
    }
}

BspNode* BspNode::getAbove() const {
    return getChild(1);
}

BspNode* BspNode::getBelow() const {
    return getChild(2);
}

BspNode* BspNode::getChild(int index) const {
    if (index >= edges.size() || !edges[index]) return nullptr;
    return edges[index]->getChild();
}

void BspNode::setEdge(int index, BspEdge* edge) {
    if (index >= edges.size()) edges.resize(index + 1, nullptr);
    edges[index] = edge;
}

void BspNode::addPolygon(BspPolygon* polygon) {
    polygons.push_back(polygon);
}

void BspNode::onChanged() {
    if (polygons.empty() && !getAbove() && !getBelow()) {
        delete this;
    }
}

bool BspNode::add(const Plane& newPlane, BspPolygon* polygon, bool isRayCast) {
    if (!plane.isValid()) {
        plane = newPlane;
        addPolygon(polygon);
        return true;
    }
    return addNormal(newPlane, polygon, isRayCast);
}

bool BspNode::addNormal(const Plane& newPlane, BspPolygon* polygon, bool isRayCast) {
    const auto& points = polygon->getPoints();
    
    if (plane.isParallel(newPlane)) {
        if (plane.sameD(newPlane)) {
            if (isRayCast) return false;
            addPolygon(polygon);
            return true;
        }
        if (plane.sign(points[0]) == 1) {
            return addAbove(newPlane, polygon, isRayCast);
        }
        return addBelow(newPlane, polygon, isRayCast);
    }

    std::vector<int> signs;
    for (const auto& point : points) {
        signs.push_back(plane.sign(point));
    }

    int currentSign = 0;
    int firstSign = 0;
    std::vector<int> transitions;

    for (size_t i = 0; i < signs.size(); i++) {
        int s = signs[i];
        if (firstSign == 0) firstSign = s;
        if (currentSign == 0) currentSign = s;
        else if ((currentSign == 1 && s == -1) ||
                 (currentSign == -1 && s == 1)) {
            currentSign = s;
            transitions.push_back(i);
        }
    }

    if (currentSign != firstSign) {
        transitions.insert(transitions.begin(), 0);
    }

    if (transitions.empty()) {
        if (currentSign == 1) {
            return addAbove(newPlane, polygon, isRayCast);
        }
        return addBelow(newPlane, polygon, isRayCast);
    }

    if (transitions.size() % 2 != 0) {
        throw std::runtime_error("There should be an even number of transitions");
    }

    std::vector<Vec3> transitionPoints;
    int N = points.size();

    for (int index : transitions) {
        int prevIndex = (index + N - 1) % N;
        if (signs[index] == 0) {
            transitionPoints.push_back(points[index]);
        } else if (signs[prevIndex] == 0) {
            transitionPoints.push_back(points[prevIndex]);
        } else {
            transitionPoints.push_back(
                plane.crossingPoint(points[index], points[prevIndex]));
        }
    }

    Vec3 v = newPlane.getNormal().cross(plane.getNormal());
    auto* intersection = intersectsPolygon(transitionPoints, v, isRayCast);
    
    if (intersection) {
        if (isRayCast) return false;
        return false;
    }

    std::vector<Vec3> abovePoints;
    std::vector<Vec3> belowPoints;
    currentSign *= -1;

    for (size_t i = 0; i < transitions.size(); i++) {
        size_t i1 = (i + 1) % transitions.size();
        int index0 = transitions[i];
        int index1 = transitions[i1];
        if (index1 < index0) index1 += N;

        auto& pointsToAdd = (currentSign == 1) ? abovePoints : belowPoints;
        pointsToAdd.push_back(transitionPoints[i]);
        
        for (int j = index0; j < index1; j++) {
            pointsToAdd.push_back(points[j % N]);
        }
        pointsToAdd.push_back(transitionPoints[i1]);
        
        currentSign *= -1;
    }

    if (isRayCast) {
        return addAbove(newPlane, polygon, isRayCast) ||
               addBelow(newPlane, polygon, isRayCast);
    }

    auto* abovePolygon = new BspPolygon(abovePoints, polygon->getFace());
    auto* belowPolygon = new BspPolygon(belowPoints, polygon->getFace());
    
    return addAbove(newPlane, abovePolygon, isRayCast) &&
           addBelow(newPlane, belowPolygon, isRayCast);
}

bool BspNode::addAbove(const Plane& newPlane, BspPolygon* polygon, bool isRayCast) {
    auto* above = getAbove();
    if (above) {
        return above->addNormal(newPlane, polygon, isRayCast);
    }
    
    if (isRayCast) return false;

    auto* edge = new BspEdge();
    auto* aboveNode = new BspNode(newPlane, {polygon});
    edge->addNodes(this, aboveNode, true);
    return true;
}

bool BspNode::addBelow(const Plane& newPlane, BspPolygon* polygon, bool isRayCast) {
    auto* below = getBelow();
    if (below) {
        return below->addNormal(newPlane, polygon, isRayCast);
    }
    
    if (isRayCast) return false;

    auto* edge = new BspEdge();
    auto* belowNode = new BspNode(newPlane, {polygon});
    edge->addNodes(this, belowNode, false);
    return true;
}

BspPolygon* BspNode::intersectsPolygon(const std::vector<Vec3>& transitionPoints,
                                      const Vec3& v, bool isRayCast) const {
    if (!isRayCast) {
        // Check if any edges formed by transitionPoints intersect polygon edges
        for (size_t i = 0; i < transitionPoints.size(); i += 2) {
            const Vec3& a0 = transitionPoints[i];
            const Vec3& a1 = transitionPoints[i + 1];
            
            for (const auto* polygon : polygons) {
                const auto& points = polygon->getPoints();
                for (size_t j = 0; j < points.size(); j++) {
                    const Vec3& b0 = points[j];
                    const Vec3& b1 = points[(j + 1) % points.size()];
                    if (intersect3D(a0, a1, b0, b1)) {
                        return polygon;
                    }
                }
            }
        }
    }

    // Check if a point is inside any polygon
    if (!transitionPoints.empty()) {
        Vec3 rayEnd = transitionPoints[0] + v * 1e4;
        
        for (const auto* polygon : polygons) {
            const auto& points = polygon->getPoints();
            int intersectionCount = 0;
            
            for (size_t j = 0; j < points.size(); j++) {
                const Vec3& b0 = points[j];
                const Vec3& b1 = points[(j + 1) % points.size()];
                if (intersect3D(transitionPoints[0], rayEnd, b0, b1)) {
                    intersectionCount++;
                }
            }
            
            if (intersectionCount % 2 == 1) {
                return polygon;
            }
        }
    }

    return nullptr;
}

} // namespace ms 