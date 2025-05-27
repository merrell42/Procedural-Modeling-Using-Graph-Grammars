#include "pch.h"
#include "graph_drawing.h"
#include <fstream>

namespace ms {

GraphDrawing::GraphDrawing(
	std::map<int, Endpoint*> endpointMap,
	std::map<int, Face*>     faceMap,
	std::map<int, Line*>     lineMap,
	std::map<int, Vertex*>   vertexMap,
	std::map<int, BspNode*> bspNodeMap,
	int bspRootId
) : endpointMap(endpointMap)
	, faceMap(faceMap)
	, lineMap(lineMap)
	, vertexMap(vertexMap)
	, bspNodeMap(bspNodeMap)
	, bspRootId(bspRootId) {}

// This will copy each item into the current model. This happen in the constructor.
void GraphDrawing::copy() {
	for (const auto& [id, ptr] : endpointMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : faceMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : lineMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : vertexMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : bspNodeMap) {
		ptr && ptr->copy();
	}
}

void GraphDrawing::removeEndpoint(Endpoint* endpoint) {
	endpointMap.erase(endpoint->getId());
}
void GraphDrawing::removeFace(Face* face) {
	faceMap.erase(face->getId());
}
void GraphDrawing::removeLine(Line* line) {
	lineMap.erase(line->getId());
}
void GraphDrawing::removeVertex(Vertex* vertex) {
	vertexMap.erase(vertex->getId());
}
void GraphDrawing::removeBspNode(BspNode* bspNode) {
	bspNodeMap.erase(bspNode->getId());
}

void GraphDrawing::save(std::string suffix) {
	const std::string filename = "graph_drawing" + suffix + ".obj";
	const std::string mtlFilename = "graph_drawing.mtl";
	std::ofstream outFile(filename);
	if (!outFile) {
		std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
		return;
	}

	// Reference the material file
	// outFile << "mtllib " << mtlFilename << "\n";

	// Write vertices
	std::vector<int> vertexIds;
	for (const auto& [id, vertex] : vertexMap) {
		const Vec3 v = vertex->getPosition();
		outFile << "v " << v.getX() << " " << v.getY() << " " << v.getZ() << "\n";
		vertexIds.push_back(vertex->getId());
	}

	// Write faces with material assignment
	std::string currentMaterial;

	for (const auto& [id, face] : faceMap) {
		const std::string material = face->getFaceType()->getMaterial();
		if (material != currentMaterial) {
			currentMaterial = material;
			// outFile << "usemtl " << currentMaterial << "\n";
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

Mesh GraphDrawing::exportMesh() {
	std::vector<Vec3> positions;
	std::vector<Vec3> normals;
	std::vector<int> triangles;
	std::vector<int> faceIndices;

	for (const auto& [id, face] : faceMap) {
		face->exportMesh(positions, normals, triangles, faceIndices);
	}

	return createMesh(positions, normals, triangles, faceIndices);
}

bool GraphDrawing::bspAddLine(Line* line) {
	if (bspRootId == -1) {
		bspRootId = 0;
		bspNodeMap[bspRootId] = new BspNode(line->getModel(), bspRootId);
	}
	return bspNodeMap[bspRootId]->addLine(line);
}

void GraphDrawing::bspRemoveLine(Line* line) {
	bspNodeMap[bspRootId]->removeLine(line);
}

bool GraphDrawing::bspAddFace(Face* face) {
	if (bspRootId == -1) {
		bspRootId = 0;
		bspNodeMap[bspRootId] = new BspNode(face->getModel(), bspRootId);
	}
	return bspNodeMap[bspRootId]->addFace(face);
}

void GraphDrawing::bspRemoveFace(Face* face) {
	bspNodeMap[bspRootId]->removeFace(face);
}

}