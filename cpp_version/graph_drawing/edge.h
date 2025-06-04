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
	class Edge;
	class HalfEdge;
	class Vertex;

	struct SplitData {
		std::vector<Edge*> edges;
		std::vector<HalfEdge*> nextHalfEdges;

		SplitData(const std::vector<Edge*>& newEdges, const std::vector<HalfEdge*>& newHalfEdges)
			: edges(newEdges), nextHalfEdges(newHalfEdges) {}
	};

	class Edge {
	public:
		Edge(Model* model, int id, EdgeType3D* type, std::vector<int> halfedgeIds, std::vector<int> bspNodeIds);
		Edge* copy();
		~Edge();
		int getId() const { return id; };
		Model* getModel() const { return model; };
		void setId(int newId) { id = newId; };
		void setBspNodeIds(const std::vector<int>& newBspNodeIds) { bspNodeIds = newBspNodeIds; }
		void addBspNodeId(int bspNodeId) { bspNodeIds.push_back(bspNodeId); }
		const std::vector<int>& getBspNodeIds() const { return bspNodeIds; }
		bool addToBsp();
		void removeFromBsp();
		HalfEdge* getHalfEdge(int index) const;
		std::vector<HalfEdge*> getHalfEdges() const;
		void addHalfEdge(HalfEdge* halfedge, int index);
		void setHalfEdge(int index, HalfEdge* halfedge);
        void setHalfEdgeIds(const std::vector<int>& ids);
		EdgeType3D* getEdgeType() const { return type; };
		SplitData split(bool splitFaces);
		std::pair<SplitData, Vertex*> fullSplit(double s);
		void destroy();
		static VertexType* getVertexType(EdgeType3D* edgeType);
		bool intersects(Edge* edgeB);
		Vec3* getDirection() const;

	private:
		int id;
		EdgeType3D* type;	
		std::vector<int> halfedgeIds;
		std::vector<int> bspNodeIds;
		static std::unordered_map<int, VertexType*> splitVertexTypes;

		Model* model;
	};
}
