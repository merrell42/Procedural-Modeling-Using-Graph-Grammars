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
#include "../mesh_extraction/obj_loader.h"
#include "../mesh_extraction/half_edge_mesh.h"
#include "../mesh_extraction/type_extractor.h"
#include "../mesh_extraction/seed_grammar_writer.h"

using namespace std;
using Json = nlohmann::json;

// Convert an OBJ mesh into a pmugg seed grammar JSON file.
static int meshToGrammar(const char* objPath, const char* outPath) {
	mesh_extraction::ObjMesh obj;
	std::string err;
	if (!mesh_extraction::loadObj(objPath, obj, &err)) {
		cerr << "loadObj failed: " << err << endl;
		return 1;
	}
	mesh_extraction::HalfEdgeMesh hem;
	if (!mesh_extraction::buildHalfEdgeMesh(obj, hem, /*useGroupAsVolume*/ false, &err)) {
		cerr << "buildHalfEdgeMesh failed: " << err << endl;
		return 1;
	}
	mesh_extraction::ExtractedTypes types;
	mesh_extraction::TypeExtractionConfig tcfg;
	if (!mesh_extraction::extractTypes(hem, obj, tcfg, types, &err)) {
		cerr << "extractTypes failed: " << err << endl;
		return 1;
	}
	if (!mesh_extraction::writeSeedGrammar(hem, types, "seed", outPath, &err)) {
		cerr << "writeSeedGrammar failed: " << err << endl;
		return 1;
	}
	cout << "Wrote seed grammar to " << outPath
	     << " (" << types.vertexTypes.size() << " vertex types, "
	     << types.edgeTypes.size() << " edge types, "
	     << types.faceTypes.size() << " face types)" << endl;

	// Sanity-check that the JSON we just wrote loads via the normal pmugg
	// grammar loader. The mutator isn't invoked here -- this is a parse check.
	try {
		Json parsed = readJsonFile(outPath, false);
		auto* grammar = GraphGrammar::import(parsed);
		cout << "  Round-trip: GraphGrammar::import succeeded." << endl;
		delete grammar;
	} catch (const std::exception& e) {
		cerr << "  Round-trip: GraphGrammar::import threw: " << e.what() << endl;
		return 2;
	}
	return 0;
}

// The main function for the command line version.
int main(int argc, char** argv) {
	if (argc >= 4 && std::string(argv[1]) == "--mesh-to-grammar") {
		return meshToGrammar(argv[2], argv[3]);
	}

	vector<string> filePaths = {
		"../../grammar data/3D Complex Shapes/castle.json",
		"../../grammar data/2D Basic Shapes/square filled.json",
		"../../grammar data/3D Shapes/house1.json",
		"../../grammar data/3D Shapes/docks.json",
	};
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
