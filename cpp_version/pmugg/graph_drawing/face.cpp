#include "pch.h"
#include "face.h"
#include "face_group.h"
#include "endpoint.h"
#include "../shapes3D/face_type3d.h"
#include "../util/util.h"
#include <vector>
#include <algorithm>
#include "../third_party/earcut/earcut.h"

namespace ms {

Face::Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds, bool looped, std::vector<int> bspNodeIds)
	: model(model)
	, id(id)
    , faceType(faceType)
    , endpointIds(endpointIds)
    , looped(looped)
    , bspNodeIds(bspNodeIds) {
    model->getCurrent()->addFace(id, this);
}

Face::Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds, std::vector<int> bspNodeIds)
    : model(model)
    , id(id)
    , faceType(faceType)
    , endpointIds(endpointIds)
    , bspNodeIds(bspNodeIds)
    , looped(false) {
    model->getCurrent()->addFace(id, this);
}

Face::~Face() {
    removeFromBsp();
}

Face* Face::copy() {
	auto result = new Face(model, id, faceType, endpointIds, looped, bspNodeIds);
	return result;
}

void Face::destroy() {
	model->getCurrent()->removeFace(this);
	delete this;
};

Endpoint* Face::getEndpoint(int index) const {
	return model->getCurrent()->getEndpoint(endpointIds[index]);
}

std::vector<Endpoint*> Face::getEndpoints() const {
	std::vector<Endpoint*> result;
	for (int i = 0; i < endpointIds.size(); i++) {
		result.push_back(getEndpoint(i));
	}
	return result;
}

Range Face::dirBounds(const Vec3& dir) const {
    double low = std::numeric_limits<double>::infinity();
    double high = -std::numeric_limits<double>::infinity();

    std::vector<Endpoint*> endpoints = getEndpoints();
    for (const Endpoint* endpoint : endpoints) {
        double d = dir.dot(endpoint->getPosition());
        low = min(d, low);
        high = max(d, high);
    }

    return Range((float)low, (float)high);
}

void Face::append(Face* faceB) {
    // this->dirty = true;

    // Check if the current object is the same as `faceB`
    if (this == faceB) {
        this->setLooped(true);
        return;
    }

    // Copy endpoints from `faceB`
    auto endpointsB = faceB->getEndpoints(); // Assuming `getEndpoints` returns a `std::vector<Endpoint*>`

    // Connect each endpoint to this face's node
    for (auto* endpointB : endpointsB) {
        endpointIds.push_back(endpointB->getId());
        endpointB->setFace(this);
    }

    // Destroy the node of `faceB`
    model->getCurrent()->removeFace(faceB);
}

std::vector<Vec3> Face::getPositions() const {
    std::vector<Vec3> positions; // Assuming Vec3 is the type for positions
    auto endpoints = getEndpoints(); // Get the endpoints

    for (const auto& endpoint : endpoints) {
        positions.push_back(endpoint->getPosition()); // Assuming getPosition returns a Vec3
    }

    return positions; // Return the vector of positions
}

std::vector<Vec2> Face::getPositions2D() const {
    auto positions = getPositions();
    auto maxDim = faceType->getMaxDim();
    std::vector<Vec2> positions2D;
    for (const auto& p : positions) {
        positions2D.push_back(p.dropDim(maxDim));
    }
    return positions2D;
}

FaceGroup* Face::getGroup() const {
    FaceGroup* group = new FaceGroup();
    group->addFace(const_cast<Face*>(this));
    return group;
}

void Face::split(Endpoint* endpoint) {
    std::vector<Endpoint*> endpoints = this->getEndpoints();
    auto index = std::find(endpoints.begin(), endpoints.end(), endpoint) - endpoints.begin();

    if (looped) {
        this->setLooped(false);
        std::vector<int> newOrder(endpointIds.begin() + index, endpointIds.end());
        newOrder.insert(newOrder.end(), endpointIds.begin(), endpointIds.begin() + index);
        endpointIds = newOrder;
    } else {
        if (index == 0) {
            // ms::alert("Should not be splitting off all of the endpoints.");
            return;
        }

        std::vector<Endpoint*> splitEndpoints(endpoints.begin() + index, endpoints.end());
        std::vector<int> splitEndpointIds(endpointIds.begin() + index, endpointIds.end());
        Face* newFace = new Face(model, model->newId(), faceType, splitEndpointIds, std::vector<int>());
        /* if (isHole()) {
            newFace->setHole(true);
        }

        bool insertAtStart = !this->isHole();
        if (insertAtStart) {
            this->getGroup()->getNode()->splice(newFace, 0);
        } else {
            this->getGroup()->getNode()->connect(newFace);
        } */

        for (auto& splitEndpoint : splitEndpoints) {
            splitEndpoint->setFace(newFace);
        }
        endpointIds.erase(endpointIds.begin() + index, endpointIds.end());
    }
}

void Face::insert(Endpoint* endpoint, Endpoint* prevEndpoint) {
    // Get the current endpoints of the face
    std::vector<Endpoint*> endpoints = this->getEndpoints();
    
    // Find the index of the previous endpoint
    auto it = std::find(endpoints.begin(), endpoints.end(), prevEndpoint);
    int index = (it != endpoints.end()) ? std::distance(endpoints.begin(), it) : -1;

    // Insert the new endpoint at the correct position
    int id = endpoint->getId();
    if (index >= 0) {
        endpointIds.insert(endpointIds.begin() + index + 1, id);
        endpoint->setFace(this);
    } else {
        endpointIds.insert(endpointIds.begin(), id);
        endpoint->setFace(this);
    }
}

void Face::removeEndpoint(Endpoint* endpoint) {
    Util::remove(endpointIds, endpoint->getId());
    if (endpointIds.size() == 0) {
        destroy();
    }
}

double Face::signedArea() {
    auto positions2D = getPositions2D();
    float sum = 0.0f;
    int n = positions2D.size();
    for (int i = 0; i < n; i++) {
        float xi = positions2D[i].x;
        float yp = positions2D[(i + 1) % n].y;
        float yn = positions2D[(i + n - 1) % n].y;
        sum += xi * (yn - yp);
    }

    return -sum / 2.0f;
}

void Face::exportMesh(
    std::vector<Vec3>& positions,
    std::vector<Vec3>& normals,
    std::vector<int>& triangles,
    std::vector<int>& faceIndices
) {
    int startIndex = positions.size();
    auto facePositions = getPositions();
    positions.insert(positions.end(), facePositions.begin(), facePositions.end());

    auto normal = faceType->getNormal();
    for (int i = 0; i < facePositions.size(); i++) {
        normals.push_back(normal);
    }
    auto indices = getTriangleIndices();
    if (indices.size() >= 3) {
        auto v10 = facePositions[indices[1]].copy().minus(facePositions[indices[0]]);
        auto v21 = facePositions[indices[2]].copy().minus(facePositions[indices[1]]);
        auto normalV = v10.cross(v21);
        auto isFlipped = (normal.dot(normalV) < 0);

        for (int i = 0; i < indices.size(); i += 3) {
            triangles.push_back(indices[i] + startIndex);
            if (isFlipped) {
                // Flip the order of the vertices to face outward.
                triangles.push_back(indices[i + 1] + startIndex);
                triangles.push_back(indices[i + 2] + startIndex);
            }
            else {
                triangles.push_back(indices[i + 2] + startIndex);
                triangles.push_back(indices[i + 1] + startIndex);
            }
        }
    }

    faceIndices.push_back(positions.size());
}

std::vector<int> Face::getTriangleIndices() {
    using Point = std::array<double, 2>;
    std::vector<std::vector<Point>> polygons;
    std::vector<Point> polygon;

    auto positions = getPositions();
    auto maxDim = faceType->getMaxDim();
    std::vector<std::array<double, 2>> positions2D;
    for (const auto& p : positions) {
        const Vec2 point = p.dropDim(maxDim);
        polygon.push_back({ point.x, point.y });
    }
    polygons.push_back(polygon);

    auto indices = mapbox::earcut(polygons);
    return std::vector<int>(indices.begin(), indices.end());
    /* auto endpoints = getEndpoints();
    std::vector<int> final;
    for (const auto& index : indices) {
        final.push_back(model->getCurrent()->getVertexIndex(endpoints[index]->getVertex()->getId()));
    }
    return final; */
}

void Face::removeFromBsp() {
    for (int bspNodeId : bspNodeIds) {
        model->getCurrent()->getBspNode(bspNodeId)->removeFace(this);
    }
    bspNodeIds.clear();
}

bool Face::addToBsp() {
    return model->getCurrent()->bspAddFace(this);
}

Plane Face::getPlane() const {
    Vec3 normal = faceType->getNormal();
    auto positions = getPositions();
    float d = normal.dot(positions[0]);
    return Plane(normal, d);
}

std::vector<Vec3> Face::getIntersections(Plane* plane) {
    auto positions = getPositions();
    int n = positions.size();
    std::vector<bool> isAbove(n);
    std::vector<bool> isBelow(n);
    for (int i = 0; i < n; i++) {
        isAbove[i] = plane->isAbove(positions[i]);
        isBelow[i] = plane->isBelow(positions[i]);
    }
    std::vector<Vec3> intersections;
    for (int i = 0; i < n; i++) {
        int i2 = (i + 1) % n;
        if ((isAbove[i] && isBelow[i2]) || (isBelow[i] && isAbove[i2])) {
            auto p1 = positions[i];
            auto p2 = positions[i2];
            auto intersection = plane->intersectLine(p1, p2);
            intersections.push_back(intersection);
        }
    }
    return intersections;
}

bool Face::containsPoint(Vec3 point) {
    int maxDim = faceType->getMaxDim();
    Vec2 point2D = point.dropDim(maxDim);
    auto positions = getPositions2D();
    int n = positions.size();

    std::vector<bool> isAbove(n);
    std::vector<bool> isBelow(n);
    float y = point2D.getY();
    for (int i = 0; i < n; i++) {
        isAbove[i] = positions[i].getY() > y;
        isBelow[i] = positions[i].getY() < y;
    }
    int count = 0;
    for (int i = 0; i < n; i++) {
        int i2 = (i + 1) % n;
        if ((isAbove[i] && isBelow[i2]) || (isBelow[i] && isAbove[i2])) {
            float x1 = positions[i].getX();
            float x2 = positions[i2].getX();
            float y1 = positions[i].getY();
            float y2 = positions[i2].getY();
            float s = (y - y1) / (y2 - y1);
            float x = x1 + (x2 - x1) * s;
            if (point2D.getX() < x) {
                count++;
            }
        }
    }
    return (count % 2) == 1;
}

}
