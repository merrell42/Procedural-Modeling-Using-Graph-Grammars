#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include "model.h"
#include "half_edge.h"
#include "../primitives/vertex_type.h"
#include "../primitives/edge_type.h"

class Model;
class Edge;
class HalfEdge;
class Vertex;

// After an edge is split, the result of the split.
struct SplitData {
	// An edge is split into two edges.
	vector<Edge*> edges;
	vector<HalfEdge*> nextHalfEdges;

	SplitData(const vector<Edge*>& newEdges, const vector<HalfEdge*>& newHalfEdges)
		: edges(newEdges), nextHalfEdges(newHalfEdges) {}
};

// Represents an edge of the graph drawing.
class Edge {
public:
	Edge(Model* model, int id, EdgeType* type, vector<int> halfEdgeIds, vector<int> bspNodeIds);
	Edge* copy();
	~Edge();
	int getId() const;
	void setId(int newId);
	Model* getModel() const;
	EdgeType* getEdgeType() const;
	Vec3* getDirection() const;

	// BSP functions.
	const vector<int>& getBspNodeIds() const;
	void setBspNodeIds(const vector<int>& newBspNodeIds);
	void addBspNodeId(int bspNodeId);
	bool addToBsp();
	void removeFromBsp();

	// Half edge functions.
	HalfEdge* getHalfEdge(int index) const;
	vector<HalfEdge*> getHalfEdges() const;
	void addHalfEdge(HalfEdge* halfEdge, int index);
	void setHalfEdge(int index, HalfEdge* halfEdge);
    void setHalfEdgeIds(const vector<int>& ids);

	// Split divides the edge into two pieces. It can split the faces.
	SplitData split(bool splitFaces);
	// The full split adds a vertex between the two split lines.
	// s is the position of the new vertex from 0 to 1.
	pair<SplitData, Vertex*> fullSplit(double s);

	static VertexType* getVertexType(EdgeType* edgeType);
	void destroy();

	// Return true, if this intersects an edge.
	bool intersects(Edge* edgeB);

private:
	Model* model;
	int id;
	EdgeType* type;
	// An edge normally has two half edges.
	vector<int> halfEdgeIds;
	vector<int> bspNodeIds;

	// A list of vertex types that are created when splitting an edge.
	static unordered_map<int, VertexType*> splitVertexTypes;
};
