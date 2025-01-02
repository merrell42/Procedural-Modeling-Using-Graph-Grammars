#pragma once
#include "model.h"
#include "endpoint.h"

class Model;
class Endpoint;

namespace ms {

class Face {
	public:
		Face(Model* model, int id, std::vector<Endpoint*> endpointIds);
		Face* copy();

	private:
		int id;
		std::vector<Endpoint*> endpointIds;

		Model* model;
	};
}
