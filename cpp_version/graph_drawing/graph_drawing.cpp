#include "pch.h"
#include "graph_drawing.h"
#include <fstream>
#include <unordered_map>

// This copies each item into the current model.
// Each item is copied in the constructor of each item.
void GraphDrawing::copyToCurrent() {
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

GraphDrawing::~GraphDrawing() {
	for (const auto& [id, ptr] : halfEdgeMap) {
		delete ptr;
	}
	for (const auto& [id, ptr] : faceMap) {
		delete ptr;
	}
	for (const auto& [id, ptr] : edgeMap) {
		delete ptr;
	}
	for (const auto& [id, ptr] : vertexMap) {
		delete ptr;
	}
	for (const auto& [id, ptr] : bspNodeMap) {
		delete ptr;
	}
	for (const auto& [id, ptr] : faceGroupMap) {
		delete ptr;
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

MeshCpp GraphDrawing::exportMesh() {
	unordered_map<string, vector<Face*>> materialGroups;

	// Group faces according to their material or color.
	for (const auto& [id, face] : faceMap) {
		const Vec3& color = face->getFaceType()->getColor();
		string colorKey = "r:" + to_string(color.getX()) + ",g:" + to_string(color.getY()) + ",b:" + to_string(color.getZ());
		materialGroups[colorKey].push_back(face);
	}
	
	// Create the mesh structure
	MeshCpp mesh;
	mesh.numSubmeshes = (int)materialGroups.size();
	mesh.submeshes = (SubmeshCpp*)malloc(mesh.numSubmeshes * sizeof(SubmeshCpp));
	
	// Create submeshes for each material group
	int submeshIndex = 0;
	for (const auto& [materialName, faces] : materialGroups) {
		vector<Vec3> positions;
		vector<Vec3> normals;
		vector<int> triangles;
		vector<int> faceIndices;
		for (Face* face : faces) {
			face->exportMesh(positions, normals, triangles, faceIndices);
		}
		if (!positions.empty()) {
			// Parse the color from the material name (which is actually a color key)
			// Format: "r:1.0,g:0.0,b:0.0"
			size_t redStart = materialName.find("r:") + 2;
			size_t redEnd = materialName.find(',', redStart);
			size_t greenStart = materialName.find("g:") + 2;
			size_t greenEnd = materialName.find(',', greenStart);
			size_t blueStart = materialName.find("b:") + 2;
			
			float r = stof(materialName.substr(redStart, redEnd - redStart));
			float g = stof(materialName.substr(greenStart, greenEnd - greenStart));
			float b = stof(materialName.substr(blueStart));
			
			mesh.submeshes[submeshIndex] = createSubmesh(positions, normals, triangles, faceIndices, r, g, b);
			submeshIndex++;
		}
	}
	mesh.numSubmeshes = submeshIndex;
	
	return mesh;
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
