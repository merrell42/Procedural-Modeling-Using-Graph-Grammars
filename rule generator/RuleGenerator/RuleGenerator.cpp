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
			if (numGraphs <= 1) {
				cout << "skipped (two or more graphs required)\n";
				continue;
			}
			vector<TemplateMatcher> matchers;
			for (int j = 0; j < numGraphs; j++) {
				matchers.push_back(TemplateMatcher(
					templateGraphSets[i].graphs[j],
					primitives->vertexTypes,
					eTypes
				));
				matchers.back().match();
				totalMatches += matchers.back().vertexValues.size();
			}
			RuleExporter::exportRules(grammar, matchers, primitiveGraphs);
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
