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

int SayHello() {
    std::cout << "Hello from DLL!" << std::endl;
    return 42;
}

Vec3_C GetTestVector() {
    return testVector;
}

void DoubleVector() {
    testVector.x *= 2;
    testVector.y *= 2;
    testVector.z *= 2;
}


void ResetVector() {
    testVector.x = 1;
    testVector.y = 2;
    testVector.z = 3;
}

Model* model;
GuideMutator* mutator;

int initialize() {
	// ifstream file("C:/model synthesis/model_synthesis_files/Grammar Editor/graphTemplates.txt");
	// ifstream file("../data/box.json");
	// ifstream file("../data/L-floating.json");
	// ifstream file("../data/L-grounded.json");
	ifstream file("C:/PMUGG/cpp_version/pmugg/data/L-grounded.json");
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();

	// vector<GraphTemplate> graphTemplates;
	// Remove array.
	Json parsed = Json::parse(content);
	for (int i = 0; i < parsed.size(); i++) {
		Json parsedI = parsed[i];
		cout << parsedI["category"] << " " << parsedI["name"] << endl;
		// Shape3D::import(parsedI["solution"]["types"]);
		auto hierarchy = NetworkHierarchy::import(parsedI["solution"]);
		model = new ms::Model();
		mutator = new GuideMutator(model, new NetworkMutator(hierarchy, model));
	}
	return 3;
}

void reset() {
	model->reset();
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