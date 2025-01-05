#include "endpoint.h"

namespace ms {

Endpoint::Endpoint(Model* model, int id, int vertexId, int faceId)
	: model(model)
	, id(id)
	, vertexId(vertexId)
	, faceId(faceId) {}

Endpoint* Endpoint::copy() {
	auto result = new Endpoint(model, id, vertexId, faceId);
	return result;
}

Vertex* Endpoint::getVertex() const {
	return model->getCurrent()->getVertex(vertexId);
}

Face* Endpoint::getFace() const {
	return model->getCurrent()->getFace(faceId);
}

Vec3 Endpoint::getPosition() const {
	return getVertex()->getPosition();
}

}