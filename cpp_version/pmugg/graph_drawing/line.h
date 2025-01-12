#pragma once
#include "model.h"
#include "line.h"
#include "../shapes3D/edge_type3d.h"
#include <iostream>

namespace ms {

	class Model;
	class Line;

	struct SplitData {
		std::vector<Line*> lines;
		std::vector<Endpoint*> nextEndpoints;
	};

	class Line {
	public:
		Line(Model* model, int id, EdgeType3D* type, std::vector<int> endpointIds);
		Line* copy();
		int getId() const { return id; };
		Endpoint* getEndpoint(int index) const;
		std::vector<Endpoint*> getEndpoints() const;
		EdgeType3D* getEdgeType() const { return type; };
		SplitData split() const {
			std::cout << "TODO: Implement split!" << std::endl;
			SplitData data;
			return data;
		};

	private:
		int id;
		EdgeType3D* type;	
		std::vector<int> endpointIds;

		Model* model;
	};
}
