#include "line.h"

namespace ms {

Line::Line(Model* model, int id, EdgeType3D* type, std::vector<int> endpointIds)
	: model(model)
	, type(type)
	, id(id)
	, endpointIds(endpointIds) {
	model->getCurrent()->addLine(id, this);
}

Line* Line::copy() {
	auto result = new Line(model, id, type, endpointIds);
	return result;
}

Endpoint* Line::getEndpoint(int index) const {
	return model->getCurrent()->getEndpoint(endpointIds[index]);
}

std::vector<Endpoint*> Line::getEndpoints() const {
	std::vector<Endpoint*> result;
	for (int i = 0; i < endpointIds.size(); i++) {
		result.push_back(getEndpoint(i));
	}
	return result;
}

void Line::addEndpoint(Endpoint* endpoint, int index) {
	setEndpoint(index, endpoint);
	endpoint->setLine(this);
}
void Line::setEndpoint(int index, Endpoint* endpoint) {
	endpointIds[index] = endpoint->getId();
}

void Line::destroy() {
	model->getCurrent()->removeLine(this);
	delete this;
};

}