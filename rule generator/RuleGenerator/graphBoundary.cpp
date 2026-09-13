#include "pch.h"
#include "graphBoundary.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_edge.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_vertex.h"
#include "../../cpp_version/primitives/edge_type.h"

using namespace std;

namespace {

struct BoundaryKey {
	int edgeTypeId = -1;
	bool forward = false;

	bool operator==(const BoundaryKey& other) const {
		return edgeTypeId == other.edgeTypeId && forward == other.forward;
	}
};

BoundaryKey boundaryKey(GraphVertex* vertex) {
	GraphHalfEdge* half = vertex ? vertex->interiorHalfEdge() : nullptr;
	GraphEdge* edge = vertex ? vertex->interiorEdge() : nullptr;
	if (!half || !edge || !edge->getType()) {
		return {};
	}
	return { edge->getType()->getId(), half->getForward() };
}

}

bool equalBoundaries(Graph* left, Graph* right) {
	const auto& leftBVertices = left->getBVertices();
	const auto& rightBVertices = right->getBVertices();
	if (leftBVertices.size() != rightBVertices.size()) {
		return false;
	}
	for (size_t i = 0; i < leftBVertices.size(); i++) {
		if (!(boundaryKey(leftBVertices[i]) == boundaryKey(rightBVertices[i]))) {
			return false;
		}
	}
	return true;
}
