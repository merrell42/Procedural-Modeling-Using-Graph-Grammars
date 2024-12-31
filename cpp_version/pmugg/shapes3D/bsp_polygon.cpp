#include "bsp_polygon.h"
#include "face.h"
#include "node.h"
#include "single_property.h"
#include "intersector.h"
#include "endpoint.h"

namespace ms {

BspPolygon::BspPolygon(const std::vector<Vec3>& pts, Face* f)
    : points(pts) {
    auto stats = f->getNode()->getStats();
    
    properties.bspNode = std::make_unique<SingleProperty>("bspNode", false);
    properties.face = std::make_unique<SingleProperty>("face", true);
    
    node = std::make_unique<Node>(this, stats, "bspPolygon", properties);
    
    f->getNode()->connect(this);
}

Face* BspPolygon::getFace() const {
    return node->get("face");
}

bool BspPolygon::selfIntersects() const {
    for (size_t i = 0; i < points.size() - 1; i++) {
        const Vec3& a0 = points[i];
        const Vec3& a1 = points[i + 1];
        
        for (size_t j = i + 1; j < points.size() - 1; j++) {
            const Vec3& b0 = points[j];
            const Vec3& b1 = points[j + 1];
            
            if (Intersector::intersect3D(a0, a1, b0, b1)) {
                return true;
            }
        }
    }
    return false;
}

void BspPolygon::print() const {
    getFace()->print();
}

} // namespace ms 