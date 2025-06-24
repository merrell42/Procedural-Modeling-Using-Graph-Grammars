// pmugg.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <fstream>
#include <iostream>
#include <vector>
#include "../graph_grammar.h"
#include "../mutator.h"
#include "../graph_mutator.h"
#include "../primitives/primitives.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"
#include "../memory_counter.h"
#include "../util/util.h"
#include "../json versioning/read_json_file.h"
#include "../util/timer.h"

using namespace std;
using Json = nlohmann::json;

int main() {
	// string filePath = "../../grammar data/2D Basic Shapes/square filled.json";
	// string filePath = "../../grammar data/3D Shapes/box-grounded.json";
	// string filePath = "../../grammar data/2D Branches/A.json";
	string filePath = "../../grammar data/3D Complex Shapes/castle.json";
	resetRandom(2);

	try {
		Json parsed = readJsonFile(filePath, false);
		cout << parsed["name"] << endl;
		auto hierarchy = GraphGrammar::import(parsed);
		auto model = new Model();
		auto mutator = new Mutator(model, new GraphMutator(hierarchy, model));
		mutator->iterate(20);
		model->reset();
	} catch (const exception& e) {
	 	cout << "Error: " << e.what() << endl;
	}

	// Print memory leak statistics before program exits
	MemoryCounter::printStats();

	timer->printStats();
	
	return 0;
}
