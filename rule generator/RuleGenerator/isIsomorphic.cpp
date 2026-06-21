#include "pch.h"
#include "isIsomorphic.h"

#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/graph/graph_half_edge.h"
#include "../../cpp_version/graph/graph_vertex.h"

#include <algorithm>

using namespace std;

namespace {

int getVertexTypeId(GraphVertex* vertex) {
	return vertex->getType()->getRuleGeneratorId();
}

vector<GraphVertex*> getNeighborVertices(GraphVertex* vertex) {
	vector<GraphVertex*> neighbors;
	for (auto* half : vertex->getHalfEdges()) {
		if (!half || !half->getEdge()) {
			continue;
		}
		GraphHalfEdge* twin = half->getTwin();
		if (!twin) {
			continue;
		}
		GraphVertex* neighbor = twin->getVertex();
		if (neighbor) {
			neighbors.push_back(neighbor);
		}
	}
	return neighbors;
}

vector<int> getNeighborTypeIds(GraphVertex* vertex) {
	vector<int> neighborTypeIds;
	for (auto* neighbor : getNeighborVertices(vertex)) {
		neighborTypeIds.push_back(getVertexTypeId(neighbor));
	}
	sort(neighborTypeIds.begin(), neighborTypeIds.end());
	return neighborTypeIds;
}

int vertexIndex(const vector<GraphVertex*>& vertices, GraphVertex* vertex) {
	for (size_t i = 0; i < vertices.size(); i++) {
		if (vertices[i] == vertex) {
			return (int)i;
		}
	}
	return -1;
}

bool isNeighbor(GraphVertex* vertex, GraphVertex* candidate) {
	for (auto* neighbor : getNeighborVertices(vertex)) {
		if (neighbor == candidate) {
			return true;
		}
	}
	return false;
}

using VertexSignature = pair<int, vector<int>>;

VertexSignature getVertexSignature(GraphVertex* vertex) {
	return { getVertexTypeId(vertex), getNeighborTypeIds(vertex) };
}

vector<VertexSignature> getVertexSignatures(const Graph* graph) {
	vector<VertexSignature> signatures;
	for (auto* vertex : graph->getVertices()) {
		signatures.push_back(getVertexSignature(vertex));
	}
	sort(signatures.begin(), signatures.end());
	return signatures;
}

bool verticesCompatibleUnderMapping(
	GraphVertex* vertexA,
	GraphVertex* vertexB,
	const vector<int>& mapAtoB,
	const vector<int>& mapBtoA,
	const vector<GraphVertex*>& verticesA,
	const vector<GraphVertex*>& verticesB
) {
	auto neighborsA = getNeighborVertices(vertexA);
	auto neighborsB = getNeighborVertices(vertexB);
	if (neighborsA.size() != neighborsB.size()) {
		return false;
	}

	vector<int> unmappedNeighborTypesA;
	vector<int> unmappedNeighborTypesB;
	for (auto* neighborA : neighborsA) {
		int aIndex = vertexIndex(verticesA, neighborA);
		if (aIndex < 0) {
			return false;
		}
		if (mapAtoB[aIndex] >= 0) {
			if (!isNeighbor(vertexB, verticesB[mapAtoB[aIndex]])) {
				return false;
			}
		} else {
			unmappedNeighborTypesA.push_back(getVertexTypeId(neighborA));
		}
	}
	for (auto* neighborB : neighborsB) {
		int bIndex = vertexIndex(verticesB, neighborB);
		if (bIndex < 0) {
			return false;
		}
		if (mapBtoA[bIndex] >= 0) {
			if (!isNeighbor(vertexA, verticesA[mapBtoA[bIndex]])) {
				return false;
			}
		} else {
			unmappedNeighborTypesB.push_back(getVertexTypeId(neighborB));
		}
	}
	sort(unmappedNeighborTypesA.begin(), unmappedNeighborTypesA.end());
	sort(unmappedNeighborTypesB.begin(), unmappedNeighborTypesB.end());
	return unmappedNeighborTypesA == unmappedNeighborTypesB;
}

bool tryIsomorphismMatch(
	int orderIndex,
	const vector<int>& vertexOrder,
	vector<int>& mapAtoB,
	vector<int>& mapBtoA,
	const vector<GraphVertex*>& verticesA,
	const vector<GraphVertex*>& verticesB
) {
	if (orderIndex == (int)verticesA.size()) {
		return true;
	}

	int aIndex = vertexOrder[orderIndex];
	GraphVertex* vertexA = verticesA[aIndex];
	int typeId = getVertexTypeId(vertexA);

	for (int bIndex = 0; bIndex < (int)verticesB.size(); bIndex++) {
		if (mapBtoA[bIndex] >= 0) {
			continue;
		}
		GraphVertex* vertexB = verticesB[bIndex];
		if (getVertexTypeId(vertexB) != typeId) {
			continue;
		}
		if (!verticesCompatibleUnderMapping(
			vertexA,
			vertexB,
			mapAtoB,
			mapBtoA,
			verticesA,
			verticesB
		)) {
			continue;
		}

		mapAtoB[aIndex] = bIndex;
		mapBtoA[bIndex] = aIndex;
		if (tryIsomorphismMatch(
			orderIndex + 1,
			vertexOrder,
			mapAtoB,
			mapBtoA,
			verticesA,
			verticesB
		)) {
			return true;
		}
		mapAtoB[aIndex] = -1;
		mapBtoA[bIndex] = -1;
	}
	return false;
}

vector<int> buildVertexMatchOrder(const vector<GraphVertex*>& vertices) {
	vector<int> order(vertices.size());
	for (size_t i = 0; i < vertices.size(); i++) {
		order[i] = (int)i;
	}
	sort(order.begin(), order.end(), [&](int i, int j) {
		auto signatureI = getVertexSignature(vertices[i]);
		auto signatureJ = getVertexSignature(vertices[j]);
		if (signatureI.second.size() != signatureJ.second.size()) {
			return signatureI.second.size() > signatureJ.second.size();
		}
		if (signatureI.first != signatureJ.first) {
			return signatureI.first < signatureJ.first;
		}
		return signatureI.second > signatureJ.second;
	});
	return order;
}

} // namespace

vector<int> getVertexTypeIds(const Graph* graph) {
	vector<int> vertexTypeIds;
	for (auto* vertex : graph->getVertices()) {
		vertexTypeIds.push_back(getVertexTypeId(vertex));
	}
	sort(vertexTypeIds.begin(), vertexTypeIds.end());
	return vertexTypeIds;
}

bool isIsomorphic(
	Graph* graphA, const vector<int>& vertexTypeIdsA,
	Graph* graphB, const vector<int>& vertexTypeIdsB
) {
	if (vertexTypeIdsA != vertexTypeIdsB) {
		return false;
	}

	const auto& verticesA = graphA->getVertices();
	const vector<GraphVertex*>& verticesB = graphB->getVertices();
	if (verticesA.size() != verticesB.size()) {
		return false;
	}
	if (verticesA.empty()) {
		return true;
	}

	if (getVertexSignatures(graphA) != getVertexSignatures(graphB)) {
		return false;
	}

	vector<int> mapAtoB(verticesA.size(), -1);
	vector<int> mapBtoA(verticesB.size(), -1);
	vector<int> vertexOrder = buildVertexMatchOrder(verticesA);
	return tryIsomorphismMatch(
		0,
		vertexOrder,
		mapAtoB,
		mapBtoA,
		verticesA,
		verticesB
	);
}
