#include "vertex.h"

namespace ms {

Vertex::Vertex(Model* model, int id, Vec3 position, std::vector<int> endpointIds)
	: model(model)
	, id(id)
	, endpointIds(endpointIds)
	, position(position) {}

Vertex* Vertex::copy() {
	auto result = new Vertex(model, id, position, endpointIds);
	return result;
}

Endpoint* Vertex::getEndpoint(int index) const {
	return model->getCurrent()->getEndpoint(endpointIds[index]);
}

std::vector<Endpoint*> Vertex::getEndpoints() const {
	std::vector<Endpoint*> result;
	for (int i = 0; i < endpointIds.size(); i++) {
		result.push_back(getEndpoint(i));
	}
	return result;
}

}