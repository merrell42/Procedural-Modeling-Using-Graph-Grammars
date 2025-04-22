// pmugg.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <iostream>
#include <vector>
#include "../hierarchy/network_hierarchy.h"
#include "../guidelines/network_mutator.h"
#include "../guidelines/guide_mutator.h"
#include "../shapes3D/shape3d.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"

using namespace std;
using namespace ms;
using Json = nlohmann::json;

int main() {
	// ifstream file("C:/model synthesis/model_synthesis_files/Grammar Editor/graphTemplates.txt");
	// ifstream file("../data/box.json");
	// ifstream file("../data/docks.json");
	// ifstream file("../data/hexTower.json");
	ifstream file("../data/house1.json");
	// ifstream file("../data/L-floating.json");
	// ifstream file("../data/L-grounded.json");
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();

	// vector<GraphTemplate> graphTemplates;
	try {
		Json parsed = Json::parse(content);
		cout << parsed["category"] << " " << parsed["name"] << endl;
		// Shape3D::import(parsed["solution"]["types"]);
		auto hierarchy = NetworkHierarchy::import(parsed["solution"]);
		auto model = new ms::Model();
		auto mutator = new GuideMutator(model, new NetworkMutator(hierarchy, model));
		mutator->iterate(50);
		// auto mesh = model->getCurrent()->exportMesh();
	} catch (const std::exception& e) {
		cout << "Error: " << e.what() << endl;
	}
}
