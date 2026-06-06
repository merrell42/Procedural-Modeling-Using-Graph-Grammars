#include "pch.h"
#include "RuleGenerator.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"

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

vector<string> collectBoundaryIds(const TemplateGraph& graph) {
	vector<string> boundaryIds;
	for (const auto& vertex : graph.vertices) {
		if (!vertex.boundaryId.empty()) {
			boundaryIds.push_back(vertex.boundaryId);
		}
	}
	return boundaryIds;
}

vector<vector<int>> findBoundaryStates(
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

	vector<vector<int>> allBoundaryStates;
	for (const auto& graphStates : matcher.graphStates) {
		vector<int> boundaryStates;
		boundaryStates.reserve(n);
		for (int vertexIndex : vertexIndices) {
			boundaryStates.push_back(graphStates[vertexIndex]);
		}
		allBoundaryStates.push_back(boundaryStates);
	}
	return allBoundaryStates;
}

void printBoundaryStates(const vector<vector<int>>& boundaryStates) {
	for (size_t m = 0; m < boundaryStates.size(); m++) {
		cout << "      boundary " << m << ": [";
		for (size_t b = 0; b < boundaryStates[m].size(); b++) {
			if (b > 0) {
				cout << ", ";
			}
			cout << boundaryStates[m][b];
		}
		cout << "]\n";
	}
}

// A group of graphs that have the same boundary.
struct GraphGroup {
	// Boundary states for each boundary ID.
	vector<int> boundaryStates;
	// The indices of the graph states within each matcher that share these boundary states.
	vector<vector<int>> graphIndices;
};

// Group graph states by their boundary states.
vector<GraphGroup> groupGraphs(
	// boundary states per graph per graph state per boundary ID.
	const vector<vector<vector<int>>>& boundaryStates
) {
	vector<GraphGroup> groups;
	int numGraphs = (int)boundaryStates.size();
	for (int g = 0; g < numGraphs; g++) {
		for (int m = 0; m < (int)boundaryStates[g].size(); m++) {
			const vector<int>& states = boundaryStates[g][m];
			int groupIndex = -1;
			// Search for a group with the same boundary states.
			for (int j = 0; j < (int)groups.size(); j++) {
				if (groups[j].boundaryStates == states) {
					groupIndex = j;
					break;
				}
			}
			// If no such group exists, create it.
			if (groupIndex == -1) {
				GraphGroup group;
				group.boundaryStates = states;
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
		cout << "      states [";
		for (size_t b = 0; b < group.boundaryStates.size(); b++) {
			if (b > 0) {
				cout << ", ";
			}
			cout << group.boundaryStates[b];
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
		
		// Set ruleGeneratorId on edges if present in JSON, otherwise generate from index.
		Json types = parsed["types"];
		vector<EdgeType*> eTypes;
		for (size_t i = 0; i < primitives->edgeTypes.size(); i++) {
			EdgeType* eType = primitives->edgeTypes[i];
			if (types["edgeTypes"].size() > i && types["edgeTypes"][i].contains("id")) {
				eType->setRuleGeneratorId(types["edgeTypes"][i]["id"].get<string>());
			} else {
				// Generate ID from index if not present in JSON.
				eType->setRuleGeneratorId("edge" + to_string(i));
			}
			eTypes.push_back(eType);
		}
		
		// Set ruleGeneratorId on vertices if present in JSON.
		vector<VertexType*> vTypes;
		for (size_t i = 0; i < primitives->vertexTypes.size(); i++) {
			VertexType* vType = primitives->vertexTypes[i];
			
			// Set ruleGeneratorId if present in JSON.
			if (types["vertexTypes"].size() > i && types["vertexTypes"][i].contains("id")) {
				vType->setRuleGeneratorId(types["vertexTypes"][i]["id"].get<int>());
			}
			
			vTypes.push_back(vType);
		}

		auto templateGraphSets = importTemplateGraphs(templatesPath);
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
				matchers.push_back(TemplateMatcher(templateGraphSets[i].graphs[j], vTypes, eTypes));
			}
			vector<vector<vector<int>>> allBoundaryStates;
			vector<string> boundaryIds = collectBoundaryIds(templateGraphSets[i].graphs[0]);
			for (int j = 0; j < numGraphs; j++) {
				matchers[j].match();
				auto boundaryStates = findBoundaryStates(matchers[j], boundaryIds);
				allBoundaryStates.push_back(boundaryStates);
				cout << "    graph " << j << " allBoundaryStates:\n";
				printBoundaryStates(boundaryStates);
			}
			auto GraphGroups = filterEmptyGraphGroups(groupGraphs(allBoundaryStates));
			cout << "    boundary state groups across graphs:\n";
			printGraphGroups(GraphGroups);
		}
		cout << "  total     : " << totalMatches << " match(es) across "
			<< templateGraphSets.size() << " entries" << endl;
		return 0;
	} catch (const exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}
}
