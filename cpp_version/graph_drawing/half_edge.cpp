#include "pch.h"
#include "half_edge.h"

HalfEdge::HalfEdge(Model* model, int id, bool isAtStart, EdgeType* edgeType, Vec3 dir, int vertexId, int faceId_, int edgeId, bool createFace, int faceIndex)
	: model(model)
	, id(id)
    , isAtStart(isAtStart)
    , edgeType(edgeType)
    , dir(dir)
	, vertexId(vertexId)
    , faceIndex(faceIndex)
	, faceId(faceId_)
	, edgeId(edgeId) {
    MemoryCounter::halfEdgeCreated++;
    model->getCurrent()->addHalfEdge(id, this);

    faceTypeCached = nullptr;
    if (createFace) {
        auto faceType = getFaceType();
        vector<int> halfEdgeIds;
        halfEdgeIds.push_back(id);
        faceId = model->newId();
        vector<int> bspNodeIds;
        auto face = new Face(model, faceId, faceType, halfEdgeIds, bspNodeIds);
        face->createGroup();
    }
}

HalfEdge::~HalfEdge() {
	MemoryCounter::halfEdgeDestroyed++;
}

FaceType* HalfEdge::getFaceType() {
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

EdgeType* HalfEdge::getEdgeType() const {
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
    vector<HalfEdge*> halfEdges = getFace()->getHalfEdges();
    size_t N = halfEdges.size();
    auto it = find(halfEdges.begin(), halfEdges.end(), this);
    size_t index = distance(halfEdges.begin(), it);
    return halfEdges[(index + 1) % N];
}

HalfEdge* HalfEdge::prev() const {
    vector<HalfEdge*> halfEdges = getFace()->getHalfEdges();
    size_t index = find(halfEdges.begin(), halfEdges.end(), this) - halfEdges.begin();

    if (index == 0) {
        size_t N = halfEdges.size();
        // Wrap around to the last halfEdge.
        index = N;
    }

    return halfEdges[index - 1];
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

bool HalfEdge::getIsAtStart() const {
	return isAtStart;
}

int HalfEdge::getId() const {
	return id;
}

Vec3 HalfEdge::getDir() const {
	return dir;
}



