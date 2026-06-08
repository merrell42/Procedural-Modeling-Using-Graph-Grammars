#include "pch.h"
#include <fstream>
#include <iostream>
#include <string>
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
#include "../mesh_extraction/mesh_to_grammar.h"

using namespace std;
using Json = nlohmann::json;

// The main function for the command line version.
int main(int argc, char** argv) {
	if (argc >= 4 && std::string(argv[1]) == "--mesh-to-grammar") {
		return mesh_extraction::meshToGrammar(argv[2], argv[3]);
	}

	vector<string> filePaths = {
		"../../grammar data/2D Basic Shapes/square hollow.json",
		// "../../rule generator/generatedRules.json",
	};
	/* vector<string> filePaths = {
		"../../grammar data/3D Complex Shapes/castle.json",
		"../../grammar data/2D Basic Shapes/square filled.json",
		"../../grammar data/3D Shapes/house1.json",
		"../../grammar data/3D Shapes/docks.json",
	}; */
	resetRandom(2);

	// Run the program for each file.
	for (string filePath : filePaths) {
		timer->reset();
		MemoryCounter::reset();
		
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
			delete model;
			delete grammar;
		} catch (const exception& e) {
			cout << "Error: " << e.what() << endl;
		}

		// Print memory leak and timing statistics after each file.
		MemoryCounter::printStats();
		timer->printStats();
	}
	
	return 0;
}
