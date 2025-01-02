#include "graph_drawing.h"

namespace ms {

GraphDrawing::GraphDrawing(
	std::unordered_map<int, Endpoint*> endpointMap,
	std::unordered_map<int, Face*>     faceMap,
	std::unordered_map<int, Line*>     lineMap,
	std::unordered_map<int, Vertex*>   vertexMap
) : endpointMap(endpointMap)
	, faceMap(faceMap)
	, lineMap(lineMap)
	, vertexMap(vertexMap)
{}

GraphDrawing* GraphDrawing::copy() {
	std::unordered_map<int, Endpoint*> newEndpointMap;
	std::unordered_map<int, Face*>     newFaceMap;
	std::unordered_map<int, Line*>     newLineMap;
	std::unordered_map<int, Vertex*>   newVertexMap;

	for (const auto& [id, ptr] : endpointMap) {
		newEndpointMap[id] = ptr->copy();
	}
	for (const auto& [id, ptr] : faceMap) {
		newFaceMap[id] = ptr->copy();
	}
	for (const auto& [id, ptr] : lineMap) {
		newLineMap[id] = ptr->copy();
	}
	for (const auto& [id, ptr] : vertexMap) {
		newVertexMap[id] = ptr->copy();
	}
	
	return new GraphDrawing(newEndpointMap, newFaceMap, newLineMap, newVertexMap);
}

}