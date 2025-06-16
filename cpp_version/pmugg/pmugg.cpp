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
#include "../json versioning/readJsonFile.h"

using namespace std;
using Json = nlohmann::json;

int main() {
	string filePath = "../../grammar data/2D Basic Shapes/square filled.json";
	// string filePath = "../../grammar data/3D Shapes/box-grounded.json";
	resetRandom(2);

	try {
		Json parsed = readJsonFile(filePath, true);
		cout << parsed["name"] << endl;
		auto hierarchy = GraphGrammar::import(parsed);
		auto model = new Model();
		auto mutator = new Mutator(model, new GraphMutator(hierarchy, model));
		mutator->iterate(100);
	} catch (const exception& e) {
	 	cout << "Error: " << e.what() << endl;
	}
}
