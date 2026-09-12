#include "pch.h"
#include "graphBoundary.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_edge.h"
#include "../../cpp_version/graph/graph_face.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_vertex.h"
#include "../../cpp_version/primitives/edge_type.h"

#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

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
	const int n = (int)bVertices.size();
	if (n == 0) {
		return {};
	}

	GraphVertex* startVertex = bVertices[0];
	vector<GraphVertex*> order = {};
	GraphHalfEdge* start = startVertex->interiorHalfEdge();
	if (!start) {
		cout << "no start half-edge\n";
		return {};
	}
	GraphHalfEdge* current = start;

	// Walk each face until we return to the start.
	current = walkOneFace(current);
	order.push_back(current->getVertex());
	while (current != start) {
		current = walkOneFace(current);
		order.push_back(current->getVertex());
	}
	return order;
}

void printKeys(const vector<BoundaryKey>& keys) {
	for (size_t i = 0; i < keys.size(); i++) {
		if (i > 0) {
			cout << ", ";
		}
		cout << keys[i].edgeTypeId << (keys[i].forward ? "F" : "B");
	}
	cout << "\n";
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

int findBoundaryShift(
	const vector<BoundaryKey>& leftKeys,
	const vector<BoundaryKey>& rightKeys
) {
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

bool equalBoundaries(Graph* left, Graph* right) {
	vector<GraphVertex*> leftOrder = walkOuterBoundary(left);
	vector<GraphVertex*> rightOrder = walkOuterBoundary(right);
	if (leftOrder.size() != rightOrder.size()) {
		return false;
	}
	const int n = (int)leftOrder.size();
	if (n == 0) {
		return true;
	}

	auto leftKeys = boundaryKeys(leftOrder);
	auto rightKeys = boundaryKeys(rightOrder);
	// cout << "left keys: ";	printKeys(leftKeys);
	// cout << "right keys: ";	printKeys(rightKeys);
	return findBoundaryShift(leftKeys, rightKeys) >= 0;
}

void alignBoundaries(Graph* left, Graph* right) {
	vector<GraphVertex*> leftOrder = walkOuterBoundary(left);
	vector<GraphVertex*> rightOrder = walkOuterBoundary(right);
	if (leftOrder.size() != rightOrder.size() || leftOrder.empty()) {
		return;
	}

	const int start = findBoundaryShift(boundaryKeys(leftOrder), boundaryKeys(rightOrder));
	if (start < 0) {
		return;
	}

	left->setBVertices(leftOrder);
	right->setBVertices(rotatedOrder(rightOrder, start));
	cout << "    aligned boundary vertices with shift " << start << "\n";
}
