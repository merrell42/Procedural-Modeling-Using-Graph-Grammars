#include "pch.h"
#include "RuleGenerator.h"
#include "TemplateGraph.h"
#include "TemplateMatcher.h"
#include "json.h"
#include <vector>
#include <fstream>
#include <map>
#include <iostream>
#include "../../cpp_version/primitives/primitives.h"

using Json = nlohmann::json;
using namespace std;

void writeStringToFile(const string& filename, const string& content) {
	ofstream file(filename);

	if (file.is_open()) {
		file << content;
		file.close();
	}
}

void GenerateRules(const char* input_cstr, char* output, int maxLength) {
	string input(input_cstr);

	Json parsed = Json::parse(input);

	// Import using the same Primitives::import as PMUGG.
	// The JSON structure has "types" nested, similar to graph grammars.
	Primitives* primitives = Primitives::import(parsed["types"]);
	
	// Set ruleGeneratorId on edges if present in JSON, otherwise generate from index.
	// Access nested structure: parsed["types"]["edgeTypes"].
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
	// Access nested structure: parsed["types"]["vertexTypes"].
	vector<VertexType*> vTypes;
	for (size_t i = 0; i < primitives->vertexTypes.size(); i++) {
		VertexType* vType = primitives->vertexTypes[i];
		
		// Set ruleGeneratorId if present in JSON.
		if (types["vertexTypes"].size() > i && types["vertexTypes"][i].contains("id")) {
			vType->setRuleGeneratorId(types["vertexTypes"][i]["id"].get<int>());
		}
		
		vTypes.push_back(vType);
	}

	auto templateGraphSets = importTemplateGraphs("../graph templates/graph_templates.json");
	for (int i = 0; i < templateGraphSets.size(); i++) {
		if (templateGraphSets[i].graphs.size() == 0) {
			continue;
		}
		vector<Json> outputVector;
		const bool excludeRepeats = false; // parsed["excludeRepeats"]);
		TemplateMatcher matcher(templateGraphSets[i].graphs[0], vTypes, eTypes, excludeRepeats);
		matcher.match();
		matcher.GetMatches(outputVector);
		cout << "--------------------------------" << endl;
		// cout << outputVector << endl;
	}

	// parsed["matches"] = outputVector;
	// writeStringToFile("../../generatedRules.js", "ms.generatedRules = " + parsed.dump() + ";");

	/*
	Json outputs;
	outputs["matches"] = outputVector;
	string outputTemp = outputs.dump();
	strcpy_s(output, maxLength, outputTemp.c_str());
	*/
	strcpy_s(output, maxLength, "Done");
}