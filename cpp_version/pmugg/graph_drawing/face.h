#pragma once
#include "model.h"
#include "endpoint.h"
#include "../util/range.h"
#include "../shape/vec3.h"

class Model;
class Endpoint;

namespace ms {

class Face {
	public:
		Face(Model* model, int id, std::vector<int> endpointIds);
		Face* copy();
		std::vector<Endpoint*> getEndpoints() const;
		Endpoint* getEndpoint(int index) const;
		Range dirBounds(const Vec3& dir) const;

	private:
		int id;
		std::vector<int> endpointIds;

		Model* model;
	};
}
