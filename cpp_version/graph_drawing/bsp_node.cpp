#include "pch.h"
#include "bsp_node.h"

BspNode::BspNode(Model* model, int id) :
    model(model),
    id(id),
    parentId(-1),
    aboveId(-1),
    belowId(-1),
    faceIds(),
    edgeIds(),
    plane(nullptr) {
    model->getCurrent()->addBspNode(id, this);
}

BspNode::BspNode(Model* model, int id, int parentId, int aboveId, int belowId, Plane* plane, vector<int> faceIds, vector<int> edgeIds) :
    model(model),
    id(id),
    parentId(parentId),
    aboveId(aboveId),
    belowId(belowId),
    faceIds(faceIds),
    edgeIds(edgeIds),
    plane(plane) {
    model->getCurrent()->addBspNode(id, this);
}

BspNode::~BspNode() {
    delete plane;
}

int BspNode::getId() {
    return id;
}

BspNode* BspNode::copy() {
    auto newPlane = plane ? new Plane(*plane) : nullptr;
	return new BspNode(model, id, parentId, aboveId, belowId, newPlane, faceIds, edgeIds);
}

void BspNode::setParentId(int newParentId) {
    parentId = newParentId;
}

// Returns true, if the edge does not intersect any other edges.
bool BspNode::addEdge(Edge* edge) {
    // If this node has no edges, add it here.
    if (edgeIds.size() > 0) {
        auto thisEdge = model->getCurrent()->getEdge(edgeIds[0]);
        auto halfEdges = thisEdge->getHalfEdges();
        auto p0 = halfEdges[0]->getPosition();
        auto p1 = halfEdges[1]->getPosition();
    }

    // Based on the plane, add the edge to one of the two child nodes or both.
    PlaneClassification classification = classifyEdge(edge);
    switch (classification) {
        case PlaneClassification::ON_PLANE:
            connectEdge(edge);
            return true;
        case PlaneClassification::ABOVE: {
            BspNode* above = getAboveNode();
            return above->addEdge(edge);
        }
        case PlaneClassification::BELOW: {
            BspNode* below = getBelowNode();
            return below->addEdge(edge);
        }
        case PlaneClassification::BOTH: {
            // hasEdgeIntersection could be made slightly more efficient as we already know the edge is on both sides.
            if (hasEdgeIntersection(edge)) {
                return false;
            }
            BspNode* above = getAboveNode();
            BspNode* below = getBelowNode();
            return above->addEdge(edge) && below->addEdge(edge);
        }
        default:
            return false;
    }
}

// Returns true, if the face does not intersect any other faces.
bool BspNode::addFace(Face* face) {
    // Based on the plane, add the face here or to one of the two child nodes or both.
    PlaneClassification classification = classifyFace(face);
    switch (classification) {
        case PlaneClassification::ON_PLANE:
            connectFace(face);
            return true;
        case PlaneClassification::ABOVE: {
            BspNode* above = getAboveNode();
            return above->addFace(face);
        }
        case PlaneClassification::BELOW: {
            BspNode* below = getBelowNode();
            return below->addFace(face);
        }
        case PlaneClassification::BOTH: {
            if (hasFaceIntersection(face)) {
                return false;
            }
            BspNode* above = getAboveNode();
            BspNode* below = getBelowNode();
            return above->addFace(face) && below->addFace(face);
        }
        default:
            return false;
    }
}

void BspNode::removeEdge(Edge* edge) {
    edgeIds.erase(remove(edgeIds.begin(), edgeIds.end(), edge->getId()), edgeIds.end());
    deleteIfEmpty();
}

void BspNode::removeFace(Face* face) {
    faceIds.erase(remove(faceIds.begin(), faceIds.end(), face->getId()), faceIds.end());
    deleteIfEmpty();
}

// Delete nodes if they are empty.
void BspNode::deleteIfEmpty() {
    // Return if not empty.
    if (edgeIds.size() > 0 || faceIds.size() > 0) {
        return;
    }

    // If this has no children, delete the node.
    if (aboveId == -1 && belowId == -1) {
        auto current = model->getCurrent();
        current->removeBspNode(this);

        // If this is the root of the tree, delete the tree.
        if (parentId == -1) {
            current->setBspRootId(-1);
            delete this;
            return;
        }

        // Otherwise, remove it from its parent.
        auto parent = current->getBspNode(parentId);
        if (parent->aboveId == id) {
            parent->aboveId = -1;
        }
        if (parent->belowId == id) {
            parent->belowId = -1;
        }
        parent->deleteIfEmpty();
    } else if (aboveId == -1 || belowId == -1) {
        // If one child is present, delete this node and promote the child to be a
        // child of the grandparent.
        auto current = model->getCurrent();
        current->removeBspNode(this);

        int childId = aboveId == -1 ? belowId : aboveId;
        auto child = current->getBspNode(childId);
        child->parentId = parentId;
        if (parentId == -1) {
            // Handle root case.
            current->setBspRootId(childId);
        } else {
            auto parent = current->getBspNode(parentId);
            if (parent->aboveId == id) {
                parent->aboveId = childId;
            } else if (parent->belowId == id) {
                parent->belowId = childId;
            }
        }
        delete this;
    }
}

// Connect an above child to this node.
void BspNode::connectAbove(BspNode* above) {
    aboveId = above->getId();
    above->setParentId(id);
}

// Connect a below child to this node.
void BspNode::connectBelow(BspNode* below) {
    belowId = below->getId();
    below->setParentId(id);
}

// Connect an edge to this node.
void BspNode::connectEdge(Edge* edge) {
    edgeIds.push_back(edge->getId());
    edge->addBspNodeId(id);
    if (!plane) {
        auto edge = model->getCurrent()->getEdge(edgeIds[0]);
        auto normal = edge->getDirection()->cross(Vec3(0, 0, 1));
        auto d = edge->getHalfEdge(0)->getPosition().dot(normal);
        plane = new Plane(normal, d);
    }
}

// Connect a face to this node.
void BspNode::connectFace(Face* face) {
    faceIds.push_back(face->getId());
    face->addBspNodeId(id);
    if (!plane) {
        plane = new Plane(face->getPlane());
    }
}

// Classify an edge based on the plane.
PlaneClassification BspNode::classifyEdge(Edge* edge) {
    if (!plane) {
        return PlaneClassification::ON_PLANE;
    }
    auto halfEdges = edge->getHalfEdges();
    auto p0 = halfEdges[0]->getPosition();
    auto p1 = halfEdges[1]->getPosition();
    bool isAbove = (isPointAbovePlane(p0) || isPointAbovePlane(p1));
    bool isBelow = (isPointBelowPlane(p0) || isPointBelowPlane(p1));
    if (isAbove && isBelow) {
        return PlaneClassification::BOTH;
    }
    if (isAbove) {
        return PlaneClassification::ABOVE;
    }
    if (isBelow) {
        return PlaneClassification::BELOW;
    }
    return PlaneClassification::ON_PLANE;
}

// Classify a face based on the plane.
PlaneClassification BspNode::classifyFace(Face* face) {
    if (faceIds.size() == 0) {
        return PlaneClassification::ON_PLANE;
    }
    auto positions = face->getPositions();
    bool isAbove = false;
    bool isBelow = false;
    for (const auto& position : positions) {
        if (isPointAbovePlane(position)) {
            isAbove = true;
            break;
        }
    }
    for (const auto& position : positions) {
        if (isPointBelowPlane(position)) {
            isBelow = true;
            break;
        }
    }
    if (isAbove && isBelow) {
        return PlaneClassification::BOTH;
    }
    if (isAbove) {
        return PlaneClassification::ABOVE;
    }
    if (isBelow) {
        return PlaneClassification::BELOW;
    }
    return PlaneClassification::ON_PLANE;
}

Plane* BspNode::getPlane() {
    return plane;
}

// Get the above child node. Create it if it doesn't exist.
BspNode* BspNode::getAboveNode() {
    auto current = model->getCurrent();
    if (aboveId == -1) {
        int id = model->newId();
        BspNode* above = new BspNode(model, id);
        connectAbove(above);
        current->addBspNode(id, above);
        aboveId = id;
        return above;
    }
    return current->getBspNode(aboveId);
}

// Get the below child node. Create it if it doesn't exist.
BspNode* BspNode::getBelowNode() {
    auto current = model->getCurrent();
    if (belowId == -1) {
        int id = model->newId();
        BspNode* below = new BspNode(model, id);
        connectBelow(below);
        current->addBspNode(id, below); 
        belowId = id;
        return below;
    }
    return current->getBspNode(belowId);
}

// Detect edge intersections.
bool BspNode::hasEdgeIntersection(Edge* edgeA) {
    auto current = model->getCurrent();
    for (int edgeId : edgeIds) {
        Edge* edgeB = current->getEdge(edgeId);
        if (edgeA->intersects(edgeB)) {
            return true;
        }
    }
    return false;
}

// Detect face intersections.
bool BspNode::hasFaceIntersection(Face* faceA) {
    auto current = model->getCurrent();
    // TODO: It might be more efficient to first check if the planes are parallel.
    // We also ignore cases where the plane exactly slices an edge of the face.
    auto intersections = faceA->getIntersections(plane);
    for (int faceId : faceIds) {
        Face* faceB = current->getFace(faceId);
        for (const Vec3& intersection : intersections) {
            if (faceB->containsPoint(intersection)) {
                return true;
            }
        }
    }
    return false;
}

bool BspNode::isPointAbovePlane(Vec3 point) {
    return plane->isAbove(point);
}

bool BspNode::isPointBelowPlane(Vec3 point) {
    return plane->isBelow(point);
}
