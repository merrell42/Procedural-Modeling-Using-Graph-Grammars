#include "pch.h"
#include "RuleGenerator.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "../../cpp_version/json versioning/read_json_file.h"
#include "../../cpp_version/primitives/primitives.h"
#include "../../cpp_version/primitives/vertex_type.h"
#include "../../cpp_version/primitives/edge_type.h"

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
		
		// Set ruleGeneratorId on edges if present in JSON, otherwise generate from index.
		Json types = parsed["types"];
		for (size_t i = 0; i < primitives->edgeTypes.size(); i++) {
			EdgeType* eType = primitives->edgeTypes[i];
			if (types["edgeTypes"].size() > i && types["edgeTypes"][i].contains("id")) {
				eType->setRuleGeneratorId(types["edgeTypes"][i]["id"].get<string>());
			} else {
				// Generate ID from index if not present in JSON.
				eType->setRuleGeneratorId("edge" + to_string(i));
			}
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
			if (templateGraphSets[i].graphs.empty()) {
				cout << "skipped (empty graphs)\n";
				continue;
			}
			vector<Json> matches;
			const bool excludeRepeats = false; // parsed["excludeRepeats"]);
			TemplateMatcher matcher(templateGraphSets[i].graphs[0], vTypes, excludeRepeats);
			matcher.match();
			matcher.GetMatches(matches);
			cout << "matches=" << matches.size() << "\n";
			totalMatches += matches.size();
		}
		cout << "  total     : " << totalMatches << " match(es) across "
			<< templateGraphSets.size() << " entries" << endl;

		// parsed["matches"] = outputVector;
		// writeStringToFile("../../generatedRules.js", "ms.generatedRules = " + parsed.dump() + ";");

		/*
		Json outputs;
		outputs["matches"] = outputVector;
		string outputTemp = outputs.dump();
		strcpy_s(output, maxLength, outputTemp.c_str());
		*/
		return 0;
	} catch (const exception& e) {
		cerr << "Error: " << e.what() << endl;
		return 1;
	}
}
