#pragma once

#include "TemplateMatcher.h"
#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/primitives/primitives.h"

#include <array>
#include <memory>
#include <vector>

// Mirrors json.matches[i] from ms.networkHierarchy.partialImport.
struct Match {
	std::vector<int> vertices;
	std::vector<std::array<int, 4>> edges;
};

Match matchFromGraphValues(const GraphValues& values);

// One Graph per vertex type. Connectors are graph->getBVertices().
std::vector<std::unique_ptr<Graph>> createVertexTypeGraphs(Primitives* primitives);

std::vector<std::vector<int>> connectionOrdersFor(Primitives* primitives);

// C++ port of ms.networkHierarchy.glueMatch. Caller owns the returned Graph*.
Graph* glueMatch(
	const Match& match,
	const std::vector<std::unique_ptr<Graph>>& vertexTypeGraphs,
	const std::vector<std::vector<int>>& connectionOrders
);

Graph* buildGraphFromValues(
	const GraphValues& values,
	const std::vector<std::unique_ptr<Graph>>& vertexTypeGraphs,
	const std::vector<std::vector<int>>& connectionOrders
);
