#include "pch.h"
#include "face.h"
#include "face_group.h"
#include "half_edge.h"
#include <vector>
#include <algorithm>
#include "../primitives/face_type.h"
#include "../third_party/earcut/earcut.h"
#include "../util/util.h"
#include "../util/timer.h"

Face::Face(Model* model, int id, FaceType* faceType, vector<int> halfEdgeIds, bool looped, vector<int> bspNodeIds, int groupId, bool hole)
	: model(model)
	, id(id)
    , faceType(faceType)
    , halfEdgeIds(halfEdgeIds)
    , looped(looped)
    , bspNodeIds(bspNodeIds)
    , groupId(groupId)
    , hole(hole) {
    MemoryCounter::creation("face");
    model->getCurrent()->addFace(id, this);
}

Face::Face(Model* model, int id, FaceType* faceType, vector<int> halfEdgeIds, vector<int> bspNodeIds)
    : model(model)
    , id(id)
    , faceType(faceType)
    , halfEdgeIds(halfEdgeIds)
    , bspNodeIds(bspNodeIds)
    , looped(false)
    , groupId(-1)
    , hole(false) {
    MemoryCounter::creation("face");
    model->getCurrent()->addFace(id, this);
}

Face::~Face() {
    MemoryCounter::destruction("face");
}

Face* Face::copy() {
	auto result = new Face(model, id, faceType, halfEdgeIds, looped, bspNodeIds, groupId, hole);
	return result;
}

void Face::destroy() {
	model->getCurrent()->removeFace(this);
    auto group = getGroup();
    if (group) {
        group->removeFace(this);
        group->destroyIfEmpty();
    }
    removeFromBsp();
	delete this;
};

HalfEdge* Face::getHalfEdge(int index) const {
	return model->getCurrent()->getHalfEdge(halfEdgeIds[index]);
}

vector<HalfEdge*> Face::getHalfEdges() const {
	vector<HalfEdge*> result;
	for (int i = 0; i < halfEdgeIds.size(); i++) {
		result.push_back(getHalfEdge(i));
	}
	return result;
}

Range Face::dirBounds(const Vec3& dir) const {
    double low = numeric_limits<double>::infinity();
    double high = -numeric_limits<double>::infinity();

    vector<HalfEdge*> halfEdges = getHalfEdges();
    for (const HalfEdge* halfEdge : halfEdges) {
        double d = dir.dot(halfEdge->getPosition());
        low = min(d, low);
        high = max(d, high);
    }

    return Range((double)low, (double)high);
}

void Face::append(Face* faceB) {
    // Set looped if appending the same face.
    if (this == faceB) {
        this->setLooped(true);
        return;
    }

    // Attach all of faceB's halfEdges to this face.
    auto halfEdgesB = faceB->getHalfEdges();
    for (auto* halfEdgeB : halfEdgesB) {
        halfEdgeIds.push_back(halfEdgeB->getId());
        halfEdgeB->setFace(this);
    }
    // Destroy faceB.
    model->getCurrent()->removeFace(faceB);
    faceB->destroy();
}

vector<Vec3> Face::getPositions() const {
    vector<Vec3> positions;
    auto halfEdges = getHalfEdges();
    for (const auto& halfEdge : halfEdges) {
        positions.push_back(halfEdge->getPosition());
    }
    return positions;
}

// Remove one of the dimensions.
vector<Vec2> Face::getPositions2D() const {
    auto positions = getPositions();
    auto maxDim = faceType->getMaxDim();
    vector<Vec2> positions2D;
    for (const auto& p : positions) {
        positions2D.push_back(p.dropDim(maxDim));
    }
    return positions2D;
}

void Face::setGroup(FaceGroup* group) {
    groupId = group->getId();
}

FaceGroup* Face::getGroup() const {
    return model->getCurrent()->getFaceGroup(groupId);
}

void Face::split(HalfEdge* halfEdge) {
    vector<HalfEdge*> halfEdges = getHalfEdges();
    auto index = find(halfEdges.begin(), halfEdges.end(), halfEdge) - halfEdges.begin();

    // If looped, cut the loop at the halfEdge and keep this as one face starting there.
    if (looped) {
        setLooped(false);
        vector<int> newOrder(halfEdgeIds.begin() + index, halfEdgeIds.end());
        newOrder.insert(newOrder.end(), halfEdgeIds.begin(), halfEdgeIds.begin() + index);
        halfEdgeIds = newOrder;
    } else {
        if (index == 0) {
            cout << "Should not be splitting off all of the halfEdges." << endl;
            return;
        }

        // If not looped, cut off the end of the face.
        // Starting at index create a new face. Move all halfEdge past index to the new face.
        vector<HalfEdge*> splitHalfEdges(halfEdges.begin() + index, halfEdges.end());
        vector<int> splitHalfEdgeIds(halfEdgeIds.begin() + index, halfEdgeIds.end());
        Face* newFace = new Face(model, model->newId(), faceType, splitHalfEdgeIds, vector<int>());
        for (auto& splitHalfEdge : splitHalfEdges) {
            splitHalfEdge->setFace(newFace);
        }
        halfEdgeIds.erase(halfEdgeIds.begin() + index, halfEdgeIds.end());
        if (isHole()) {
            newFace->setHole(true);
        }

        // Add it to the existing face group.
        bool insertAtStart = !isHole();
        if (insertAtStart) {
            getGroup()->insertFace(newFace, 0);
        } else {
            getGroup()->addFace(newFace);
        }
    }
}

void Face::insert(HalfEdge* halfEdge, HalfEdge* prevHalfEdge) {
    vector<HalfEdge*> halfEdges = this->getHalfEdges();

    // Find the insertion index. It is inserted after prevHalfEdge.
    int prevId = prevHalfEdge->getId();
    size_t index = -1;
    for (size_t i = 0; i < halfEdges.size(); i++) {
        if (halfEdges[i]->getId() == prevId) {
            index = i;
            break;
        }
    }

    int id = halfEdge->getId();
    halfEdgeIds.insert(halfEdgeIds.begin() + index + 1, id);
    halfEdge->setFace(this);
}

void Face::removeHalfEdge(HalfEdge* halfEdge) {
    Util::remove(halfEdgeIds, halfEdge->getId());
    if (halfEdgeIds.size() == 0) {
        destroy();
    }
}

// Find the signed area using the shoelace formula.
double Face::signedArea() const {
    auto positions2D = getPositions2D();
    double sum = 0.0f;
    size_t n = positions2D.size();
    for (size_t i = 0; i < n; i++) {
        double xi = positions2D[i].x;
        double yp = positions2D[(i + 1) % n].y;
        double yn = positions2D[(i + n - 1) % n].y;
        sum += xi * (yn - yp);
    }
    return -sum / 2.0f;
}

void Face::exportMesh(
    vector<Vec3>& positions,
    vector<Vec3>& normals,
    vector<int>& triangles,
    vector<int>& faceIndices
) const {
    int startIndex = (int)positions.size();
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
            } else {
                triangles.push_back(indices[i + 2] + startIndex);
                triangles.push_back(indices[i + 1] + startIndex);
            }
        }
    }

    faceIndices.push_back((int)positions.size());
}

vector<int> Face::getTriangleIndices() const {
    using Point = array<double, 2>;
    vector<vector<Point>> polygons;
    vector<Point> polygon;

    auto positions = getPositions();
    auto maxDim = faceType->getMaxDim();
    vector<array<double, 2>> positions2D;
    for (const auto& p : positions) {
        const Vec2 point = p.dropDim(maxDim);
        polygon.push_back({ point.x, point.y });
    }
    polygons.push_back(polygon);

    auto indices = mapbox::earcut(polygons);
    return vector<int>(indices.begin(), indices.end());
}

void Face::removeFromBsp() {
    for (int bspNodeId : bspNodeIds) {
        model->getCurrent()->getBspNode(bspNodeId)->removeFace(this);
    }
    bspNodeIds.clear();
}

bool Face::addToBsp() {
    timer->start("AddToBsp");
    bool success = model->getCurrent()->bspAddFace(this);
    timer->stop("AddToBsp");
    return success;
}

Plane Face::getPlane() const {
    Vec3 normal = faceType->getNormal();
    auto positions = getPositions();
    double d = normal.dot(positions[0]);
    return Plane(normal, d);
}

void Face::createGroup() {
    groupId = model->newId();
    auto group = new FaceGroup(model, groupId);
    group->addFace(this);
}

void Face::splitGroup() {
    auto group = getGroup();
    group->removeFace(this);
    createGroup();
}

vector<Vec3> Face::getIntersections(Plane* plane) const {
    auto positions = getPositions();
    auto n = positions.size();
    vector<bool> isAbove(n);
    vector<bool> isBelow(n);
    for (int i = 0; i < n; i++) {
        isAbove[i] = plane->isAbove(positions[i]);
        isBelow[i] = plane->isBelow(positions[i]);
    }
    vector<Vec3> intersections;
    for (int i = 0; i < n; i++) {
        int i2 = (i + 1) % n;
        if ((isAbove[i] && isBelow[i2]) || (isBelow[i] && isAbove[i2])) {
            auto p1 = positions[i];
            auto p2 = positions[i2];
            auto intersection = plane->intersectEdge(p1, p2);
            intersections.push_back(intersection);
        }
    }
    return intersections;
}

bool Face::containsPoint(Vec3 point) const {
    int maxDim = faceType->getMaxDim();
    Vec2 point2D = point.dropDim(maxDim);
    auto positions = getPositions2D();
    auto n = positions.size();

    vector<bool> isAbove(n);
    vector<bool> isBelow(n);
    double y = point2D.getY();
    for (int i = 0; i < n; i++) {
        isAbove[i] = positions[i].getY() > y;
        isBelow[i] = positions[i].getY() < y;
    }
    int count = 0;
    for (int i = 0; i < n; i++) {
        int i2 = (i + 1) % n;
        if ((isAbove[i] && isBelow[i2]) || (isBelow[i] && isAbove[i2])) {
            double x1 = positions[i].getX();
            double x2 = positions[i2].getX();
            double y1 = positions[i].getY();
            double y2 = positions[i2].getY();
            double s = (y - y1) / (y2 - y1);
            double x = x1 + (x2 - x1) * s;
            if (point2D.getX() < x) {
                count++;
            }
        }
    }
    return (count % 2) == 1;
}

void Face::setLooped(bool newLooped) {
    looped = newLooped;
}

void Face::addBspNodeId(int bspNodeId) {
    bspNodeIds.push_back(bspNodeId);
}

void Face::setHole(bool newHole) {
    hole = newHole;
}
