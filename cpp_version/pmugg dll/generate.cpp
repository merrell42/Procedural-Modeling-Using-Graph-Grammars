#include "pch.h"
#include "generate.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include "../graph_grammar.h"
#include "../mutator.h"
#include "../graph_mutator.h"
#include "../primitives/primitives.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"
#include "../util/util.h"
#include "../settings.h"

using namespace std;
using Json = nlohmann::json;

namespace ms {

Model* model;
GuideMutator* mutator;

void initialize(const char* filePath, char* result, int len, int seed) {
	resetRandom(seed);

	ifstream file(filePath);
	if (!file.is_open()) {
		strcpy_s(result, len, "Error: Could not open file");
		return;
	}
		
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();
	try {
		Json parsed= Json::parse(content);
		auto hierarchy = GraphGrammar::import(parsed["solution"]);
		model = new ms::Model();
		mutator = new GuideMutator(model, new GraphMutator(hierarchy, model));
		strcpy_s(result, len, "Success");
	} catch (const Json::exception& e) {
		string errorMsg = "Error: JSON parsing failed - ";
		errorMsg += e.what();
		strcpy_s(result, len, errorMsg.c_str());
	} catch (const runtime_error& e) {
		string errorMsg = "Error: ";
		errorMsg += e.what();
		strcpy_s(result, len, errorMsg.c_str());
	}
}

void reset(int seed) {
	model->reset();
	mutator->reset();
	resetRandom(seed);
}

void iterate(int steps) {
	mutator->iterate(steps);
}

int getNumFaces() {
	return (int)model->getCurrent()->getFaceMap().size();
}

Mesh getMesh() {
	return model->getCurrent()->exportMesh();
}

void setSize(float x, float y, float z) {
	std::vector<double> extents = {x, y, z};
	globalSettings["Extents"] = extents;
}

};