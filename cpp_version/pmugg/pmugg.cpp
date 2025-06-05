// pmugg.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <fstream>
#include <iostream>
#include <vector>
#include "../graph_grammar.h"
#include "../mutator.h"
#include "../graph_mutator.h"
#include "../primitives/primitives.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"
#include "../util/util.h"

using namespace std;
using namespace ms;
using Json = nlohmann::json;

int main() {
	// ifstream file("../../grammar data/2D Basic Shapes/square filled.json");
	// ifstream file("../../grammar data/2D Branches/intersection.json");
	ifstream file("../../grammar data/3D Shapes/box-grounded.json");
	string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
	file.close();
	resetRandom(2);

	try {
		Json parsed = Json::parse(content);
		cout << parsed["category"] << " " << parsed["name"] << endl;
		auto hierarchy = GraphGrammar::import(parsed["solution"]);
		auto model = new ms::Model();
		auto mutator = new GuideMutator(model, new GraphMutator(hierarchy, model));
		mutator->iterate(100);
	} catch (const std::exception& e) {
	 	cout << "Error: " << e.what() << endl;
	}
}
