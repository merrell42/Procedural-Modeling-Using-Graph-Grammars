#pragma once
#include "model.h"
#include "endpoint.h"
#include "../shapes3D/edge_type3d.h"
#include <iostream>

namespace ms {
	class Model;
	class Line;
	class Endpoint;

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
		void addEndpoint(Endpoint* endpoint, int index);
		void setEndpoint(int index, Endpoint* endpoint);
		EdgeType3D* getEdgeType() const { return type; };
		SplitData split() const {
			std::cout << "TODO: Implement split!" << std::endl;
			SplitData data;
			return data;
		};
		void destroy();

	private:
		int id;
		EdgeType3D* type;	
		std::vector<int> endpointIds;

		Model* model;
	};
}
