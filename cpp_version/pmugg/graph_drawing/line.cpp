#include "line.h"

namespace ms {

Line::Line(Model* model, int id, std::vector<Endpoint*> endpointIds)
	: model(model)
	, id(id)
	, endpointIds(endpointIds) {}

Line* Line::copy() {
	auto result = new Line(model, id, endpointIds);
	return result;
}

}