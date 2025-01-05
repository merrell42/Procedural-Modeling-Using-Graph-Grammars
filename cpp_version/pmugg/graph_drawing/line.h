#pragma once
#include "model.h"
#include "../shapes3D/edge_type3d.h"

class Model;

namespace ms {
	class Line {
	public:
		Line(Model* model, int id, EdgeType3D* type, std::vector<int> endpointIds);
		Line* copy();
		int getId() const { return id; };
		Endpoint* getEndpoint(int index) const;
		std::vector<Endpoint*> getEndpoints() const;
		EdgeType3D* getEdgeType() const { return type; }

	private:
		int id;
		EdgeType3D* type;	
		std::vector<int> endpointIds;

		Model* model;
	};
}
