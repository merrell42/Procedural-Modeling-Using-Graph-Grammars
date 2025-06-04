#include "pch.h"
#include "half_edge.h"

namespace ms {

// TODO: Rename as halfEdge.
HalfEdge::HalfEdge(Model* model, int id, bool isAtStart, EdgeType3D* edgeType, Vec3 dir, int vertexId, int faceId_, int edgeId, bool createFace, int faceIndex)
	: model(model)
	, id(id)
    , isAtStart(isAtStart)
    , edgeType(edgeType)
    , dir(dir)
	, vertexId(vertexId)
    , faceIndex(faceIndex)
	, faceId(faceId_)
	, edgeId(edgeId) {
    model->getCurrent()->addHalfEdge(id, this);

    faceTypeCached = nullptr;
    if (createFace) {
        auto faceType = getFaceType();
        std::vector<int> halfedgeIds;
        halfedgeIds.push_back(id);
        faceId = model->newId();
        std::vector<int> bspNodeIds;
        auto face = new Face(model, faceId, faceType, halfedgeIds, bspNodeIds);
        face->createGroup();
    }
}

FaceType3D* HalfEdge::getFaceType() {
    if (!faceTypeCached) {
        const auto& faceData = edgeType->faceData;
        faceTypeCached = faceData[faceIndex].type;
    }
    return faceTypeCached;
}

HalfEdge* HalfEdge::copy() {
	auto result = new HalfEdge(model, id, isAtStart, edgeType, dir, vertexId, faceId, edgeId, false, -1);
	return result;
}

void HalfEdge::destroy() {
    model->getCurrent()->removeHalfEdge(this);
    delete this;
};

EdgeType3D* HalfEdge::getEdgeType() const {
    return edgeType;
}

Vertex* HalfEdge::getVertex() const {
	return model->getCurrent()->getVertex(vertexId);
}

Face* HalfEdge::getFace() const {
	return model->getCurrent()->getFace(faceId);
}


Edge* HalfEdge::getEdge() const {
	return model->getCurrent()->getEdge(edgeId);
}

Vec3 HalfEdge::getPosition() const {
	return getVertex()->getPosition();
}

void HalfEdge::transfer(Edge* replacement) {
	int index = isAtStart ? 1 : 0;
    replacement->addHalfEdge(this, index);
}

HalfEdge* HalfEdge::next() const {
    std::vector<HalfEdge*> halfedges = getFace()->getHalfEdges();
    size_t N = halfedges.size();
    auto it = std::find(halfedges.begin(), halfedges.end(), this);
    size_t index = std::distance(halfedges.begin(), it);
    return halfedges[(index + 1) % N];
}

HalfEdge* HalfEdge::prev() const {
    std::vector<HalfEdge*> halfedges = getFace()->getHalfEdges();
    size_t index = std::find(halfedges.begin(), halfedges.end(), this) - halfedges.begin();

    if (index == 0) {
        size_t N = halfedges.size();
        // Wrap around to the last halfedge.
        index = N;
    }

    return halfedges[index - 1];
}

HalfEdge* HalfEdge::twin() const {
    Edge* edge = getEdge();
    if (edge) {
        std::vector<HalfEdge*> halfedges = edge->getHalfEdges();
        auto it = std::find(halfedges.begin(), halfedges.end(), this);
        if (it != halfedges.end()) {
            size_t index = std::distance(halfedges.begin(), it);
            // Return the other halfedge.
            return halfedges[1 - index];
        }
    }
    return nullptr;
}

void HalfEdge::setEdge(Edge* edge) {
    edgeId = edge->getId();
}

void HalfEdge::setFace(Face* face) {
    faceId = face->getId();
}

void HalfEdge::mergeFaces(HalfEdge* next) {
    getFace()->append(next->getFace());
}

void HalfEdge::maybeMergeNextFace() {
    HalfEdge* n = next();
    if (n) {
        this->getFace()->append(n->getFace());
    }
}

}