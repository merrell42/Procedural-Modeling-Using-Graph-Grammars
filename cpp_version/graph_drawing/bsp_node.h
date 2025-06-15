#pragma once
#include "../graph_drawing/edge.h"
#include "../graph_drawing/face.h"
#include "../geometry/plane.h"

class Model;

enum class PlaneClassification {
	BELOW = -1,
	BOTH = 0,
	ABOVE = 1,
	ON_PLANE = 2
};

// One node of a Binary Space Partitioning (BSP) tree.
// This is used to efficiently calculate intersections between edges and between faces.
class BspNode {
	public:
		BspNode(Model* model, int id);
		BspNode(Model* model, int id, int parentId, int aboveId, int belowId, Plane* plane, vector<int> faceIds, vector<int> edgeIds);
		~BspNode() { delete plane; }
		BspNode* copy();
		void setParentId(int newParentId);

		// addEdge and addFace returns true, if the edge or faces does not intersect any other geometry.
		bool addEdge(Edge* edge);
		bool addFace(Face* face);

		void removeEdge(Edge* edge);
		void removeFace(Face* face);
		int getId() { return id; }

	private:
		Model* model;
		Plane* plane;
		int id;

		// The IDs of rhe parent node and the two child nodes.
		// The plane of this node divides the geometry into object above or below the plane. 
		int parentId;
		int aboveId;
		int belowId;
		vector<int> faceIds;
		vector<int> edgeIds;

		void connectAbove(BspNode* above);
		void connectBelow(BspNode* below);
		void connectEdge(Edge* edge);
		void connectFace(Face* face);
		BspNode* getAboveNode();
		BspNode* getBelowNode();
		bool hasEdgeIntersection(Edge* edge);
		bool hasFaceIntersection(Face* face);

		PlaneClassification classifyEdge(Edge* edge);
		PlaneClassification classifyFace(Face* face);
		Plane* getPlane();
		bool isPointAbovePlane(Vec3 point);
		bool isPointBelowPlane(Vec3 point);
		void deleteIfEmpty();
};