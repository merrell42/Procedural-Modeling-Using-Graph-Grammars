#pragma once
#include "model.h"

class Model;

namespace ms {
	class Line {
	public:
		Line(Model* model, int id, std::vector<int> endpointIds);
		Line* copy();
		Endpoint* getEndpoint(int index) const;
		std::vector<Endpoint*> getEndpoints() const;

	private:
		int id;
		std::vector<int> endpointIds;

		Model* model;
	};
}
