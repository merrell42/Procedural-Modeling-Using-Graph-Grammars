#pragma once
#include <vector>
#include <memory>
#include "plane.h"

namespace ms {

class BspPolygon;
class BspEdge;

class BspNode {
public:
    BspNode(const Plane& plane, const std::vector<BspPolygon*>& polygons);
    ~BspNode() = default;

    // Core accessors
    const std::vector<BspPolygon*>& getPolygons() const { return polygons; }
    BspNode* getAbove() const;
    BspNode* getBelow() const;
    const Plane& getPlane() const { return plane; }

    // Polygon operations
    void addPolygon(BspPolygon* polygon);
    void onChanged();

    // BSP operations
    bool add(const Plane& newPlane, BspPolygon* polygon, bool isRayCast = false);
    bool addNormal(const Plane& newPlane, BspPolygon* polygon, bool isRayCast = false);
    bool addAbove(const Plane& newPlane, BspPolygon* polygon, bool isRayCast = false);
    bool addBelow(const Plane& newPlane, BspPolygon* polygon, bool isRayCast = false);
    BspPolygon* intersectsPolygon(const std::vector<Vec3>& transitionPoints, 
                                 const Vec3& v, bool isRayCast = false) const;

private:
    std::vector<BspPolygon*> polygons;
    std::vector<BspEdge*> edges;  // [parent, above, below]
    Plane plane;

    // Helper methods
    BspNode* getChild(int index) const;
    void setEdge(int index, BspEdge* edge);
};

} // namespace ms 