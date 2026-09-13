#include "pch.h"
#include "graphBoundary.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_edge.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_vertex.h"
#include "../../cpp_version/primitives/vertex_type.h"

#include <memory>
#include <unordered_map>
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

void matchBoundaryOrder(Graph* source, Graph* pattern) {
	const auto& patternVertices = pattern->getBVertices();
	const auto& sourceVertices = source->getBVertices();
	if (patternVertices.size() != sourceVertices.size()) {
		return;
	}
	vector<GraphVertex*> ordered;
	vector<bool> used(sourceVertices.size(), false);
	for (GraphVertex* patternVertex : patternVertices) {
		const BoundaryKey patternKey = boundaryKey(patternVertex);
		GraphVertex* match = nullptr;
		for (size_t i = 0; i < sourceVertices.size(); i++) {
			if (used[i]) {
				continue;
			}
			if (boundaryKey(sourceVertices[i]) == patternKey) {
				match = sourceVertices[i];
				used[i] = true;
				break;
			}
		}
		if (!match) {
			return;
		}
		ordered.push_back(match);
	}
	source->setBVertices(ordered);
}

bool hasBoundaryHalfEdge(GraphVertex* vertex) {
	if (!vertex) {
		return false;
	}
	for (auto* half : vertex->getHalfEdges()) {
		if (half && !half->getEdge()) {
			return true;
		}
	}
	return false;
}

bool assignBoundaryVerticesFromPattern(Graph* source, Graph* pattern) {
	const auto& patternVertices = pattern->getBVertices();
	const auto& sourceVertices = source->getVertices();
	if (patternVertices.empty()) {
		return true;
	}
	vector<GraphVertex*> ordered;
	vector<bool> used(sourceVertices.size(), false);
	for (GraphVertex* patternVertex : patternVertices) {
		const BoundaryKey patternKey = boundaryKey(patternVertex);
		GraphVertex* match = nullptr;
		for (size_t i = 0; i < sourceVertices.size(); i++) {
			if (used[i]) {
				continue;
			}
			GraphVertex* candidate = sourceVertices[i];
			if (candidate->getType()->getSpliced()) {
				continue;
			}
			if (!hasBoundaryHalfEdge(candidate)) {
				continue;
			}
			if (boundaryKey(candidate) == patternKey) {
				match = candidate;
				used[i] = true;
				break;
			}
		}
		if (!match) {
			return false;
		}
		ordered.push_back(match);
	}
	source->setBVertices(ordered);
	return true;
}

bool assignBoundaryVerticesFromSurvivors(Graph* source, Graph* pattern) {
	const auto& patternVertices = pattern->getBVertices();
	if (patternVertices.empty()) {
		return true;
	}

	auto despliced = unique_ptr<Graph>(source->copy());
	const vector<GraphVertex*> vertsBeforeRemoval = despliced->getVertices();
	despliced->removeSplices();
	matchBoundaryOrder(despliced.get(), pattern);
	if (!equalBoundaries(despliced.get(), pattern)) {
		return false;
	}

	unordered_map<GraphVertex*, int> vertexToIndex;
	for (int i = 0; i < (int)vertsBeforeRemoval.size(); i++) {
		vertexToIndex[vertsBeforeRemoval[i]] = i;
	}

	const auto& sourceVertices = source->getVertices();
	vector<GraphVertex*> ordered;
	ordered.reserve(patternVertices.size());
	for (GraphVertex* desplicedVertex : despliced->getBVertices()) {
		auto it = vertexToIndex.find(desplicedVertex);
		if (it == vertexToIndex.end()) {
			return false;
		}
		const int index = it->second;
		if (index < 0 || index >= (int)sourceVertices.size()) {
			return false;
		}
		ordered.push_back(sourceVertices[index]);
	}
	source->setBVertices(ordered);
	return true;
}
