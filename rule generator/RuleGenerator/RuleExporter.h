#pragma once

#include <memory>
#include <vector>
#include "TemplateMatcher.h"
#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/primitives/primitives.h"

class GraphGrammar;

struct GraphGroup {
	vector<int> boundaryValues;
	vector<vector<int>> graphIndices;
};

struct PrimitiveGraphs {
	vector<unique_ptr<Graph>> vertexGraphs;
	vector<unique_ptr<Graph>> edgeGraphs;
};

PrimitiveGraphs createPrimitiveGraphs(Primitives* primitives);

class RuleExporter {
public:
	static void exportGroups(
		GraphGrammar& grammar,
		const vector<GraphGroup>& groups,
		const vector<TemplateMatcher>& matchers,
		const PrimitiveGraphs& primitiveGraphs,
		const vector<string>& boundaryIds
	);
};
