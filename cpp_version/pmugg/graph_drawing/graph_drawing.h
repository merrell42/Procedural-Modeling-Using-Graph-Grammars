#pragma once
#include <unordered_map>
#include "endpoint.h"
#include "face.h"
#include "line.h"
#include "vertex.h"

namespace ms {

class Endpoint;
class Face;
class Line;
class Vertex;

class GraphDrawing {
	public:
		GraphDrawing() {}
		GraphDrawing(
			std::map<int, Endpoint*> endpointMap,
			std::map<int, Face*>     faceMap,
			std::map<int, Line*>     lineMap,
			std::map<int, Vertex*>   vertexMap
		);
		GraphDrawing* copy();
		Endpoint* getEndpoint(int id) {
			return endpointMap[id];
		}
		Face* getFace(int id) {         return faceMap[id]; }
		Line* getLine(int id) {
			return lineMap[id];
		}
		Vertex* getVertex(int id) {
			return vertexMap[id];
		}

		void addEndpoint(int id, Endpoint* endpoint) { endpointMap[id] = endpoint; }
		void addFace    (int id, Face* face) {         faceMap[id] = face; }
		void addLine    (int id, Line* line) {         lineMap[id] = line; }
		void addVertex  (int id, Vertex* vertex) {
			vertexMap[id] = vertex;
		}

		void removeEndpoint(Endpoint* endpoint);
		void removeFace(Face* face);
		void removeLine(Line* line);
		void removeVertex(Vertex* vertex);

		std::map<int, Endpoint*> getEndpointMap() { return endpointMap; }
		std::map<int, Face*> getFaceMap() { return faceMap; }
		std::map<int, Line*> getLineMap() { return lineMap; }
		std::map<int, Vertex*> getVertexMap() { return vertexMap; }

		// Save the mesh to an OBJ file.
		void save();

	private:
		std::map<int, Endpoint*> endpointMap;
		std::map<int, Face*>     faceMap;
		std::map<int, Line*>     lineMap;
		std::map<int, Vertex*>   vertexMap;
};
}