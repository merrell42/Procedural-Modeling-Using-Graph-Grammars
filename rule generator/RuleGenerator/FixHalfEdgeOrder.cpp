#include "pch.h"
#include "FixHalfEdgeOrder.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/face_type.h"

#include <iostream>

using namespace std;

namespace {

enum FaceSide {
	LEFT,
	RIGHT,
	UNDETERMINED
};

// Find which side of the half-edge the given face is on.
FaceSide getFaceSide(const HalfEdgeType& halfEdge, FaceType* face) {
	if (halfEdge.edge == nullptr || face == nullptr) {
		return UNDETERMINED;
	}
	for (const auto& faceDatum : halfEdge.edge->getFaceData()) {
		if (faceDatum.type == face) {
			if (faceDatum.onRight == halfEdge.isAtStart) {
				return RIGHT;
			} else {
				return LEFT;
			}
		}
	}
	return UNDETERMINED;
}

FaceType* getRightFace(const HalfEdgeType& halfEdge) {
	if (halfEdge.edge == nullptr) {
		return nullptr;
	}
	for (const auto& faceDatum : halfEdge.edge->getFaceData()) {
		if (getFaceSide(halfEdge, faceDatum.type) == RIGHT) {
			return faceDatum.type;
		}
	}
	return nullptr;
}

// Find all half-edges that have the given face on their left side.
vector<int> nextHalfEdgeIndices(
	const vector<HalfEdgeType>& halfEdges,
	int current,
	FaceType* face
) {
	vector<int> found;
	for (int i = 0; i < (int)halfEdges.size(); i++) {
		if (i == current) {
			continue;
		}
		if (getFaceSide(halfEdges[i], face) == LEFT) {
			found.push_back(i);
		}
	}
	return found;
}

// Walk around the vertex: from each outgoing half-edge take the face on its
// right, then the other half-edge of that face, until back at the start.
// That cycle is the halfEdgeTypes order. If the walk cannot close through
// every half-edge, leave the imported order alone.
void orderByRightFaces(VertexType* vertexType) {
	const vector<HalfEdgeType>& halfEdges = vertexType->getHalfEdgeTypes();
	const int n = (int)halfEdges.size();
	if (n < 3) {
		return;
	}

	vector<int> order;
	vector<bool> visited(n, false);
	int current = 0;
	for (int step = 0; step < n; step++) {
		if (visited[current]) {
			cout << "Walk closed early.\n";
			return;
		}
		visited[current] = true;
		order.push_back(current);

		FaceType* rightFace = getRightFace(halfEdges[current]);
		if (!rightFace) {
			cout << "No right face.\n";
			return;
		}

		vector<int> nextIndices = nextHalfEdgeIndices(halfEdges, current, rightFace);
		if (nextIndices.empty()) {
			cout << "No next half-edge.\n";
			return;
		}
		if (nextIndices.size() > 1) {
			cout << "Multiple next half-edges.\n";
			return;
		}
		current = nextIndices[0];
	}
	if (current != 0) {
		cout << "Walk did not close.\n";
		return;
	}
	for (int i = 0; i < n; i++) {
		if (!visited[i]) {
			cout << "Missed a half-edge.\n";
			return;
		}
	}

	// Reorder the half-edges.
	vector<HalfEdgeType> ordered;
	ordered.reserve(n);
	for (int index : order) {
		ordered.push_back(halfEdges[index]);
	}
	vertexType->setHalfEdgeTypes(std::move(ordered));
}

}

void fixHalfEdgeOrder(const vector<VertexType*>& vertexTypes) {
	for (VertexType* vertexType : vertexTypes) {
		orderByRightFaces(vertexType);
	}
}
