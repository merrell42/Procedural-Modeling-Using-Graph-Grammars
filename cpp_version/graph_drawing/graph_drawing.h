#pragma once
#include <unordered_map>
#include "endpoint.h"
#include "face.h"
#include "line.h"
#include "vertex.h"
#include "face_group.h"
#include <iostream>
#include "../shape/mesh.h"
#include "../bsp/bsp_node.h"

namespace ms {

class Endpoint;
class Face;
class Line;
class Vertex;
class BspNode;

class GraphDrawing {
	public:
		GraphDrawing() { bspRootId = -1; }
		GraphDrawing(
			std::map<int, Endpoint*> endpointMap,
			std::map<int, Face*>     faceMap,
			std::map<int, Line*>     lineMap,
			std::map<int, Vertex*>   vertexMap,
			std::map<int, BspNode*> bspNodeMap,
			int bspRootId
		);
		void copy();
		Endpoint* getEndpoint(int id) { return endpointMap[id]; }
		Face* getFace(int id) {         return faceMap[id]; }
		Line* getLine(int id) {         return lineMap[id]; }
		Vertex* getVertex(int id) {     return vertexMap[id];}
		BspNode* getBspNode(int id) {   return bspNodeMap[id];}
		FaceGroup* getFaceGroup(int id) {   return faceGroupMap[id];}

		void addEndpoint(int id, Endpoint* endpoint) {
			endpointMap[id] = endpoint;
		}
		void addFace    (int id, Face* face) {         faceMap[id] = face; }
		void addLine    (int id, Line* line) {         lineMap[id] = line; }
		void addVertex  (int id, Vertex* vertex) {     vertexMap[id] = vertex; }
		void addBspNode (int id, BspNode* bspNode) { bspNodeMap[id] = bspNode; }
		void addFaceGroup(int id, FaceGroup* faceGroup) { faceGroupMap[id] = faceGroup; }

		void removeEndpoint(Endpoint* endpoint);
		void removeFace(Face* face);
		void removeLine(Line* line);
		void removeVertex(Vertex* vertex);
		void removeBspNode(BspNode* bspNode);
		void removeFaceGroup(FaceGroup* faceGroup);

		std::map<int, Endpoint*> getEndpointMap() { return endpointMap; }
		std::map<int, Face*> getFaceMap() { return faceMap; }
		std::map<int, Line*> getLineMap() { return lineMap; }
		std::map<int, Vertex*> getVertexMap() { return vertexMap; }
		std::map<int, BspNode*> getBspNodeMap() { return bspNodeMap; }
		std::map<int, FaceGroup*> getFaceGroupMap() { return faceGroupMap; }

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
		void save(std::string suffix);

		Mesh exportMesh();

		bool bspAddLine(Line* line);
		void bspRemoveLine(Line* line);
		bool bspAddFace(Face* face);
		void bspRemoveFace(Face* face);
		void setBspRootId(int id) { bspRootId = id; }
		int getBspRootId() { return bspRootId; }

	private:
		std::map<int, Endpoint*> endpointMap;
		std::map<int, Face*>     faceMap;
		std::map<int, Line*>     lineMap;
		std::map<int, Vertex*>   vertexMap;
		std::map<int, BspNode*>  bspNodeMap;
		std::map<int, FaceGroup*> faceGroupMap;
		int bspRootId;
};
}