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
		vector<Edge*> edges;
		vector<HalfEdge*> nextHalfEdges;

		SplitData(const vector<Edge*>& newEdges, const vector<HalfEdge*>& newHalfEdges)
			: edges(newEdges), nextHalfEdges(newHalfEdges) {}
	};

	class Edge {
	public:
		Edge(Model* model, int id, EdgeType* type, vector<int> halfEdgeIds, vector<int> bspNodeIds);
		Edge* copy();
		~Edge();
		int getId() const { return id; };
		Model* getModel() const { return model; };
		void setId(int newId) { id = newId; };
		void setBspNodeIds(const vector<int>& newBspNodeIds) { bspNodeIds = newBspNodeIds; }
		void addBspNodeId(int bspNodeId) { bspNodeIds.push_back(bspNodeId); }
		const vector<int>& getBspNodeIds() const { return bspNodeIds; }
		bool addToBsp();
		void removeFromBsp();
		HalfEdge* getHalfEdge(int index) const;
		vector<HalfEdge*> getHalfEdges() const;
		void addHalfEdge(HalfEdge* halfEdge, int index);
		void setHalfEdge(int index, HalfEdge* halfEdge);
        void setHalfEdgeIds(const vector<int>& ids);
		EdgeType* getEdgeType() const { return type; };
		SplitData split(bool splitFaces);
		pair<SplitData, Vertex*> fullSplit(double s);
		void destroy();
		static VertexType* getVertexType(EdgeType* edgeType);
		bool intersects(Edge* edgeB);
		Vec3* getDirection() const;

	private:
		int id;
		EdgeType* type;	
		vector<int> halfEdgeIds;
		vector<int> bspNodeIds;
		static unordered_map<int, VertexType*> splitVertexTypes;

		Model* model;
	};
}
