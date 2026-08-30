#include "pch.h"
#include "CreateSplicedTypes.h"

#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/face_type.h"
#include "../../cpp_version/geometry/vec3.h"

#include <cmath>
#include <map>

using namespace std;

namespace {

constexpr double kDirEps = 1e-10;

Vec3 facePlaneAxis(FaceType* face) {
	const Vec3& n = face->getNormal();
	Vec3 q = Vec3::X_AXIS;
	if (fabs(n.dot(q)) > 0.9) {
		q = Vec3::Y_AXIS;
	}
	Vec3 axis = n.cross(q);
	axis.normalize();
	return axis;
}

Vec3 intoFaceDir(const Vec3& edgeDir, FaceType* face, bool onRight) {
	const Vec3& n = face->getNormal();
	Vec3 into = onRight ? edgeDir.cross(n) : n.cross(edgeDir);
	if (into.length2() < kDirEps) {
		return Vec3();
	}
	into.normalize();
	return into;
}

VertexType* createSplicedVertexType(
	EdgeType* edgeType,
	EdgeType* splicedEdge,
	FaceType* face,
	bool faceOnRight
) {
	auto* vType = new VertexType();
	vType->setSpliced(true);

	// CCW around the vertex. The two T half-edges point opposite ways.
	// The spliced half-edge sits between them on the side of this face:
	//   right face: T-end, spliced, T-start
	//   left face:  T-end, T-start, spliced
	if (faceOnRight) {
		vType->addHalfEdge(edgeType, false);
		vType->addHalfEdge(splicedEdge, true);
		vType->addHalfEdge(edgeType, true);
	} else {
		vType->addHalfEdge(edgeType, false);
		vType->addHalfEdge(edgeType, true);
		vType->addHalfEdge(splicedEdge, false);
	}

	Vec3 into = intoFaceDir(edgeType->getDir(), face, faceOnRight);
	if (into.length2() >= kDirEps) {
		vector<HalfEdgeType> halfEdges = vType->getHalfEdgeTypes();
		const int splicedIndex = faceOnRight ? 1 : 2;
		halfEdges[splicedIndex].dir = into;
		vType->setHalfEdgeTypes(std::move(halfEdges));
	}
	return vType;
}

}

void filterSplicedTypes(Primitives* primitives) {
	if (!primitives) {
		return;
	}

	vector<VertexType*> vertexTypes;
	vertexTypes.reserve(primitives->vertexTypes.size());
	for (VertexType* vType : primitives->vertexTypes) {
		if (!vType->getSpliced()) {
			vertexTypes.push_back(vType);
		}
	}
	primitives->vertexTypes = std::move(vertexTypes);

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
	if (!primitives) {
		return;
	}

	const int numNormalEdges = (int)primitives->edgeTypes.size();
	map<FaceType*, EdgeType*> splicedEdgeByFace;
	for (int i = 0; i < numNormalEdges; i++) {
		EdgeType* eType = primitives->edgeTypes[i];
		// Ignore ground plane edges with one half-edge.
		if (eType->getFaceData().size() < 2) {
			continue;
		}
		for (const FaceData& faceDatum : eType->getFaceData()) {
			FaceType* face = faceDatum.type;
			if (!face || splicedEdgeByFace.count(face)) {
				continue;
			}
			vector<FaceData> faceData = {
				{face, true},
				{face, false}
			};
			auto* splicedEdge = new EdgeType(faceData, facePlaneAxis(face), false);
			splicedEdge->setSpliced(true);
			splicedEdgeByFace[face] = splicedEdge;
			primitives->edgeTypes.push_back(splicedEdge);
		}
	}

	for (int i = 0; i < numNormalEdges; i++) {
		EdgeType* eType = primitives->edgeTypes[i];
		// Ignore ground plane edges with one half-edge.
		if (eType->getFaceData().size() < 2) {
			continue;
		}
		for (const FaceData& faceDatum : eType->getFaceData()) {
			FaceType* face = faceDatum.type;
			if (!face) {
				continue;
			}
			auto it = splicedEdgeByFace.find(face);
			if (it == splicedEdgeByFace.end()) {
				continue;
			}
			VertexType* vType = createSplicedVertexType(eType, it->second, face, faceDatum.onRight);
			primitives->vertexTypes.push_back(vType);
		}
	}
}
