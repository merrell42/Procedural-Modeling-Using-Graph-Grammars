#include "face.h"

namespace ms {

Face::Face(Model* model, int id, std::vector<Endpoint*> endpointIds)
	: model(model)
	, id(id)
	, endpointIds(endpointIds) {}

Face* Face::copy() {
	auto result = new Face(model, id, endpointIds);
	return result;
}

}
