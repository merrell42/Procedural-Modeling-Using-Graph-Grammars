#include "face.h"

namespace ms {

Face::Face(Model* model, int id, std::vector<int> endpointIds)
	: model(model)
	, id(id)
	, endpointIds(endpointIds) {}

Face* Face::copy() {
	auto result = new Face(model, id, endpointIds);
	return result;
}

Endpoint* Face::getEndpoint(int index) const {
	return model->getCurrent()->getEndpoint(endpointIds[index]);
}

std::vector<Endpoint*> Face::getEndpoints() const {
	std::vector<Endpoint*> result;
	for (int i = 0; i < endpointIds.size(); i++) {
		result.push_back(getEndpoint(i));
	}
	return result;
}

Range Face::dirBounds(const Vec3& dir) const {
    double low = std::numeric_limits<double>::infinity();
    double high = -std::numeric_limits<double>::infinity();

    std::vector<Endpoint*> endpoints = getEndpoints();
    for (const Endpoint* endpoint : endpoints) {
        double d = dir.dot(endpoint->getPosition());
        low = std::min(d, low);
        high = std::max(d, high);
    }

    return Range(low, high);
}

}
