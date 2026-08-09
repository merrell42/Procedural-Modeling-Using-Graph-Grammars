#include "pch.h"
#include "SpliceCheckCommand.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"

#include <iostream>
#include <string>
#include <vector>

using Json = nlohmann::json;
using namespace std;

namespace {

void assignRuleGeneratorIds(Primitives* primitives, const Json& types) {
	for (size_t i = 0; i < primitives->edgeTypes.size(); i++) {
		EdgeType* eType = primitives->edgeTypes[i];
		if (types["edgeTypes"][i].contains("id")) {
			eType->setRuleGeneratorId(types["edgeTypes"][i]["id"].get<string>());
		} else {
			eType->setRuleGeneratorId("edge" + to_string(i));
		}
	}
	for (size_t i = 0; i < primitives->vertexTypes.size(); i++) {
		VertexType* vType = primitives->vertexTypes[i];
		if (types["vertexTypes"][i].contains("id")) {
			vType->setRuleGeneratorId(types["vertexTypes"][i]["id"].get<int>());
		} else {
			vType->setRuleGeneratorId((int)i);
		}
	}
}

int checkLeftRightOnGraph(
	const TemplateGraph& graph,
	const vector<VertexType*>& vTypes,
	const vector<EdgeType*>& eTypes
) {
	TemplateMatcher matcher(graph, vTypes, eTypes);
	int checks = 0;
	int failures = 0;

	for (int v = 0; v < (int)graph.vertices.size(); v++) {
		if (!graph.vertices[v].spliced) {
			continue;
		}
		if (!matcher.usesEdgeState(v)) {
			cerr << "FAIL: spliced vertex " << v << " should use edge states\n";
			failures++;
			continue;
		}

		int spliceConn = -1;
		for (int c = 0; c < (int)graph.vertices[v].connections.size(); c++) {
			if (graph.edges[graph.vertices[v].connections[c]].spliced) {
				spliceConn = c;
				break;
			}
		}
		if (spliceConn < 0) {
			cerr << "FAIL: spliced vertex " << v << " has no spliced edge\n";
			failures++;
			continue;
		}

		for (int s = 0; s < matcher.numEdgeStates; s += 2) {
			bool onRightS = false;
			bool onRightE = false;
			bool okS = matcher.splicedFaceOnRight(v, s, spliceConn, onRightS);
			bool okE = matcher.splicedFaceOnRight(v, s + 1, spliceConn, onRightE);
			string faceIdS = matcher.faceIdForSplicedConnection(v, s, spliceConn);
			string faceIdE = matcher.faceIdForSplicedConnection(v, s + 1, spliceConn);
			checks++;

			cout << "  v" << v << " " << matcher.edgeStates[s].getName()
				 << " onRight=" << (okS ? (onRightS ? "1" : "0") : "?")
				 << " face=" << (faceIdS.empty() ? "(none)" : faceIdS)
				 << " | " << matcher.edgeStates[s + 1].getName()
				 << " onRight=" << (okE ? (onRightE ? "1" : "0") : "?")
				 << " face=" << (faceIdE.empty() ? "(none)" : faceIdE)
				 << "\n";

			if (!okS || !okE) {
				cerr << "FAIL: could not resolve spliced face side at v" << v
					 << " for " << matcher.edgeStates[s].getName() << "\n";
				failures++;
				continue;
			}
			// S and E reverse the walk, so the splice must flip left/right.
			if (onRightS == onRightE) {
				cerr << "FAIL: S/E did not flip left/right at v" << v
					 << " for " << matcher.edgeStates[s].getName() << "\n";
				failures++;
			}
		}
	}

	cout << "  left/right checks: " << checks << ", failures: " << failures << "\n";
	return failures;
}

}  // namespace

int runSpliceCheck(const string& primitivesPath, const string& templatesPath) {
	try {
		Json parsed = readJsonFile(primitivesPath);
		Primitives* primitives = Primitives::import(parsed["types"]);
		assignRuleGeneratorIds(primitives, parsed["types"]);

		vector<EdgeType*> eTypes = primitives->edgeTypes;
		vector<VertexType*> vTypes = primitives->vertexTypes;

		auto sets = importTemplateGraphs(templatesPath);
		cout << "splice-check:\n"
			 << "  primitives: " << primitivesPath << "\n"
			 << "  library   : " << templatesPath << "\n";

		int failures = 0;
		for (size_t i = 0; i < sets.size(); i++) {
			cout << "  [" << i << "] \"" << sets[i].comment << "\"\n";
			for (size_t g = 0; g < sets[i].graphs.size(); g++) {
				const auto& graph = sets[i].graphs[g];
				int splicedVerts = 0;
				int splicedEdges = 0;
				for (const auto& v : graph.vertices) {
					if (v.spliced) splicedVerts++;
				}
				for (const auto& e : graph.edges) {
					if (e.spliced) splicedEdges++;
				}
				if (splicedVerts == 0 && splicedEdges == 0) {
					continue;
				}
				cout << "    graph " << g << " splicedVerts=" << splicedVerts
					 << " splicedEdges=" << splicedEdges << "\n";
				failures += checkLeftRightOnGraph(graph, vTypes, eTypes);

				TemplateMatcher matcher(graph, vTypes, eTypes);
				matcher.match();
				cout << "    graph " << g << " matches=" << matcher.vertexValues.size() << "\n";

				// Boundary + splice smoke: every surviving match should assign edge states
				// at spliced and boundary vertices.
				for (size_t m = 0; m < matcher.vertexValues.size(); m++) {
					const auto& values = matcher.vertexValues[m];
					if ((int)values.size() != matcher.numTemplateVertices) {
						cerr << "FAIL: match " << m << " has " << values.size()
							 << " values, expected " << matcher.numTemplateVertices << "\n";
						failures++;
						continue;
					}
					for (int v = 0; v < matcher.numTemplateVertices; v++) {
						if (!matcher.usesEdgeState(v)) {
							continue;
						}
						int stateIndex = values[v];
						if (stateIndex < 0 || stateIndex >= matcher.numEdgeStates) {
							cerr << "FAIL: match " << m << " v" << v
								 << " edge-valued site has bad state " << stateIndex << "\n";
							failures++;
						}
					}

					// Normal edge between boundary and spliced: same edge type.
					// The two ends are S and E; the spliced mid-edge holds one
					// orientation and advertises S/E toward the two stubs.
					for (int e = 0; e < (int)graph.edges.size(); e++) {
						if (graph.edges[e].spliced) {
							continue;
						}
						int a = graph.edges[e].start;
						int b = graph.edges[e].end;
						bool aBoundary = !graph.vertices[a].boundaryId.empty();
						bool bBoundary = !graph.vertices[b].boundaryId.empty();
						bool aSpliced = graph.vertices[a].spliced;
						bool bSpliced = graph.vertices[b].spliced;
						if (!((aBoundary && bSpliced) || (bBoundary && aSpliced))) {
							continue;
						}
						const EdgeState& stateA = matcher.edgeStates[values[a]];
						const EdgeState& stateB = matcher.edgeStates[values[b]];
						if (stateA.getTypeValue() != stateB.getTypeValue()) {
							cerr << "FAIL: match " << m << " edge " << e
								 << " boundary/spliced edge types differ ("
								 << stateA.getName() << " vs " << stateB.getName() << ")\n";
							failures++;
						}
					}
				}
			}
		}

		if (failures > 0) {
			cerr << "splice-check FAILED with " << failures << " failure(s)\n";
			return 1;
		}
		cout << "splice-check PASSED\n";
		return 0;
	} catch (const exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}
}
