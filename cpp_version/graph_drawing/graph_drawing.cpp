#include "pch.h"
#include "graph_drawing.h"
#include <fstream>

GraphDrawing::GraphDrawing(
	map<int, HalfEdge*> halfEdgeMap,
	map<int, Face*>     faceMap,
	map<int, Edge*>     edgeMap,
	map<int, Vertex*>   vertexMap,
	map<int, BspNode*> bspNodeMap,
	int bspRootId
) : halfEdgeMap(halfEdgeMap)
	, faceMap(faceMap)
	, edgeMap(edgeMap)
	, vertexMap(vertexMap)
	, bspNodeMap(bspNodeMap)
	, bspRootId(bspRootId) {}

// This will copy each item into the current model.
// This happens in the constructor of each item.
void GraphDrawing::copy() {
	for (const auto& [id, ptr] : halfEdgeMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : faceMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : edgeMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : vertexMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : bspNodeMap) {
		ptr && ptr->copy();
	}
	for (const auto& [id, ptr] : faceGroupMap) {
		ptr && ptr->copy();
	}
}

void GraphDrawing::removeHalfEdge(HalfEdge* halfEdge) {
	halfEdgeMap.erase(halfEdge->getId());
}
void GraphDrawing::removeFace(Face* face) {
	faceMap.erase(face->getId());
}
void GraphDrawing::removeEdge(Edge* edge) {
	edgeMap.erase(edge->getId());
}
void GraphDrawing::removeVertex(Vertex* vertex) {
	vertexMap.erase(vertex->getId());
}
void GraphDrawing::removeBspNode(BspNode* bspNode) {
	bspNodeMap.erase(bspNode->getId());
}
void GraphDrawing::removeFaceGroup(FaceGroup* faceGroup) {
	faceGroupMap.erase(faceGroup->getId());
}

void GraphDrawing::save(string suffix) {
	const string filename = "graph_drawing" + suffix + ".obj";
	const string mtlFilename = "graph_drawing.mtl";
	ofstream outFile(filename);
	if (!outFile) {
		cerr << "Error: Could not open file " << filename << " for writing." << endl;
		return;
	}

	// Reference the material file
	// outFile << "mtllib " << mtlFilename << "\n";

	// Write vertices
	vector<int> vertexIds;
	for (const auto& [id, vertex] : vertexMap) {
		const Vec3 v = vertex->getPosition();
		outFile << "v " << v.getX() << " " << v.getY() << " " << v.getZ() << "\n";
		vertexIds.push_back(vertex->getId());
	}

	// Write faces with material assignment
	string currentMaterial;

	for (const auto& [id, face] : faceMap) {
		const string material = face->getFaceType()->getMaterial();
		if (material != currentMaterial) {
			currentMaterial = material;
			// outFile << "usemtl " << currentMaterial << "\n";
		}

		outFile << "f";
		for (HalfEdge* halfEdge : face->getHalfEdges()) {
			const int id = halfEdge->getVertex()->getId();
			auto it = find(vertexIds.begin(), vertexIds.end(), id);

			if (it != vertexIds.end()) {
				outFile << " " << (distance(vertexIds.begin(), it) + 1); // Compute index
			}
		}
		outFile << "\n";
	}

	outFile.close();
	cout << "OBJ file successfully written to " << filename << endl;
}

Mesh GraphDrawing::exportMesh() {
	vector<Vec3> positions;
	vector<Vec3> normals;
	vector<int> triangles;
	vector<int> faceIndices;

	for (const auto& [id, face] : faceMap) {
		face->exportMesh(positions, normals, triangles, faceIndices);
	}

	return createMesh(positions, normals, triangles, faceIndices);
}

bool GraphDrawing::bspAddEdge(Edge* edge) {
	if (bspRootId == -1) {
		bspRootId = 0;
		bspNodeMap[bspRootId] = new BspNode(edge->getModel(), bspRootId);
	}
	return bspNodeMap[bspRootId]->addEdge(edge);
}

void GraphDrawing::bspRemoveEdge(Edge* edge) {
	bspNodeMap[bspRootId]->removeEdge(edge);
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
