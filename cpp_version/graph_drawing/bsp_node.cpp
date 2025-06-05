#include "pch.h"
#include "bsp_node.h"

namespace ms {
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

	BspNode* BspNode::copy() {
        auto newPlane = plane ? new Plane(*plane) : nullptr;
		return new BspNode(model, id, parentId, aboveId, belowId, newPlane, faceIds, edgeIds);
	}

    bool BspNode::addEdge(Edge* edge) {
        if (edgeIds.size() > 0) {
            auto thisEdge = model->getCurrent()->getEdge(edgeIds[0]);
            auto halfedges = thisEdge->getHalfEdges();
            auto p0 = halfedges[0]->getPosition();
            auto p1 = halfedges[1]->getPosition();
        }

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
                // This could be made slightly more efficient as we already know the edge is on both sides.
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

    bool BspNode::addFace(Face* face) {
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

    void BspNode::deleteIfEmpty() {
        if (edgeIds.size() > 0 || faceIds.size() > 0) {
            return;
        }
        if (aboveId == -1 && belowId == -1) {
            auto current = model->getCurrent();
            current->removeBspNode(this);
            if (parentId == -1) {
                current->setBspRootId(-1);
                delete this;
                return;
            }
            auto parent = current->getBspNode(parentId);
            if (parent->aboveId == id) {
                parent->aboveId = -1;
            }
            if (parent->belowId == id) {
                parent->belowId = -1;
            }
            parent->deleteIfEmpty();
        } else if (aboveId == -1 || belowId == -1) {
            auto current = model->getCurrent();
            current->removeBspNode(this);

            // Promote the child to be a child of the parent.
            int childId = aboveId == -1 ? belowId : aboveId;
            auto child = current->getBspNode(childId);
            child->parentId = parentId;
            if (parentId == -1) {
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

    void BspNode::connectAbove(BspNode* above) {
        aboveId = above->getId();
        above->setParentId(id);
    }

    void BspNode::connectBelow(BspNode* below) {
        belowId = below->getId();
        below->setParentId(id);
    }

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

    void BspNode::connectFace(Face* face) {
        faceIds.push_back(face->getId());
        face->addBspNodeId(id);
        if (!plane) {
            plane = new Plane(face->getPlane());
        }
    }

    PlaneClassification BspNode::classifyEdge(Edge* edge) {
        if (!plane) {
            return PlaneClassification::ON_PLANE;
        }
        auto halfedges = edge->getHalfEdges();
        auto p0 = halfedges[0]->getPosition();
        auto p1 = halfedges[1]->getPosition();
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

    PlaneClassification BspNode::classifyFace(Face* face) {
        if (faceIds.size() == 0) {
            return PlaneClassification::ON_PLANE;
        }
        auto positions = face->getPositions();
        bool isAbove = false;
        bool isBelow = false;
        for (auto position : positions) {
            if (isPointAbovePlane(position)) {
                isAbove = true;
                break;
            }
        }
        for (auto position : positions) {
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

    bool BspNode::hasFaceIntersection(Face* faceA) {
        auto current = model->getCurrent();
        // TODO: It might be more efficient to first check if the planes are parallel.
        // We also ignore cases where the plane exactly slices an edge of the face.
        auto intersections = faceA->getIntersections(plane);
        for (int faceId : faceIds) {
            Face* faceB = current->getFace(faceId);
            for (Vec3 intersection : intersections) {
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
}