#include "pch.h"
#include "generate.h"
#include <iostream>
#include <fstream>
#include <vector>
#include "../hierarchy/network_hierarchy.h"
#include "../guidelines/network_mutator.h"
#include "../guidelines/guide_mutator.h"
#include "../shapes3D/shape3d.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"

using namespace std;
using Json = nlohmann::json;

namespace ms {

Model* model;
GuideMutator* mutator;

int initialize(const char* filePath) {
	ifstream file(filePath);
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();

	// vector<GraphTemplate> graphTemplates;
	// Remove array.
	Json parsed = Json::parse(content);
	auto hierarchy = NetworkHierarchy::import(parsed["solution"]);
	model = new ms::Model();
	mutator = new GuideMutator(model, new NetworkMutator(hierarchy, model));
	return 4;
}

void reset() {
	model->reset();
	mutator->reset();
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