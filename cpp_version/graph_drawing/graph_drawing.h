#pragma once
#include <unordered_map>
#include "half_edge.h"
#include "face.h"
#include "edge.h"
#include "vertex.h"
#include "face_group.h"
#include <iostream>
#include "bsp_node.h"
#include "../geometry/mesh.h"

class HalfEdge;
class Face;
class Edge;
class Vertex;
class BspNode;

// A graph drawing is represented as a doubly connected edge list (DCEL).
// It is a collection of half edges, faces, edges, vertices, face groups,
// and a BSP tree.
class GraphDrawing {
	public:
		GraphDrawing() { bspRootId = -1; }
		~GraphDrawing();
		void copyToCurrent();

		// Get various objects from an ID.
		HalfEdge* getHalfEdge(int id) { return halfEdgeMap[id]; }
		Face* getFace(int id) {         return faceMap[id]; }
		Edge* getEdge(int id) {         return edgeMap[id]; }
		Vertex* getVertex(int id) {     return vertexMap[id];}
		BspNode* getBspNode(int id) {   return bspNodeMap[id];}
		FaceGroup* getFaceGroup(int id) {   return faceGroupMap[id];}

		// Add various objects to the graph drawing.
		void addHalfEdge(int id, HalfEdge* halfEdge) { halfEdgeMap[id] = halfEdge; }
		void addFace    (int id, Face* face) {         faceMap[id] = face; }
		void addEdge    (int id, Edge* edge) {         edgeMap[id] = edge; }
		void addVertex  (int id, Vertex* vertex) {     vertexMap[id] = vertex; }
		void addBspNode (int id, BspNode* bspNode) { bspNodeMap[id] = bspNode; }
		void addFaceGroup(int id, FaceGroup* faceGroup) { faceGroupMap[id] = faceGroup; }

		// Remove various objects from the graph drawing.
		void removeHalfEdge(HalfEdge* halfEdge);
		void removeFace(Face* face);
		void removeEdge(Edge* edge);
		void removeVertex(Vertex* vertex);
		void removeBspNode(BspNode* bspNode);
		void removeFaceGroup(FaceGroup* faceGroup);

		// Get the full maps.
		map<int, HalfEdge*> getHalfEdgeMap() { return halfEdgeMap; }
		map<int, Face*> getFaceMap() { return faceMap; }
		map<int, Edge*> getEdgeMap() { return edgeMap; }
		map<int, Vertex*> getVertexMap() { return vertexMap; }
		map<int, BspNode*> getBspNodeMap() { return bspNodeMap; }
		map<int, FaceGroup*> getFaceGroupMap() { return faceGroupMap; }

		// Save the mesh to an OBJ file.
		void save(string suffix);
		MeshCpp exportMesh();

		// BSP functions.
		bool bspAddEdge(Edge* edge);
		void bspRemoveEdge(Edge* edge);
		bool bspAddFace(Face* face);
		void bspRemoveFace(Face* face);
		void setBspRootId(int id) {
			bspRootId = id;
		}
		int getBspRootId() { return bspRootId; }

	private:
		// Maps from an ID to the various objects in the graph drawing.
		map<int, HalfEdge*> halfEdgeMap;
		map<int, Face*>     faceMap;
		map<int, Edge*>     edgeMap;
		map<int, Vertex*>   vertexMap;
		map<int, BspNode*>  bspNodeMap;
		map<int, FaceGroup*> faceGroupMap;

		// The root node of the BSP tree.
		int bspRootId;
};