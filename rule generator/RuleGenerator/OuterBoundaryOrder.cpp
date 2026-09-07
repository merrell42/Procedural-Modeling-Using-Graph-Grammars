#include "pch.h"
#include "OuterBoundaryOrder.h"

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

struct BoundaryVertexKey {
	int edgeTypeId = -1;
	bool forward = false;

	bool operator==(const BoundaryVertexKey& other) const {
		return edgeTypeId == other.edgeTypeId && forward == other.forward;
	}
};

BoundaryVertexKey boundaryVertexKey(GraphVertex* vertex) {
	GraphHalfEdge* half = vertex ? vertex->interiorHalfEdge() : nullptr;
	GraphEdge* edge = vertex ? vertex->interiorEdge() : nullptr;
	if (!half || !edge || !edge->getType()) {
		return {};
	}
	return { edge->getType()->getId(), half->getForward() };
}

vector<BoundaryVertexKey> boundaryVertexKeys(const vector<GraphVertex*>& vertices) {
	vector<BoundaryVertexKey> keys;
	keys.reserve(vertices.size());
	for (auto* vertex : vertices) {
		keys.push_back(boundaryVertexKey(vertex));
	}
	return keys;
}

GraphHalfEdge* findStartHalfEdge(GraphVertex* vertex) {
	for (auto* half : vertex->getHalfEdges()) {
		if (half->getNext()) {
			return half;
		}
	}
	return nullptr;
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
	GraphHalfEdge* start = findStartHalfEdge(startVertex);
	GraphHalfEdge* current = start;
	current = walkOneFace(current);
	order.push_back(current->getVertex());
	while (current != start) {
		current = walkOneFace(current);
		order.push_back(current->getVertex());
	}
	return order;
}

void printKeys(const char* label, const vector<BoundaryVertexKey>& keys) {
	cout << "      " << label << " [";
	for (size_t i = 0; i < keys.size(); i++) {
		if (i > 0) {
			cout << ", ";
		}
		cout << keys[i].edgeTypeId << (keys[i].forward ? "F" : "B");
	}
	cout << "]\n";
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

	auto leftKeys = boundaryVertexKeys(leftOrder);
	auto rightKeys = boundaryVertexKeys(rightOrder);
	printKeys("left keys:", leftKeys);
	printKeys("right keys:", rightKeys);
	int offset = -1;
	for (int start = 0; start < n; start++) {
		bool match = true;
		for (int i = 0; i < n; i++) {
			if (!(leftKeys[i] == rightKeys[(i + start) % n])) {
				match = false;
				break;
			}
		}
		if (match) {
			offset = start;
			break;
		}
	}
	if (offset < 0) {
		return false;
	}
	return true;
}
