#include "pch.h"
#include "generate.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <chrono>
#include "../graph_grammar.h"
#include "../mutator.h"
#include "../primitives/primitives.h"
#include "../graph_drawing/model.h"
#include "../util/util.h"
#include "../settings.h"
#include "../json versioning/read_json_file.h"

using namespace std;
using Json = nlohmann::json;

Model* model;
Mutator* mutator;

// Initialize the model and mutator from the JSON file. Return messages in the result string.
void initialize(const char* filePath, char* result, int len, int seed) {
	resetRandom(seed);

	try {
		Json parsed = readJsonFile(filePath);
		auto grammar = GraphGrammar::import(parsed);
		model = new Model();
		mutator = new Mutator(model, grammar);
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
}

// Reset the model and mutator.
void reset(int seed) {
	model->reset();
	mutator->reset();
	resetRandom(seed);
}

// Iterate some number of steps.
void iterate(int steps) {
	mutator->iterate(steps);
}

// Iterate until a certain amount of time has passed.
int iterateToTime(float timeSeconds) {
	int steps = 0;
	auto startTime = std::chrono::high_resolution_clock::now();
	auto targetDuration = std::chrono::duration<float>(timeSeconds);
	
	while (std::chrono::high_resolution_clock::now() - startTime < targetDuration) {
		mutator->iterate(1);
		steps++;
	}
	return steps;
}

// Return the number of faces in the current model.
int getNumFaces() {
	return (int)model->getCurrent()->getFaceMap().size();
}

// Return the current mesh.
MeshCpp getMesh() {
	return model->getCurrent()->exportMesh();
}

// Set the size of the model.
void setSize(float x, float y, float z) {
	std::vector<double> extents = {x, y, z};
	globalSettings["Extents"] = extents;
}

// Free memory for the mesh.
void destroyMesh(MeshCpp& mesh) {
	freeMeshMemory(mesh);
}
