#include "endpoint.h"

namespace ms {

Endpoint::Endpoint(Model* model, int id, bool isAtStart, EdgeType3D* edgeType, Vec3 dir, int vertexId, int faceId_, int lineId, bool createFace, int faceIndex)
	: model(model)
	, id(id)
    , isAtStart(isAtStart)
    , edgeType(edgeType)
    , dir(dir)
	, vertexId(vertexId)
    , faceIndex(faceIndex)
	, faceId(faceId_)
	, lineId(lineId) {
    model->getCurrent()->addEndpoint(id, this);

    faceTypeCached = nullptr;
    if (createFace) {
        auto faceType = getFaceType();
        std::vector<int> endpointIds;
        endpointIds.push_back(id);
        faceId = model->newId();
        auto face = new Face(model, faceId, faceType, endpointIds);
        // face.createGroup();
        // face.getNode().connect(this);
    }
}

FaceType3D* Endpoint::getFaceType() {
    if (!faceTypeCached) {
        const auto& faceData = edgeType->faceData;
        faceTypeCached = faceData[faceIndex].type;
    }
    return faceTypeCached;
}

Endpoint* Endpoint::copy() {
	auto result = new Endpoint(model, id, isAtStart, edgeType, dir, vertexId, faceId, lineId, false, -1);
	return result;
}

EdgeType3D* Endpoint::getEdgeType() const {
    return edgeType;
}

Vertex* Endpoint::getVertex() const {
	return model->getCurrent()->getVertex(vertexId);
}

Face* Endpoint::getFace() const {
	return model->getCurrent()->getFace(faceId);
}


Line* Endpoint::getLine() const {
	return model->getCurrent()->getLine(lineId);
}

Vec3 Endpoint::getPosition() const {
	return getVertex()->getPosition();
}

void Endpoint::transfer(Line* replacement) {
	int index = isAtStart ? 1 : 0;
    replacement->addEndpoint(this, index);
}

Endpoint* Endpoint::next() const {
    std::vector<Endpoint*> endpoints = getFace()->getEndpoints();
    size_t N = endpoints.size();
    auto it = std::find(endpoints.begin(), endpoints.end(), this);
    size_t index = std::distance(endpoints.begin(), it);
    return endpoints[(index + 1) % N];
}

Endpoint* Endpoint::prev() const {
    std::vector<Endpoint*> endpoints = getFace()->getEndpoints();
    size_t index = std::find(endpoints.begin(), endpoints.end(), this) - endpoints.begin();

    if (index == 0) {
        size_t N = endpoints.size();
        index = N; // Wrap around to the last endpoint
    }

    return endpoints[index - 1];
}

Endpoint* Endpoint::twin() const {
    Line* line = getLine();
    if (line) {
        std::vector<Endpoint*> endpoints = line->getEndpoints();
        auto it = std::find(endpoints.begin(), endpoints.end(), this);
        if (it != endpoints.end()) {
            size_t index = std::distance(endpoints.begin(), it);
            return endpoints[1 - index]; // Return the other endpoint
        }
    }
    return nullptr;
}

void Endpoint::setLine(Line* line) {
    lineId = line->getId();
}

void Endpoint::setFace(Face* face) {
    faceId = face->getId();
}

void Endpoint::mergeFaces(Endpoint* next) {
    getFace()->append(next->getFace());
}

void Endpoint::maybeMergeNextFace() {
    Endpoint* n = next();
    if (n) {
        this->getFace()->append(n->getFace());
    }
}

//void Endpoint::maybeMergeNextFace() {
//    Endpoint* nextEndpoint = next();
//    if (nextEndpoint) {
//        getFace()->append(nextEndpoint->getFace());
//    }
//}
//
//void Endpoint::maybeMergePrevFace() {
//    Endpoint* prevEndpoint = prev();
//    if (prevEndpoint) {
//        prevEndpoint->getFace()->append(getFace());
//    }
//}

}