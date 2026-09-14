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

// Set a breakpoint on the first line of debugInspectGraph to inspect a built Graph
// in the debugger (Watch: graph, graph->getBVertices(), graph->getBHalfEdges()).
void debugInspectGraph(const char* label, Graph* graph, int templateGraphIndex, int matchIndex);

#ifdef _DEBUG
// Builds the bent [1,0,26,27] match and the v15/v33 spliced match, then calls
// debugInspectGraph for each (even if grouping would drop the spliced match).
void debugInspectBendWallMatches(
	const vector<TemplateMatcher>& matchers,
	const PrimitiveGraphs& primitiveGraphs,
	const vector<string>& boundaryIds
);
#endif

class RuleExporter {
public:
	static void exportGroups(
		GraphGrammar& grammar,
		const vector<GraphGroup>& groups,
		const vector<TemplateMatcher>& matchers,
		const PrimitiveGraphs& primitiveGraphs
	);
};
