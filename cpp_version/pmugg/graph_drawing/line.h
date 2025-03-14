#pragma once
#include "model.h"
#include "endpoint.h"
#include "../shapes3D/edge_type3d.h"
#include <iostream>
#include <unordered_map>
#include "../guidelines/vertex_type.h"
#include "../shapes3D/edge_type3d.h"

namespace ms {
	class Model;
	class Line;
	class Endpoint;
	class Vertex;

	struct SplitData {
		std::vector<Line*> lines;
		std::vector<Endpoint*> nextEndpoints;

		SplitData(const std::vector<Line*>& newLines, const std::vector<Endpoint*>& newEndpoints)
			: lines(newLines), nextEndpoints(newEndpoints) {}
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
		SplitData split();
		std::pair<SplitData, Vertex*> fullSplit(double s);
		void destroy();
		static VertexType* getVertexType(EdgeType3D* edgeType);

	private:
		int id;
		EdgeType3D* type;	
		std::vector<int> endpointIds;
		static std::unordered_map<int, VertexType*> splitVertexTypes;

		Model* model;
	};
}
