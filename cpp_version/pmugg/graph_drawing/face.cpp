#include "face.h"

namespace ms {

Face::Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds, bool looped)
	: model(model)
	, id(id)
    , faceType(faceType)
	, endpointIds(endpointIds)
    , looped(looped){
	model->getCurrent()->addFace(id, this);
}

Face::Face(Model* model, int id, FaceType3D* faceType, std::vector<int> endpointIds)
    : model(model)
    , id(id)
    , faceType(faceType)
    , endpointIds(endpointIds)
    , looped(false) {
    model->getCurrent()->addFace(id, this);
}

Face* Face::copy() {
	auto result = new Face(model, id, faceType, endpointIds, looped);
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

    return Range((float)low, (float)high);
}

void Face::append(Face* faceB) {
    // this->dirty = true;

    // Check if the current object is the same as `faceB`
    if (this == faceB) {
        this->setLooped(true);
        return;
    }

    // Copy endpoints from `faceB`
    auto endpointsB = faceB->getEndpoints(); // Assuming `getEndpoints` returns a `std::vector<Endpoint*>`

    // Connect each endpoint to this face's node
    for (auto* endpointB : endpointsB) {
        endpointIds.push_back(endpointB->getId());
        endpointB->setFace(this);
    }

    // Destroy the node of `faceB`
    model->getCurrent()->removeFace(faceB);
}

}
