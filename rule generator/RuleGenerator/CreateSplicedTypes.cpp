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

constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;

struct SplicedEdgeKey {
	FaceType* face = nullptr;
	int edgeAngleDeg = 0;

	bool operator<(const SplicedEdgeKey& other) const {
		if (face != other.face) {
			return face < other.face;
		}
		return edgeAngleDeg < other.edgeAngleDeg;
	}
};

int undirectedFaceAngleDegrees(FaceType* face, const Vec3& dir) {
	int deg = (int)round(face->angle(dir) * kRadToDeg);
	deg %= 360;
	if (deg < 0) {
		deg += 360;
	}
	if (deg >= 180) {
		deg -= 180;
	}
	return deg;
}

Vec3 splicedDir(const Vec3& edgeDir, FaceType* face, bool onRight) {
	const Vec3& n = face->getNormal();
	Vec3 dir = onRight ? edgeDir.cross(n) : n.cross(edgeDir);
	dir.normalize();
	return dir;
}

VertexType* createSplicedVertexType(
	EdgeType* edgeType,
	EdgeType* splicedType,
	FaceType* face,
	bool faceOnRight
) {
	auto* vType = new VertexType();
	vType->setSpliced(true);

	const bool splicedAtStart = splicedDir(
		edgeType->getDir(),
		face,
		faceOnRight
	).dot(splicedType->getDir()) > 0;

	// Half-edges are ordered counterclockwise around the vertex.
	// Two opposite pointing half-edges are based on the edgeType.
	if (faceOnRight) {
		vType->addHalfEdge(edgeType, false);
		vType->addHalfEdge(splicedType, splicedAtStart);
		vType->addHalfEdge(edgeType, true);
	} else {
		vType->addHalfEdge(edgeType, false);
		vType->addHalfEdge(edgeType, true);
		vType->addHalfEdge(splicedType, splicedAtStart);
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
	// One spliced edge type per face and undirected splice direction.
	map<SplicedEdgeKey, EdgeType*> splicedTypeByKey;
	const int numNormalEdges = (int)primitives->edgeTypes.size();
	for (int i = 0; i < numNormalEdges; i++) {
		EdgeType* eType = primitives->edgeTypes[i];
		// Ignore ground plane edges with one half-edge.
		if (eType->getFaceData().size() < 2) {
			continue;
		}

		for (const FaceData& faceDatum : eType->getFaceData()) {
			FaceType* face = faceDatum.type;
			bool onRight = faceDatum.onRight;
			if (!face) {
				continue;
			}

			// Key by face and undirected splice-arm direction (not base edge angle).
			// Bend Wall corners on the same face share one spliced edge type.
			const Vec3 spliceArm = splicedDir(eType->getDir(), face, onRight);
			SplicedEdgeKey key{
				face,
				undirectedFaceAngleDegrees(face, spliceArm)
			};

			// Create spliced edge type if it doesn't exist.
			EdgeType* splicedType = nullptr;
			auto it = splicedTypeByKey.find(key);
			if (it == splicedTypeByKey.end()) {
				vector<FaceData> faceData = {{face, true}, {face, false}};
				auto dir = splicedDir(eType->getDir(), face, onRight);
				splicedType = new EdgeType(faceData, dir, false);
				splicedType->setSpliced(true);
				splicedTypeByKey[key] = splicedType;
				primitives->edgeTypes.push_back(splicedType);
			} else {
				splicedType = it->second;
			}

			auto splicedVertexType = createSplicedVertexType(
				eType,
				splicedType,
				face,
				onRight
			);
			primitives->vertexTypes.push_back(splicedVertexType);
		}
	}
}
