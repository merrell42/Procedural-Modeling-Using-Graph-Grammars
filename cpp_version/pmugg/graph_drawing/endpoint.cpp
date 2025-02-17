#include "endpoint.h"

namespace ms {

Endpoint::Endpoint(Model* model, int id, bool isAtStart, EdgeType3D* edgeType, Vec3 dir, int vertexId, int faceId, int lineId)
	: model(model)
	, id(id)
    , isAtStart(isAtStart)
    , edgeType(edgeType)
    , dir(dir)
	, vertexId(vertexId)
	, faceId(faceId)
	, lineId(lineId) {
    model->getCurrent()->addEndpoint(id, this);
}

Endpoint* Endpoint::copy() {
	auto result = new Endpoint(model, id, isAtStart, edgeType, dir, vertexId, faceId, lineId);
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

void Endpoint::setLine(Line* line) {
    lineId = line->getId();
}

void Endpoint::setFace(Face* face) {
    faceId = face->getId();
}

void Endpoint::mergeFaces(Endpoint* next) {
    getFace()->append(next->getFace());
}

}