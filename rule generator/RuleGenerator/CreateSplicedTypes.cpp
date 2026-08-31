#include "pch.h"
#include "CreateSplicedTypes.h"

#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/face_type.h"
#include "../../cpp_version/geometry/vec3.h"

#include <map>

using namespace std;

namespace {

constexpr double kDirEps = 1e-10;

Vec3 splicedDir(const Vec3& edgeDir, FaceType* face, bool onRight) {
	const Vec3& n = face->getNormal();
	Vec3 dir = onRight ? edgeDir.cross(n) : n.cross(edgeDir);
	dir.normalize();
	return dir;
}

VertexType* createSplicedVertexType(
	EdgeType* edgeType,
	EdgeType* splicedType,
	bool faceOnRight
) {
	auto* vType = new VertexType();
	vType->setSpliced(true);

	// Half-edges are ordered counterclockwise around the vertex.
	// Two opposite pointing half-edges are based on the edgeType.
	if (faceOnRight) {
		vType->addHalfEdge(edgeType, false);
		vType->addHalfEdge(splicedType, true);
		vType->addHalfEdge(edgeType, true);
	} else {
		vType->addHalfEdge(edgeType, false);
		vType->addHalfEdge(edgeType, true);
		vType->addHalfEdge(splicedType, false);
	}
	return vType;
}

}

void filterSplicedTypes(Primitives* primitives) {
	// Remove spliced vertex types.
	vector<VertexType*> vertexTypes;
	vertexTypes.reserve(primitives->vertexTypes.size());
	for (VertexType* vType : primitives->vertexTypes) {
		if (!vType->getSpliced()) {
			vertexTypes.push_back(vType);
		}
	}
	primitives->vertexTypes = std::move(vertexTypes);

	// Remove spliced edge types.
	vector<EdgeType*> edgeTypes;
	edgeTypes.reserve(primitives->edgeTypes.size());
	for (EdgeType* eType : primitives->edgeTypes) {
		if (!eType->getSpliced()) {
			edgeTypes.push_back(eType);
		}
	}
	primitives->edgeTypes = std::move(edgeTypes);
}

void createSplicedTypes(Primitives* primitives) {
	const int numNormalEdges = (int)primitives->edgeTypes.size();
	for (int i = 0; i < numNormalEdges; i++) {
		EdgeType* eType = primitives->edgeTypes[i];
		// Ignore ground plane edges with one half-edge.
		if (eType->getFaceData().size() < 2) {
			continue;
		}

		// Cache of the spliced edge type for each face.
		map<FaceType*, EdgeType*> splicedTypeByFace;
		for (const FaceData& faceDatum : eType->getFaceData()) {
			FaceType* face = faceDatum.type;
			bool onRight = faceDatum.onRight;
			if (!face) {
				continue;
			}

			// Create spliced edge type if it doesn't exist.
			EdgeType* splicedType = nullptr;
			auto it = splicedTypeByFace.find(face);
			if (it == splicedTypeByFace.end()) {
				vector<FaceData> faceData = {{face, true}, {face, false}};
				auto dir = splicedDir(eType->getDir(), face, onRight);
				splicedType = new EdgeType(faceData, dir, false);
				splicedType->setSpliced(true);
				splicedTypeByFace[face] = splicedType;
				primitives->edgeTypes.push_back(splicedType);
			} else {
				splicedType = it->second;
			}

			auto splicedVertexType = createSplicedVertexType(eType, splicedType, onRight);
			primitives->vertexTypes.push_back(splicedVertexType);
		}
	}
}
