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
        lineIds(),
        plane(nullptr) {
        model->getCurrent()->addBspNode(id, this);
	}

    BspNode::BspNode(Model* model, int id, int parentId, int aboveId, int belowId, Plane* plane, std::vector<int> faceIds, std::vector<int> lineIds) :
        model(model),
        id(id),
        parentId(parentId),
        aboveId(aboveId),
        belowId(belowId),
        faceIds(faceIds),
        lineIds(lineIds),
        plane(plane) {
        model->getCurrent()->addBspNode(id, this);
	}

	BspNode* BspNode::copy() {
        auto newPlane = plane ? new Plane(*plane) : nullptr;
		return new BspNode(model, id, parentId, aboveId, belowId, newPlane, faceIds, lineIds);
	}

    bool BspNode::addLine(Line* line) {
        if (lineIds.size() > 0) {
            auto thisLine = model->getCurrent()->getLine(lineIds[0]);
            auto endpoints = thisLine->getEndpoints();
            auto p0 = endpoints[0]->getPosition();
            auto p1 = endpoints[1]->getPosition();
        }

        PlaneClassification classification = classifyLine(line);
        switch (classification) {
            case PlaneClassification::ON_PLANE:
                connectLine(line);
                return true;
            case PlaneClassification::ABOVE: {
                BspNode* above = getAboveNode();
                return above->addLine(line);
            }
            case PlaneClassification::BELOW: {
                BspNode* below = getBelowNode();
                return below->addLine(line);
            }
            case PlaneClassification::BOTH: {
                // This could be made slightly more efficient as we already know the line is on both sides.
                if (hasLineIntersection(line)) {
                    return false;
                }
                BspNode* above = getAboveNode();
                BspNode* below = getBelowNode();
                return above->addLine(line) && below->addLine(line);
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
                BspNode* above = getAboveNode();
                BspNode* below = getBelowNode();
                return above->addFace(face) && below->addFace(face);
            }
            default:
                return false;
        }
    }

    void BspNode::removeLine(Line* line) {
        lineIds.erase(std::remove(lineIds.begin(), lineIds.end(), line->getId()), lineIds.end());
        deleteIfEmpty();
    }

    void BspNode::removeFace(Face* face) {
        faceIds.erase(std::remove(faceIds.begin(), faceIds.end(), face->getId()), faceIds.end());
        deleteIfEmpty();
    }

    void BspNode::deleteIfEmpty() {
        if (lineIds.size() > 0 || faceIds.size() > 0) {
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

    void BspNode::connectLine(Line* line) {
        lineIds.push_back(line->getId());
        line->addBspNodeId(id);
        if (!plane) {
            auto line = model->getCurrent()->getLine(lineIds[0]);
            auto normal = line->getDirection()->cross(Vec3(0, 0, 1));
            auto d = line->getEndpoint(0)->getPosition().dot(normal);
            plane = new Plane(normal, d);
        }
    }

    void BspNode::connectFace(Face* face) {
        faceIds.push_back(face->getId());
        face->setBspNodeId(id);
        if (!plane) {
            auto face = model->getCurrent()->getFace(faceIds[0]);
            auto normal =  face->getFaceType()->getNormal();
            auto d = face->getPositions()[0].dot(normal);
            plane = new Plane(normal, d);
        }
    }

    PlaneClassification BspNode::classifyLine(Line* line) {
        if (!plane) {
            return PlaneClassification::ON_PLANE;
        }
        auto endpoints = line->getEndpoints();
        auto p0 = endpoints[0]->getPosition();
        auto p1 = endpoints[1]->getPosition();
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

    bool BspNode::hasLineIntersection(Line* lineA) {
        auto current = model->getCurrent();
        for (int lineId : lineIds) {
            Line* lineB = current->getLine(lineId);
            if (lineA->intersects(lineB)) {
                return true;
            }
        }
        return false;
    }

    bool BspNode::isPointAbovePlane(Vec3 point) {
        Plane* plane = getPlane();
        return plane->normal.dot(point) > plane->d + PLANE_EPSILON;
    }

    bool BspNode::isPointBelowPlane(Vec3 point) {
        Plane* plane = getPlane();
        return plane->normal.dot(point) < plane->d - PLANE_EPSILON;
    }
}