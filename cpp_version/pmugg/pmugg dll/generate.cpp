#include "pch.h"
#include "generate.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include "../hierarchy/network_hierarchy.h"
#include "../guidelines/network_mutator.h"
#include "../guidelines/guide_mutator.h"
#include "../shapes3D/shape3d.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"
#include "../util/util.h"

using namespace std;
using Json = nlohmann::json;

namespace ms {

Model* model;
GuideMutator* mutator;

void initialize(const char* filePath, char* result, int len, int seed) {
	resetRandom(seed);
	// try {
		ifstream file(filePath);
		if (!file.is_open()) {
			strcpy_s(result, len, "Error: Could not open file");
			return;
		}
		
		string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
		file.close();

		try {
			Json parsed= Json::parse(content);
			auto hierarchy = NetworkHierarchy::import(parsed["solution"]);
			model = new ms::Model();
			mutator = new GuideMutator(model, new NetworkMutator(hierarchy, model));
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
	// }
	/*catch (const std::exception& e) {
		string errorMsg = "Error: Exception occurred - ";
		errorMsg += e.what();
		strcpy_s(result, len, errorMsg.c_str());
	}
	catch (...) {
		strcpy_s(result, len, "Error: Unknown exception occurred");
	}*/
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
	return model->getCurrent()->getFaceMap().size();
}

Mesh getMesh() {
	return model->getCurrent()->exportMesh();
}

};