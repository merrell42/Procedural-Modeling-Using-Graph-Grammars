#include "pch.h"
#include "RuleExporter.h"
#include "MatchGraph.h"

#include "../../cpp_version/graph/graph.h"

#include <iostream>

using namespace std;

void RuleExporter::exportRule(
	const GraphValues& leftValues,
	const GraphValues& rightValues,
	Primitives* primitives
) {
	auto vertexTypeGraphs = createVertexTypeGraphs(primitives);
	auto connectionOrders = connectionOrdersFor(primitives);

	try {
		Graph* leftGraph = buildGraphFromValues(leftValues, vertexTypeGraphs, connectionOrders);
		Graph* rightGraph = buildGraphFromValues(rightValues, vertexTypeGraphs, connectionOrders);

		cout << "    exported rule: left graph ("
			<< leftGraph->getVertices().size() << " vertices, "
			<< leftGraph->getEdges().size() << " edges), right graph ("
			<< rightGraph->getVertices().size() << " vertices, "
			<< rightGraph->getEdges().size() << " edges)\n";

		delete leftGraph;
		delete rightGraph;
	} catch (const exception& e) {
		cerr << "    export failed: " << e.what() << "\n";
	}
}
