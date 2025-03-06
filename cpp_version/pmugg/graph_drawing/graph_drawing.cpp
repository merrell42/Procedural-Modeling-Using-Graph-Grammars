#include "graph_drawing.h"
#include <fstream>

namespace ms {

GraphDrawing::GraphDrawing(
	std::map<int, Endpoint*> endpointMap,
	std::map<int, Face*>     faceMap,
	std::map<int, Line*>     lineMap,
	std::map<int, Vertex*>   vertexMap
) : endpointMap(endpointMap)
	, faceMap(faceMap)
	, lineMap(lineMap)
	, vertexMap(vertexMap)
{}

GraphDrawing* GraphDrawing::copy() {
	std::map<int, Endpoint*> newEndpointMap;
	std::map<int, Face*>     newFaceMap;
	std::map<int, Line*>     newLineMap;
	std::map<int, Vertex*>   newVertexMap;

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

void GraphDrawing::removeEndpoint(Endpoint* endpoint) { endpointMap.erase(endpoint->getId()); }
void GraphDrawing::removeFace(Face* face) { faceMap.erase(face->getId()); }
void GraphDrawing::removeLine(Line* line) {
	lineMap.erase(line->getId());
}
void GraphDrawing::removeVertex(Vertex* vertex) {
	vertexMap.erase(vertex->getId());
}

void GraphDrawing::save() {
	const std::string filename = "graph_drawing.obj";
	const std::string mtlFilename = "graph_drawing.mtl";
	std::ofstream outFile(filename);
	if (!outFile) {
		std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
		return;
	}

	// Reference the material file
	outFile << "mtllib " << mtlFilename << "\n";

	// Write vertices
	std::vector<int> vertexIds;
	for (const auto& [id, vertex] : vertexMap) {
		const Vec3 v = vertex->getPosition();
		outFile << "v " << v.x << " " << v.y << " " << v.z << "\n";
		vertexIds.push_back(vertex->getId());
	}

	// Write faces with material assignment
	std::string currentMaterial;

	for (const auto& [id, face] : faceMap) {
		const std::string material = face->getFaceType()->getMaterial();
		if (material != currentMaterial) {
			currentMaterial = material;
			outFile << "usemtl " << currentMaterial << "\n";
		}

		outFile << "f";
		for (Endpoint* endpoint : face->getEndpoints()) {
			const int id = endpoint->getVertex()->getId();
			auto it = std::find(vertexIds.begin(), vertexIds.end(), id);

			if (it != vertexIds.end()) {
				outFile << " " << (std::distance(vertexIds.begin(), it) + 1); // Compute index
			}
			
		}
		outFile << "\n";
	}

	outFile.close();
	std::cout << "OBJ file successfully written to " << filename << std::endl;
}

}