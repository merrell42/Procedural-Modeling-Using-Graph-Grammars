#include "pch.h"
#include <fstream>
#include <iostream>
#include <vector>
#include "../graph_grammar.h"
#include "../mutator.h"
#include "../primitives/primitives.h"
#include "../third_party/json.h"
#include "../graph_drawing/model.h"
#include "../memory_counter.h"
#include "../util/util.h"
#include "../json versioning/read_json_file.h"
#include "../util/timer.h"

using namespace std;
using Json = nlohmann::json;

// The main function for the command line version.
int main() {
	vector<string> filePaths = {
		"../../grammar data/3D Complex Shapes/castle.json",
		"../../grammar data/2D Basic Shapes/square hollow.json",
		"../../grammar data/3D Shapes/house1.json",
		"../../grammar data/3D Shapes/docks.json",
	};
	resetRandom(2);

	// Run the program for each file.
	for (string filePath : filePaths) {
		timer->reset();
		try {
			// Import the grammar JSON file then iterate 100 steps.
			Json parsed = readJsonFile(filePath, false);
			cout << endl << parsed["name"] << endl;
			auto grammar = GraphGrammar::import(parsed);
			auto model = new Model();
			auto mutator = new Mutator(model, grammar);
			mutator->iterate(100);
			cout << "Num Faces: " << model->getCurrent()->getFaceMap().size() << endl;
			model->reset();
		} catch (const exception& e) {
			cout << "Error: " << e.what() << endl;
		}

		// Print memory leak and timing statistics before program exits.
		MemoryCounter::printStats();
		timer->printStats();
	}
	
	return 0;
}
