#pragma once

#include "TemplateMatcher.h"
#include "../../cpp_version/primitives/primitives.h"

class Graph;

class RuleExporter {
public:
	static void exportRule(
		const GraphValues& leftValues,
		const GraphValues& rightValues,
		Primitives* primitives
	);
};
