#pragma once

#include <vector>
class Graph;

std::vector<int> getVertexTypeIds(const Graph* graph);

// Returns true if the two graphs are isomorphic.
bool isIsomorphic(
	Graph* graphA, const std::vector<int>& vertexTypeIdsA,
	Graph* graphB, const std::vector<int>& vertexTypeIdsB
);
