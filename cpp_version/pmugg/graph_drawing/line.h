#pragma once
#include "model.h"

class Model;

namespace ms {
	class Line {
	public:
		Line(Model* model, int id, std::vector<Endpoint*> endpointIds);
		Line* copy();

	private:
		int id;
		std::vector<Endpoint*> endpointIds;

		Model* model;
	};
}
