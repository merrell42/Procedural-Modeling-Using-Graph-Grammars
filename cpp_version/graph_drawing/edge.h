#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include "model.h"
#include "half_edge.h"
#include "../primitives/vertex_type.h"
#include "../primitives/edge_type.h"

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
		Line(Model* model, int id, EdgeType3D* type, std::vector<int> endpointIds, std::vector<int> bspNodeIds);
		Line* copy();
		~Line();
		int getId() const { return id; };
		Model* getModel() const { return model; };
		void setId(int newId) { id = newId; };
		void setBspNodeIds(const std::vector<int>& newBspNodeIds) { bspNodeIds = newBspNodeIds; }
		void addBspNodeId(int bspNodeId) { bspNodeIds.push_back(bspNodeId); }
		const std::vector<int>& getBspNodeIds() const { return bspNodeIds; }
		bool addToBsp();
		void removeFromBsp();
		Endpoint* getEndpoint(int index) const;
		std::vector<Endpoint*> getEndpoints() const;
		void addEndpoint(Endpoint* endpoint, int index);
		void setEndpoint(int index, Endpoint* endpoint);
        void setEndpointIds(const std::vector<int>& ids);
		EdgeType3D* getEdgeType() const { return type; };
		SplitData split(bool splitFaces);
		std::pair<SplitData, Vertex*> fullSplit(double s);
		void destroy();
		static VertexType* getVertexType(EdgeType3D* edgeType);
		bool intersects(Line* lineB);
		Vec3* getDirection() const;

	private:
		int id;
		EdgeType3D* type;	
		std::vector<int> endpointIds;
		std::vector<int> bspNodeIds;
		static std::unordered_map<int, VertexType*> splitVertexTypes;

		Model* model;
	};
}
