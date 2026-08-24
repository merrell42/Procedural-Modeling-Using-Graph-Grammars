#include "pch.h"
#include "RuleGenerator.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "RuleExporter.h"
#include "FixHalfEdgeOrder.h"
#include "CreateGroundRule.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/graph_grammar.h"
#include "../../cpp_version/grammar_rules/production_rule.h"

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

vector<string> collectBoundaryIds(const TemplateGraph& graph) {
	vector<string> boundaryIds;
	for (const auto& vertex : graph.vertices) {
		if (!vertex.boundaryId.empty()) {
			boundaryIds.push_back(vertex.boundaryId);
		}
	}
	return boundaryIds;
}

vector<vector<int>> findBoundaryValues(
	const TemplateMatcher& matcher,
	const vector<string>& boundaryIds
) {
	int n = (int)boundaryIds.size();
	// Maps from each boundary ID to a vertex index in the template graph.
	vector<int> vertexIndices;
	vertexIndices.reserve(n);
	for (const string& boundaryId : boundaryIds) {
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

		vector<VertexType*> vertexTypes;
		vertexTypes.reserve(primitives->vertexTypes.size());
		for (VertexType* vType : primitives->vertexTypes) {
			if (vType->getSpliced()) {
				continue;
			}
			vertexTypes.push_back(vType);
		}
		primitives->vertexTypes = std::move(vertexTypes);

		fixHalfEdgeOrder(primitives->vertexTypes);
		GraphGrammar grammar(primitives);

		vector<EdgeType*> eTypes;
		for (size_t i = 0; i < primitives->edgeTypes.size(); i++) {
			EdgeType* eType = primitives->edgeTypes[i];
			eType->setRuleGeneratorId("edge" + to_string(i));
			eTypes.push_back(eType);
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
				matchers.push_back(TemplateMatcher(templateGraphSets[i].graphs[j], primitives->vertexTypes, eTypes));
			}
			vector<vector<vector<int>>> allBoundaryValues;
			vector<string> boundaryIds = collectBoundaryIds(templateGraphSets[i].graphs[0]);
			for (int j = 0; j < numGraphs; j++) {
				matchers[j].match();
				auto boundaryValues = findBoundaryValues(matchers[j], boundaryIds);
				allBoundaryValues.push_back(boundaryValues);
				cout << "    graph " << j << " allBoundaryValues:\n";
				printBoundaryValues(boundaryValues);
			}
			auto graphGroups = filterEmptyGraphGroups(groupGraphs(allBoundaryValues));
			RuleExporter::exportGroups(grammar, graphGroups, matchers, primitives);
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
