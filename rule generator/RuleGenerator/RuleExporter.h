#pragma once

#include "TemplateMatcher.h"
#include "../../cpp_version/primitives/primitives.h"

class Graph;
class GraphGrammar;

class RuleExporter {
public:
	static void exportRule(
		GraphGrammar* grammar,
		const GraphValues& leftValues,
		const GraphValues& rightValues,
		Primitives* primitives
	);
};
