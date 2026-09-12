#include "pch.h"
#include "RuleGenerator.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "RuleExporter.h"
#include "FixHalfEdgeOrder.h"
#include "CreateSplicedTypes.h"
#include "CreateGroundRule.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/graph_grammar.h"
#include "../../cpp_version/grammar_rules/production_rule.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Json = nlohmann::json;
using namespace std;

void writeStringToFile(const string& filename, const string& content) {
	ofstream file(filename);
	if (!file) {
		throw runtime_error("cannot write " + filename);
	}
	file << content;
}

struct BoundaryIdLayout {
	vector<string> ids;
	bool pairedStubs = false;
};

// Two host stubs of one spliced vertex are one pair. Listing start then end
// vs end then start of a split must not produce two grouping keys.
BoundaryIdLayout collectBoundaryIdLayout(const TemplateGraphSet& set) {
	BoundaryIdLayout layout;
	for (const auto& graph : set.graphs) {
		for (int v = 0; v < (int)graph.vertices.size(); v++) {
			if (!graph.vertices[v].spliced) {
				continue;
			}
			vector<string> stubs;
			for (int eIdx : graph.vertices[v].connections) {
				if (eIdx < 0 || eIdx >= (int)graph.edges.size()) {
					continue;
				}
				if (graph.edges[eIdx].spliced) {
					continue;
				}
				const TemplateEdge& edge = graph.edges[eIdx];
				int other = edge.start == v ? edge.end : edge.start;
				if (other < 0 || other >= (int)graph.vertices.size()) {
					continue;
				}
				const string& id = graph.vertices[other].boundaryId;
				if (!id.empty()) {
					stubs.push_back(id);
				}
			}
			if (stubs.size() == 2) {
				layout.ids.push_back(stubs[0]);
				layout.ids.push_back(stubs[1]);
				layout.pairedStubs = true;
			}
		}
		if (layout.pairedStubs) {
			return layout;
		}
	}
	if (!set.graphs.empty()) {
		for (const auto& vertex : set.graphs[0].vertices) {
			if (!vertex.boundaryId.empty()) {
				layout.ids.push_back(vertex.boundaryId);
			}
		}
	}
	return layout;
}

void sortPairedBoundaryValues(vector<int>& values) {
	for (size_t i = 0; i + 1 < values.size(); i += 2) {
		if (values[i] > values[i + 1]) {
			swap(values[i], values[i + 1]);
		}
	}
}

vector<vector<int>> findBoundaryValues(
	const TemplateMatcher& matcher,
	const BoundaryIdLayout& layout
) {
	int n = (int)layout.ids.size();
	vector<int> vertexIndices;
	vertexIndices.reserve(n);
	for (const string& boundaryId : layout.ids) {
		int vertexIndex = -1;
		for (int v = 0; v < (int)matcher.templateGraph.vertices.size(); v++) {
			if (matcher.templateGraph.vertices[v].boundaryId == boundaryId) {
				vertexIndex = v;
				break;
			}
		}
		vertexIndices.push_back(vertexIndex);
	}

	vector<vector<int>> allBoundaryValues;
	for (const auto& vertexValues : matcher.vertexValues) {
		vector<int> boundaryValues;
		boundaryValues.reserve(n);
		for (int vertexIndex : vertexIndices) {
			boundaryValues.push_back(vertexValues[vertexIndex]);
		}
		if (layout.pairedStubs) {
			sortPairedBoundaryValues(boundaryValues);
		}
		allBoundaryValues.push_back(boundaryValues);
	}
	return allBoundaryValues;
}

void printBoundaryValues(const vector<vector<int>>& boundaryValues) {
	for (size_t m = 0; m < boundaryValues.size(); m++) {
		cout << "      boundary " << m << ": [";
		for (size_t b = 0; b < boundaryValues[m].size(); b++) {
			if (b > 0) {
				cout << ", ";
			}
			cout << boundaryValues[m][b];
		}
		cout << "]\n";
	}
}

// Group graph values by their boundary values.
vector<GraphGroup> groupGraphs(
	// boundary values per graph per graph state per boundary ID.
	const vector<vector<vector<int>>>& boundaryValues
) {
	vector<GraphGroup> groups;
	int numGraphs = (int)boundaryValues.size();
	for (int g = 0; g < numGraphs; g++) {
		for (int m = 0; m < (int)boundaryValues[g].size(); m++) {
			const vector<int>& values = boundaryValues[g][m];
			int groupIndex = -1;
			// Search for a group with the same boundary values.
			for (int j = 0; j < (int)groups.size(); j++) {
				if (groups[j].boundaryValues == values) {
					groupIndex = j;
					break;
				}
			}
			// If no such group exists, create it.
			if (groupIndex == -1) {
				GraphGroup group;
				group.boundaryValues = values;
				group.graphIndices.assign(numGraphs, {});
				groupIndex = (int)groups.size();
				groups.push_back(std::move(group));
			}
			// Add the index to the group.
			groups[groupIndex].graphIndices[g].push_back(m);
		}
	}
	return groups;
}

// Every grammar rule must have a left graph and a right graph.
// Filter out any groups that do not have graphs for multiple graph templates.
vector<GraphGroup> filterEmptyGraphGroups(vector<GraphGroup> groups) {
	groups.erase(
		remove_if(groups.begin(), groups.end(), [](const GraphGroup& group) {
			int templatesWithMatches = 0;
			for (const vector<int>& graphIndices : group.graphIndices) {
				if (!graphIndices.empty()) {
					templatesWithMatches++;
				}
			}
			return templatesWithMatches < 2;
		}),
		groups.end()
	);
	return groups;
}

void printGraphGroups(const vector<GraphGroup>& groups) {
	for (size_t k = 0; k < groups.size(); k++) {
		const auto& group = groups[k];
		cout << "      values [";
		for (size_t b = 0; b < group.boundaryValues.size(); b++) {
			if (b > 0) {
				cout << ", ";
			}
			cout << group.boundaryValues[b];
		}
		cout << "]\n";
		for (size_t g = 0; g < group.graphIndices.size(); g++) {
			cout << "        graph " << g << " matches [";
			for (size_t m = 0; m < group.graphIndices[g].size(); m++) {
				if (m > 0) {
					cout << ", ";
				}
				cout << group.graphIndices[g][m];
			}
			cout << "]\n";
		}
	}
}

int GenerateRules(
	const string& primitivesPath,
    const string& templatesPath,
    const string& outputPath
) {
	try {
		Json parsed = readJsonFile(primitivesPath);
		Primitives* primitives = Primitives::import(parsed["types"]);

		filterSplicedTypes(primitives);
		vector<VertexType*> vertexTypes = primitives->vertexTypes;
		createSplicedTypes(primitives);
		fixHalfEdgeOrder(vertexTypes);
		GraphGrammar grammar(primitives);

		vector<EdgeType*> eTypes;
		for (size_t i = 0; i < primitives->edgeTypes.size(); i++) {
			EdgeType* eType = primitives->edgeTypes[i];
			eType->setRuleGeneratorId("edge" + to_string(i));
			if (!eType->getSpliced()) {
				eTypes.push_back(eType);
			}
		}

		for (size_t i = 0; i < primitives->vertexTypes.size(); i++) {
			VertexType* vType = primitives->vertexTypes[i];
			vType->setRuleGeneratorId((int)i);
		}

		auto library = importTemplateGraphs(templatesPath);
		if (library.includeGround) {
			ProductionRule* groundRule = createGroundRule(primitives);
			if (groundRule) {
				grammar.addGroundRule(groundRule);
			} else {
				cerr << "Warning: includeGround is set but no ground rule was found\n";
			}
		}
		const auto& templateGraphSets = library.sets;
		cout << "match:\n"
			<< "  primitives: " << primitivesPath << "\n"
			<< "  library   : " << templatesPath << "\n"
			<< "  entries   : " << templateGraphSets.size() << "\n";

		auto primitiveGraphs = createPrimitiveGraphs(primitives);
		size_t totalMatches = 0;
		for (size_t i = 0; i < templateGraphSets.size(); i++) {
			cout << "  [" << i << "] \"" << templateGraphSets[i].comment << "\"  ";
			int numGraphs = (int)templateGraphSets[i].graphs.size();
			vector<TemplateMatcher> matchers;
			if (numGraphs <= 1) {
				cout << "skipped (two or more graphs required)\n";
				continue;
			}
			for (int j = 0; j < numGraphs; j++) {
				const auto& templateGraph = templateGraphSets[i].graphs[j];
				matchers.push_back(TemplateMatcher(templateGraph, primitives->vertexTypes, eTypes));
			}
			vector<vector<vector<int>>> allBoundaryValues;
			BoundaryIdLayout boundaryIds = collectBoundaryIdLayout(templateGraphSets[i]);
			for (int j = 0; j < numGraphs; j++) {
				matchers[j].match();
				totalMatches += matchers[j].vertexValues.size();
				for (int m = 0; m < (int)matchers[j].vertexValues.size(); m++) {
					auto graphValues = matchers[j].getGraphValues(m);
					for (size_t v = 0; v < graphValues.vertices.size(); v++) {
						if (!graphValues.vertexOnBoundary[v] && graphValues.vertices[v] == 15) {
							cout << "    v15 match: graph " << j << " match " << m
								<< " vertex " << v << "\n";
						}
					}
				}
				auto boundaryValues = findBoundaryValues(matchers[j], boundaryIds);
				allBoundaryValues.push_back(boundaryValues);
				if (j == 0) {
					const vector<int> targetBoundary = {0, 1, 26, 27};
					for (int m = 0; m < (int)boundaryValues.size(); m++) {
						auto graphValues = matchers[j].getGraphValues(m);
						bool hasV0 = false;
						bool hasV1 = false;
						bool hasV11 = false;
						bool hasV13 = false;
						int interiorCount = 0;
						for (size_t v = 0; v < graphValues.vertices.size(); v++) {
							if (graphValues.vertexOnBoundary[v]) {
								continue;
							}
							interiorCount++;
							const int t = graphValues.vertices[v];
							if (t == 0) {
								hasV0 = true;
							} else if (t == 1) {
								hasV1 = true;
							} else if (t == 11) {
								hasV11 = true;
							} else if (t == 13) {
								hasV13 = true;
							}
						}
						auto printVertices = [&]() {
							for (size_t v = 0; v < graphValues.vertices.size(); v++) {
								cout << " v" << v << "="
									<< (graphValues.vertexOnBoundary[v] ? "e" : "v")
									<< graphValues.vertices[v];
							}
							cout << " boundary [";
							for (size_t b = 0; b < boundaryValues[m].size(); b++) {
								if (b > 0) {
									cout << ", ";
								}
								cout << boundaryValues[m][b];
							}
							cout << "]\n";
						};
						if (boundaryValues[m] == targetBoundary) {
							cout << "    graph 0 match " << m
								<< " boundary [0, 1, 26, 27] vertices:";
							printVertices();
						}
						if (interiorCount == 4 && hasV0 && hasV1 && hasV11 && hasV13) {
							cout << "    graph 0 match " << m
								<< " uses v0,v1,v11,v13 vertices:";
							printVertices();
						}
					}
				}
				cout << "    graph " << j << " allBoundaryValues:\n";
				printBoundaryValues(boundaryValues);
			}
			auto graphGroups = filterEmptyGraphGroups(groupGraphs(allBoundaryValues));
			RuleExporter::exportGroups(grammar, graphGroups, matchers, primitiveGraphs);
			cout << "    boundary values groups across graphs:\n";
			printGraphGroups(graphGroups);
		}
		cout << "  total     : " << totalMatches << " match(es) across "
			 << templateGraphSets.size() << " entries" << endl;
		writeStringToFile(outputPath, grammar.exportJson().dump(2));
		cout << "  output    : " << outputPath << "\n";
		return 0;
	} catch (const exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}
}
