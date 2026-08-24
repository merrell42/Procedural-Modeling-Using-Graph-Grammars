#pragma once

#include <vector>
#include "TemplateMatcher.h"
#include "../../cpp_version/primitives/primitives.h"

class Graph;
class GraphGrammar;
class FaceType;

struct GraphGroup {
	vector<int> boundaryValues;
	vector<vector<int>> graphIndices;
};

class RuleExporter {
public:
	static void exportGroups(
		GraphGrammar& grammar,
		const vector<GraphGroup>& groups,
		const vector<TemplateMatcher>& matchers,
		Primitives* primitives,
		FaceType* groundFace
	);
};
