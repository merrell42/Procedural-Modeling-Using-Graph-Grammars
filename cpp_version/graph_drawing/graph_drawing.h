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

namespace ms {

class HalfEdge;
class Face;
class Edge;
class Vertex;
class BspNode;

class GraphDrawing {
	public:
		GraphDrawing() { bspRootId = -1; }
		GraphDrawing(
			map<int, HalfEdge*> halfedgeMap,
			map<int, Face*>     faceMap,
			map<int, Edge*>     edgeMap,
			map<int, Vertex*>   vertexMap,
			map<int, BspNode*> bspNodeMap,
			int bspRootId
		);
		void copy();
		HalfEdge* getHalfEdge(int id) { return halfedgeMap[id]; }
		Face* getFace(int id) {         return faceMap[id]; }
		Edge* getEdge(int id) {         return edgeMap[id]; }
		Vertex* getVertex(int id) {     return vertexMap[id];}
		BspNode* getBspNode(int id) {   return bspNodeMap[id];}
		FaceGroup* getFaceGroup(int id) {   return faceGroupMap[id];}

		void addHalfEdge(int id, HalfEdge* halfedge) {
			halfedgeMap[id] = halfedge;
		}
		void addFace    (int id, Face* face) {         faceMap[id] = face; }
		void addEdge    (int id, Edge* edge) {         edgeMap[id] = edge; }
		void addVertex  (int id, Vertex* vertex) {     vertexMap[id] = vertex; }
		void addBspNode (int id, BspNode* bspNode) { bspNodeMap[id] = bspNode; }
		void addFaceGroup(int id, FaceGroup* faceGroup) { faceGroupMap[id] = faceGroup; }

		void removeHalfEdge(HalfEdge* halfedge);
		void removeFace(Face* face);
		void removeEdge(Edge* edge);
		void removeVertex(Vertex* vertex);
		void removeBspNode(BspNode* bspNode);
		void removeFaceGroup(FaceGroup* faceGroup);

		map<int, HalfEdge*> getHalfEdgeMap() { return halfedgeMap; }
		map<int, Face*> getFaceMap() { return faceMap; }
		map<int, Edge*> getEdgeMap() { return edgeMap; }
		map<int, Vertex*> getVertexMap() { return vertexMap; }
		map<int, BspNode*> getBspNodeMap() { return bspNodeMap; }
		map<int, FaceGroup*> getFaceGroupMap() { return faceGroupMap; }

		// TODO: Make this more efficient.
		int getVertexIndex(int vertexId) const {
			int index = 0;
			for (const auto& [id, vertex] : vertexMap) {
				if (id == vertexId) {
					return index;
				}
				index++;
			}
			return -1;
		}

		// Save the mesh to an OBJ file.
		void save(string suffix);

		Mesh exportMesh();

		bool bspAddEdge(Edge* edge);
		void bspRemoveEdge(Edge* edge);
		bool bspAddFace(Face* face);
		void bspRemoveFace(Face* face);
		void setBspRootId(int id) { bspRootId = id; }
		int getBspRootId() { return bspRootId; }

	private:
		map<int, HalfEdge*> halfedgeMap;
		map<int, Face*>     faceMap;
		map<int, Edge*>     edgeMap;
		map<int, Vertex*>   vertexMap;
		map<int, BspNode*>  bspNodeMap;
		map<int, FaceGroup*> faceGroupMap;
		int bspRootId;
};
}