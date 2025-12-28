#include "pch.h"
#include "RuleGenerator.h"
#include "GraphTemplate.h"
#include "TemplateMatcher.h"
#include "json.h"
#include <vector>
#include <fstream>
#include <map>
#include "../../cpp_version/primitives/face_type.h"
#include "../../cpp_version/primitives/edge_type.h"
#include "../../cpp_version/primitives/vertex_type.h"

using json = nlohmann::json;
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

	json parsed = json::parse(input);

	map<string, FaceType*> fTypes;
	map<string, EdgeType*> eTypes;
	vector<VertexType*> vTypes;
	
	// Import face types.
	for (int i = 0; i < parsed["faceTypes"].size(); i++) {
		FaceType* fType = FaceType::importRuleGenerator(parsed["faceTypes"][i]);
		fTypes[fType->getSignature()] = fType;
	}
	
	// Import edge types.
	for (int i = 0; i < parsed["edgeTypes"].size(); i++) {
		EdgeType* eType = EdgeType::importRuleGenerator(parsed["edgeTypes"][i], fTypes);
		eTypes[eType->getRuleGeneratorId()] = eType;
	}
	
	// Import vertex types.
	for (int i = 0; i < parsed["vertexTypes"].size(); i++) {
		VertexType* vType = VertexType::importRuleGenerator(parsed["vertexTypes"][i], eTypes);
		vTypes.push_back(vType);
	}

	auto templates = GraphTemplate::DefaultTemplates();
	vector<json> outputVector;
	for (int i = 0; i < templates.size(); i++) {
		TemplateMatcher matcher(templates[i], vTypes, parsed["excludeRepeats"]);
		matcher.match();
		matcher.GetMatches(outputVector);
	}

	parsed["matches"] = outputVector;
	writeStringToFile("C:/model synthesis/model_synthesis_files/Grammar Editor/generatedRules.js", "ms.generatedRules = " + parsed.dump() + ";");

	/*
	json outputs;
	outputs["matches"] = outputVector;
	string outputTemp = outputs.dump();
	strcpy_s(output, maxLength, outputTemp.c_str());
	*/
	strcpy_s(output, maxLength, "Done");
}