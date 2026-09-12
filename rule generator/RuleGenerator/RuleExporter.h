#pragma once

#include <memory>
#include <vector>
#include "TemplateMatcher.h"
#include "../../cpp_version/graph/graph.h"
#include "../../cpp_version/primitives/primitives.h"

class GraphGrammar;

struct PrimitiveGraphs {
	vector<unique_ptr<Graph>> vertexGraphs;
	vector<unique_ptr<Graph>> edgeGraphs;
};

PrimitiveGraphs createPrimitiveGraphs(Primitives* primitives);

class RuleExporter {
public:
	static void exportRules(
		GraphGrammar& grammar,
		const vector<TemplateMatcher>& matchers,
		const PrimitiveGraphs& primitiveGraphs
	);
};
