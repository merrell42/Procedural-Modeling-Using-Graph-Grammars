#include "pch.h"
#include "graphBoundary.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_edge.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_vertex.h"
#include "../../cpp_version/primitives/edge_type.h"

#include <string>
#include <vector>

using namespace std;

namespace {

struct BoundaryKey {
	int edgeTypeId = -1;
	bool forward = false;

	bool operator==(const BoundaryKey& other) const {
		return edgeTypeId == other.edgeTypeId && forward == other.forward;
	}

	bool operator<(const BoundaryKey& other) const {
		if (edgeTypeId != other.edgeTypeId) {
			return edgeTypeId < other.edgeTypeId;
		}
		return forward < other.forward;
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

vector<BoundaryKey> boundaryKeys(const vector<GraphVertex*>& vertices) {
	vector<BoundaryKey> keys;
	keys.reserve(vertices.size());
	for (auto* vertex : vertices) {
		keys.push_back(boundaryKey(vertex));
	}
	return keys;
}

GraphHalfEdge* walkOneFace(GraphHalfEdge* half) {
	while (half->getNext()) {
		half = half->getNext();
	}
	return half->getPrev()->getTwin();
}

vector<GraphVertex*> walkOuterBoundary(Graph* graph) {
	const auto& bVertices = graph->getBVertices();
	if (bVertices.empty()) {
		return {};
	}
	GraphHalfEdge* start = bVertices[0]->interiorHalfEdge();
	if (!start) {
		return {};
	}
	vector<GraphVertex*> order;
	GraphHalfEdge* current = walkOneFace(start);
	order.push_back(current->getVertex());
	while (current != start) {
		current = walkOneFace(current);
		order.push_back(current->getVertex());
	}
	return order;
}

bool keysMatchShifted(
	const vector<BoundaryKey>& leftKeys,
	const vector<BoundaryKey>& rightKeys,
	int start
) {
	const int n = (int)leftKeys.size();
	for (int i = 0; i < n; i++) {
		if (!(leftKeys[i] == rightKeys[(i + start) % n])) {
			return false;
		}
	}
	return true;
}

int findBoundaryShift(const vector<BoundaryKey>& leftKeys, const vector<BoundaryKey>& rightKeys) {
	const int n = (int)leftKeys.size();
	for (int start = 0; start < n; start++) {
		if (keysMatchShifted(leftKeys, rightKeys, start)) {
			return start;
		}
	}
	return -1;
}

vector<GraphVertex*> rotatedOrder(const vector<GraphVertex*>& order, int start) {
	const int n = (int)order.size();
	vector<GraphVertex*> rotated;
	rotated.reserve(n);
	for (int i = 0; i < n; i++) {
		rotated.push_back(order[(i + start) % n]);
	}
	return rotated;
}

}

void alignBoundaries(Graph* left, Graph* right) {
	auto leftOrder = walkOuterBoundary(left);
	auto rightOrder = walkOuterBoundary(right);
	if (leftOrder.size() != rightOrder.size() || leftOrder.empty()) {
		return;
	}
	const int start = findBoundaryShift(boundaryKeys(leftOrder), boundaryKeys(rightOrder));
	if (start < 0) {
		return;
	}
	left->setBVertices(leftOrder);
	right->setBVertices(rotatedOrder(rightOrder, start));
}

string boundaryTypeKey(Graph* graph) {
	auto keys = boundaryKeys(walkOuterBoundary(graph));
	const int n = (int)keys.size();
	if (n == 0) {
		return {};
	}
	int best = 0;
	for (int start = 1; start < n; start++) {
		for (int i = 0; i < n; i++) {
			const auto& a = keys[(best + i) % n];
			const auto& b = keys[(start + i) % n];
			if (b < a) {
				best = start;
				break;
			}
			if (a < b) {
				break;
			}
		}
	}
	string key;
	for (int i = 0; i < n; i++) {
		if (i > 0) {
			key += ",";
		}
		const auto& k = keys[(best + i) % n];
		key += to_string(k.edgeTypeId);
		key += k.forward ? "F" : "B";
	}
	return key;
}
