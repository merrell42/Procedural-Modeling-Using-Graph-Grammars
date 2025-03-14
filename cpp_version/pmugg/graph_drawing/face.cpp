#include "face.h"
#include "face_group.h"
#include "endpoint.h"
#include "../shapes3D/face_type3d.h"
#include <vector>
#include <algorithm>

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

void Face::destroy() {
	model->getCurrent()->removeFace(this);
	delete this;
};

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

std::vector<Vec3> Face::getPositions() const {
    std::vector<Vec3> positions; // Assuming Vec3 is the type for positions
    auto endpoints = getEndpoints(); // Get the endpoints

    for (const auto& endpoint : endpoints) {
        positions.push_back(endpoint->getPosition()); // Assuming getPosition returns a Vec3
    }

    return positions; // Return the vector of positions
}

FaceGroup* Face::getGroup() const {
    FaceGroup* group = new FaceGroup();
    group->addFace(const_cast<Face*>(this));
    return group;
}

void Face::split(Endpoint* endpoint) {
    std::vector<Endpoint*> endpoints = this->getEndpoints();
    auto index = std::find(endpoints.begin(), endpoints.end(), endpoint) - endpoints.begin();

    if (looped) {
        this->setLooped(false);
        std::vector<int> newOrder(endpointIds.begin() + index, endpointIds.end());
        newOrder.insert(newOrder.end(), endpointIds.begin(), endpointIds.begin() + index);
        endpointIds = newOrder;
    } else {
        if (index == 0) {
            // ms::alert("Should not be splitting off all of the endpoints.");
            return;
        }

        std::vector<Endpoint*> splitEndpoints(endpoints.begin() + index, endpoints.end());
        std::vector<int> splitEndpointIds(endpointIds.begin() + index, endpointIds.end());
        Face* newFace = new Face(model, model->newId(), faceType, splitEndpointIds);
        /* if (isHole()) {
            newFace->setHole(true);
        }

        bool insertAtStart = !this->isHole();
        if (insertAtStart) {
            this->getGroup()->getNode()->splice(newFace, 0);
        } else {
            this->getGroup()->getNode()->connect(newFace);
        } */

        for (auto& splitEndpoint : splitEndpoints) {
            splitEndpoint->setFace(newFace);
        }
    }
}

void Face::insert(Endpoint* endpoint, Endpoint* prevEndpoint) {
    // Get the current endpoints of the face
    std::vector<Endpoint*> endpoints = this->getEndpoints();
    
    // Find the index of the previous endpoint
    auto it = std::find(endpoints.begin(), endpoints.end(), prevEndpoint);
    int index = (it != endpoints.end()) ? std::distance(endpoints.begin(), it) : -1;

    // Insert the new endpoint at the correct position
    int id = endpoint->getId();
    if (index >= 0) {
        endpointIds.insert(endpointIds.begin() + index + 1, id);
        endpoint->setFace(this);
    } else {
        endpointIds.insert(endpointIds.begin(), id);
        endpoint->setFace(this);
    }
}

}
